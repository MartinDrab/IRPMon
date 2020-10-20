set CONFIG=%1%
set "VSCMD_START_DIR=%CD%"
call "c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_x64 10.0 -vcvars_ver=14.2
MSBuild ..\irpmon.sln /m /Property:Configuration=%CONFIG% /Property:Platform=Win32
if %errorlevel% NEQ 0 goto failure
MSBuild ..\irpmon.sln /m /Property:Configuration=%CONFIG% /Property:Platform=x64
if %errorlevel% NEQ 0 goto failure
call rsvars
if %errorlevel% NEQ 0 goto failure
MSBuild ../gui/irpmon.dproj /m /t:Build /p:config=%CONFIG% /p:platform=Win32
if %errorlevel% NEQ 0 goto failure
MSBuild ../gui/irpmon.dproj /m /t:Build /p:config=%CONFIG% /p:platform=Win64
if %errorlevel% NEQ 0 goto failure
goto success
:failure
echo BUILD FAILED! Error code %errorlevel%
:success


