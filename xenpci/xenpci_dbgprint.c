/*
PV Drivers for Windows Xen HVM Domains
Copyright (C) 2007 James Harper

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "xenpci.h"
#include <aux_klib.h>

#pragma intrinsic(_enable)
#pragma intrinsic(_disable)

#pragma pack(1)
#ifdef _AMD64_
typedef struct __KIDT_ENTRY
{
  USHORT addr_0_15;
  USHORT selector;
  USHORT access;
  USHORT addr_16_31;
  ULONG  addr_32_63;
  ULONG  reserved;
} IDT_ENTRY, *PIDT_ENTRY;
#else
typedef struct __KIDT_ENTRY
{
  USHORT addr_0_15;
  USHORT selector;
  USHORT access;
  USHORT addr_16_31;
} IDT_ENTRY, *PIDT_ENTRY;
#endif
#pragma pack()

#pragma pack(2)
typedef struct _IDT
{
  USHORT limit;
  PIDT_ENTRY entries;
} IDT, *PIDT;
#pragma pack()

/* Not really necessary but keeps PREfast happy */
static KBUGCHECK_CALLBACK_ROUTINE XenPci_BugcheckCallback;

KBUGCHECK_CALLBACK_RECORD callback_record;

extern VOID Int2dHandlerNew(VOID);
extern PVOID Int2dHandlerOld;

static VOID
XenPci_BugcheckCallback(PVOID buffer, ULONG length)
{
  NTSTATUS status;
  KBUGCHECK_DATA bugcheck_data;
  
  UNREFERENCED_PARAMETER(buffer);
  UNREFERENCED_PARAMETER(length);
  
  bugcheck_data.BugCheckDataSize  = sizeof(bugcheck_data);
  status = AuxKlibGetBugCheckData(&bugcheck_data);
  if(!NT_SUCCESS(status))
  {
    KdPrint((__DRIVER_NAME "     AuxKlibGetBugCheckData returned %08x\n", status));
    return;
  }
  KdPrint((__DRIVER_NAME "     Bug check 0x%08X (0x%p, 0x%p, 0x%p, 0x%p)\n",
    bugcheck_data.BugCheckCode, bugcheck_data.Parameter1, bugcheck_data.Parameter2, bugcheck_data.Parameter3, bugcheck_data.Parameter4));
}

static BOOLEAN debug_port_enabled = FALSE;

/* This appears to be called with interrupts disabled already, so no need to go to HIGH_LEVEL or anything like that */
static void XenDbgPrint(PCHAR string, ULONG length)
{
  ULONG i;
  //KIRQL old_irql = 0;

  //KeRaiseIrql(HIGH_LEVEL, &old_irql);
  for (i = 0; i < length; i++)
    WRITE_PORT_UCHAR(XEN_IOPORT_LOG, string[i]);
  //KeLowerIrql(old_irql);
}

VOID
Int2dHandlerProc(ULONG_PTR dbg_type, PVOID arg2, PVOID arg3, PVOID arg4, PVOID arg5)
{
  CHAR buf[512];

  switch (dbg_type)
  {
  case 1: /* DbgPrint */
    XenDbgPrint((PCHAR)arg2, (ULONG)(ULONG_PTR)arg3);
    break;
  case 2: /* ASSERT */
  case 3: /* ??? */
  case 4: /* ??? */
    break;
  default:
    RtlStringCbPrintfA(buf, ARRAY_SIZE(buf), "*** %d %08x %08x %08x %08x\n", dbg_type, arg2, arg3, arg4, arg5);
    XenDbgPrint(buf, (ULONG)strlen(buf));
    break;
  }
  return;
}

static VOID
XenPci_DbgPrintCallback(PSTRING output, ULONG component_id, ULONG level)
{
  UNREFERENCED_PARAMETER(component_id);
  UNREFERENCED_PARAMETER(level);
  
  XenDbgPrint(output->Buffer, output->Length);
}

#if 0
typedef struct _hook_info {
  PIDT_ENTRY idt_entry;
} hook_info_t;
#endif

#if (NTDDI_VERSION < NTDDI_VISTA)
#ifndef _AMD64_ // can't patch IDT on AMD64 unfortunately - results in bug check 0x109
static VOID
XenPci_HookDbgPrint_High(PVOID context)
{
  IDT idt;
  PIDT_ENTRY idt_entry;

  UNREFERENCED_PARAMETER(context);  
 
  idt.limit = 0;
  __sidt(&idt);
  idt_entry = &idt.entries[0x2D];
  #ifdef _AMD64_ 
  Int2dHandlerOld = (PVOID)((ULONG_PTR)idt_entry->addr_0_15 | ((ULONG_PTR)idt_entry->addr_16_31 << 16) | ((ULONG_PTR)idt_entry->addr_32_63 << 32));
  #else
  Int2dHandlerOld = (PVOID)((ULONG_PTR)idt_entry->addr_0_15 | ((ULONG_PTR)idt_entry->addr_16_31 << 16));
  #endif
  idt_entry->addr_0_15 = (USHORT)(ULONG_PTR)Int2dHandlerNew;
  idt_entry->addr_16_31 = (USHORT)((ULONG_PTR)Int2dHandlerNew >> 16);
  #ifdef _AMD64_ 
  idt_entry->addr_32_63 = (ULONG)((ULONG_PTR)Int2dHandlerNew >> 32);
  #endif
}
#endif
#endif

NTSTATUS
XenPci_HookDbgPrint()
{
  NTSTATUS status = STATUS_SUCCESS;

  if (READ_PORT_USHORT(XEN_IOPORT_MAGIC) == 0x49d2
    || READ_PORT_USHORT(XEN_IOPORT_MAGIC) == 0xd249)
  {
    //#pragma warning(suppress:4055)
    //DbgSetDebugPrintCallback = (PDBG_SET_DEBUGPRINT_CALLBACK)MmGetSystemRoutineAddress((PUNICODE_STRING)&DbgSetDebugPrintCallbackName);
#if (NTDDI_VERSION >= NTDDI_VISTA)
    KdPrint((__DRIVER_NAME "     DbgSetDebugPrintCallback found\n"));
    status = DbgSetDebugPrintCallback(XenPci_DbgPrintCallback, TRUE);
    if (!NT_SUCCESS(status))
    {
      KdPrint((__DRIVER_NAME "     DbgSetDebugPrintCallback failed - %08x\n", status));
    }
    //DbgSetDebugFilterState(componentid, level, state);
    DbgSetDebugFilterState(DPFLTR_DEFAULT_ID, 0xFFFFFFFF, TRUE);
#else
    KdPrint((__DRIVER_NAME "     DbgSetDebugPrintCallback not found\n"));      
#ifndef _AMD64_ // can't patch IDT on AMD64 unfortunately - results in bug check 0x109
    XenPci_HighSync(XenPci_HookDbgPrint_High, XenPci_HookDbgPrint_High, NULL);
#endif
#endif
  }
  else
  {
    status = STATUS_UNSUCCESSFUL;
  }
  
  KeInitializeCallbackRecord(&callback_record);
  if (!KeRegisterBugCheckCallback(&callback_record, XenPci_BugcheckCallback, NULL, 0, (PUCHAR)"XenPci"))
  {
    KdPrint((__DRIVER_NAME "     KeRegisterBugCheckCallback failed\n"));
    status = STATUS_UNSUCCESSFUL;
  }

  return status;
}
