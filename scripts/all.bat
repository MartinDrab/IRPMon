call build %1%
IF %errorlevel% NEQ 0 goto failure
call sign %1%
IF %errorlevel% NEQ 0 goto failure
"c:\Program Files (x86)\Inno Setup 6\iscc.exe" installer.iss
IF %errorlevel% NEQ 0 goto failure
call sign-installer
IF %errorlevel% NEQ 0 goto failure
goto success
:failure
echo BUILD FAILED! Error code %errorlevel%
:success
