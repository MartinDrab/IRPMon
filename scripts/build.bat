set "VSCMD_START_DIR=%CD%"
call "c:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_x64 8.1 -vcvars_ver=14.1
MSBuild ..\irpmon.sln /Property:Configuration=Release /Property:Platform=Win32
MSBuild ..\irpmon.sln /Property:Configuration=Release /Property:Platform=x64


