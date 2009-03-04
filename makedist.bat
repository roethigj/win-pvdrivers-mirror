@echo off
IF NOT EXIST set_ddk_path.bat ECHO >set_ddk_path.bat SET DDK_PATH=C:\WinDDK\6001.18002

SET VERSION=0.10.0
SET BUILD_NUMBER=0
IF EXIST build_number.bat CALL build_number.bat

SET GPLPV_VERSION=%VERSION%.%BUILD_NUMBER%

SET /A BUILD_NUMBER=%BUILD_NUMBER%+1
ECHO >build_number.bat SET BUILD_NUMBER=%BUILD_NUMBER%

ECHO BUILDING %GPLPV_VERSION%

CALL set_ddk_path.bat

SET PV_DIR=%CD%

ECHO PV_DIR=%PV_DIR%

cmd /C "%DDK_PATH%\bin\setenv.bat %DDK_PATH%\ fre WXP && CD "%PV_DIR%" && build -cZg && call sign.bat && call wix.bat"

cmd /C "%DDK_PATH%\bin\setenv.bat %DDK_PATH%\ fre WNET && CD "%PV_DIR%" && build -cZg && call sign.bat && call wix.bat"

cmd /C "%DDK_PATH%\bin\setenv.bat %DDK_PATH%\ fre x64 WNET && CD "%PV_DIR%" && build -cZg && call sign.bat && call wix.bat"

cmd /C "%DDK_PATH%\bin\setenv.bat %DDK_PATH%\ fre WLH && CD "%PV_DIR%" && build -cZg && call sign.bat && call wix.bat"

cmd /C "%DDK_PATH%\bin\setenv.bat %DDK_PATH%\ fre x64 WLH && CD "%PV_DIR%" && build -cZg && call sign.bat && call wix.bat"

cmd /C "%DDK_PATH%\bin\setenv.bat %DDK_PATH%\ chk WXP && CD "%PV_DIR%" && build -cZg && call sign.bat && call wix.bat"

cmd /C "%DDK_PATH%\bin\setenv.bat %DDK_PATH%\ chk WNET && CD "%PV_DIR%" && build -cZg && call sign.bat && call wix.bat"

cmd /C "%DDK_PATH%\bin\setenv.bat %DDK_PATH%\ chk x64 WNET && CD "%PV_DIR%" && build -cZg && call sign.bat && call wix.bat"

cmd /C "%DDK_PATH%\bin\setenv.bat %DDK_PATH%\ chk WLH && CD "%PV_DIR%" && build -cZg && && call sign.bat call wix.bat"

cmd /C "%DDK_PATH%\bin\setenv.bat %DDK_PATH%\ chk x64 WLH && CD "%PV_DIR%" && build -cZg && call sign.bat && call wix.bat"
