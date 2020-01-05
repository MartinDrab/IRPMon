set SIGNEXE="c:\Program Files (x86)\Windows Kits\10\bin\10.0.15063.0\x64\signtool.exe"
set CONFIG=%1%
set FILES="..\resources\uninst-6.0.2 (u)-86794d44db.e32" ..\bin\Win32\%CONFIG%\IRPLauncher.exe ..\bin\x64\%CONFIG%\irpmon.exe ..\bin\x64\%CONFIG%\irpmndrv.sys ..\bin\x64\%CONFIG%\kbase.dll ..\bin\x64\%CONFIG%\regman.dll ..\bin\x64\%CONFIG%\irpmondll.dll ..\bin\x64\%CONFIG%\hexer.dll ..\bin\x64\%CONFIG%\secdesc.dll ..\bin\x64\%CONFIG%\pnp-ids.dll ..\bin\x64\%CONFIG%\pnp-devcaps.dll ..\bin\x64\%CONFIG%\pnp-interface.dll ..\bin\x64\%CONFIG%\keyboard.dll ..\bin\x64\%CONFIG%\mouse.dll ..\bin\x64\%CONFIG%\pbase.dll ..\bin\Win32\%CONFIG%\irpmon.exe ..\bin\Win32\%CONFIG%\irpmndrv.sys ..\bin\Win32\%CONFIG%\kbase.dll ..\bin\Win32\%CONFIG%\regman.dll ..\bin\Win32\%CONFIG%\irpmondll.dll ..\bin\Win32\%CONFIG%\hexer.dll ..\bin\Win32\%CONFIG%\secdesc.dll ..\bin\Win32\%CONFIG%\pnp-ids.dll ..\bin\Win32\%CONFIG%\pnp-devcaps.dll ..\bin\Win32\%CONFIG%\pnp-interface.dll ..\bin\Win32\%CONFIG%\keyboard.dll ..\bin\Win32\%CONFIG%\mouse.dll ..\bin\Win32\%CONFIG%\pbase.dll ..\bin\Win32\%CONFIG%\ea.dll ..\bin\x64\%CONFIG%\ea.dll

set CROSSCERT="m:\cert\codesigning2017\crosssigning.crt"
%SIGNEXE% sign /n "Martin Drab" /a /fd sha1 /t http://time.certum.pl /ac %CROSSCERT% /v %FILES%
%SIGNEXE% sign /as /n "Martin Drab" /a /fd sha256 /tr http://time.certum.pl /ac %CROSSCERT% /v %FILES%
