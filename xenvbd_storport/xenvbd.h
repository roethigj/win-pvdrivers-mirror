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

#if !defined(_XENVBD_H_)
#define _XENVBD_H_

#include <ntddk.h>
#include <wdm.h>
#include <initguid.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <storport.h>
#include <ntddscsi.h>
#include <ntdddisk.h>
#include <stdlib.h>

#define __DRIVER_NAME "XenVbd"

#include <xen_windows.h>
#include <xen_public.h>
#include <io/protocols.h>
#include <memory.h>
#include <event_channel.h>
#include <hvm/params.h>
#include <hvm/hvm_op.h>
#include <io/ring.h>
#include <io/blkif.h>
#include <io/xenbus.h>

#include "../xenvbd_common/common.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define BLK_RING_SIZE __RING_SIZE((blkif_sring_t *)0, PAGE_SIZE)

#define SCSIOP_UNMAP 0x42

#define VPD_BLOCK_LIMITS 0xB0

#define MAX_SHADOW_ENTRIES  64
#define SHADOW_ID_ID_MASK   0x03FF /* maximum of 1024 requests - currently use a maximum of 64 though */
#define SHADOW_ID_DUMP_FLAG 0x8000 /* indicates the request was generated by dump mode */

#define SHADOW_ENTRIES min(MAX_SHADOW_ENTRIES, BLK_RING_SIZE)

/* if this is ever increased to more than 1 then we need a way of tracking it properly */
#define DUMP_MODE_UNALIGNED_PAGES 1 /* only for unaligned buffer use */

struct {
  ULONG device_state;
  KEVENT device_state_event;
  STOR_DPC dpc;
  PIO_WORKITEM disconnect_workitem;
  PIO_WORKITEM connect_workitem;
  blkif_shadow_t shadows[MAX_SHADOW_ENTRIES];
  USHORT shadow_free_list[MAX_SHADOW_ENTRIES];
  USHORT shadow_free;
  USHORT shadow_min_free;
  ULONG grant_tag;
  PDEVICE_OBJECT pdo;
  PDEVICE_OBJECT fdo;
  XN_HANDLE handle;
  evtchn_port_t event_channel;
  blkif_front_ring_t ring;
  blkif_sring_t *sring;
  grant_ref_t sring_gref; 
  KEVENT backend_event;
  ULONG backend_state;
  UCHAR last_sense_key;
  UCHAR last_additional_sense_code;
  blkif_response_t tmp_rep;
  XENVBD_DEVICETYPE device_type;
  XENVBD_DEVICEMODE device_mode;
  ULONG bytes_per_sector; /* 512 for disk, 2048 for CDROM) */
  ULONG hw_bytes_per_sector; /* underlying hardware format, eg 4K */
  ULONGLONG total_sectors;
  CHAR serial_number[64];
  ULONG feature_flush_cache;
  ULONG feature_discard;
  ULONG feature_barrier;
  LIST_ENTRY srb_list;
  BOOLEAN aligned_buffer_in_use;
  STOR_POWER_ACTION power_action;
  STOR_DEVICE_POWER_STATE power_state;
  ULONG aligned_buffer_size;
  PVOID aligned_buffer;
/*  
  ULONGLONG interrupts;
  ULONGLONG aligned_requests;
  ULONGLONG aligned_bytes;
  ULONGLONG unaligned_requests;
  ULONGLONG unaligned_bytes;
*/
  /* this is the size of the buffer to allocate at the end of DeviceExtenstion. It includes an extra PAGE_SIZE-1 bytes to assure that we can always align to PAGE_SIZE */
  #define UNALIGNED_BUFFER_DATA_SIZE ((BLKIF_MAX_SEGMENTS_PER_REQUEST + 1) * PAGE_SIZE - 1)
  #define UNALIGNED_BUFFER_DATA_SIZE_DUMP_MODE ((DUMP_MODE_UNALIGNED_PAGES + 1) * PAGE_SIZE - 1)
  /* this has to be right at the end of DeviceExtension */
  /* can't allocate too much data in dump mode so size DeviceExtensionSize accordingly */
  UCHAR aligned_buffer_data[1];
} typedef XENVBD_DEVICE_DATA, *PXENVBD_DEVICE_DATA;

#endif
