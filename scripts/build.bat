set "VSCMD_START_DIR=%CD%"
call "c:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_x64 8.1 -vcvars_ver=14.1
MSBuild ..\irpmon.sln /Property:Configuration=Release /Property:Platform=Win32
MSBuild ..\irpmon.sln /Property:Configuration=Release /Property:Platform=x64
call rsvars
msbuild ../irpmon.groupproj /t:Build /p:config=Release /p:platform=Win32
msbuild ../gui/irpmon.dproj /t:Build /p:config=Release /p:platform=Win64


