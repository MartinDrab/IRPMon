call build %1%
IF %errorlevel% NEQ 0 goto failure
rem call sign %1%
rem IF %errorlevel% NEQ 0 goto failure
"c:\Program Files (x86)\Inno Setup 6\iscc.exe" installer.iss
IF %errorlevel% NEQ 0 goto failure
rem call sign-installer
rem IF %errorlevel% NEQ 0 goto failure
goto success
:failure
echo BUILD FAILED! Error code %errorlevel%
:success
