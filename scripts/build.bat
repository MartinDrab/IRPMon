set CONFIG=%1%
set "VSCMD_START_DIR=%CD%"
call "c:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_x64 8.1 -vcvars_ver=14.1
MSBuild ..\irpmon.sln /Property:Configuration=%CONFIG% /Property:Platform=Win32
if errorlevel 1 goto failure
MSBuild ..\irpmon.sln /Property:Configuration=%CONFIG% /Property:Platform=x64
if errorlevel 1 goto failure
call rsvars
MSBuild ../irpmon.groupproj /t:Build /p:config=%CONFIG% /p:platform=Win32
if errorlevel 1 goto failure
MSBuild ../gui/irpmon.dproj /t:Build /p:config=%CONFIG% /p:platform=Win64
if errorlevel 1 goto failure
goto success
:failure
echo BUILD FAILED!
:success


