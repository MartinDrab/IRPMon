set SIGNEXE="c:\Program Files (x86)\Windows Kits\10\bin\10.0.15063.0\x64\signtool.exe"
set CONFIG=%1%
set FILES=^
	"..\resources\uninst-6.0.2 (u)-86794d44db.e32"^
	..\bin\x64\%CONFIG%\irpmon.exe^
	..\bin\x64\%CONFIG%\kernel\irpmndrv.sys^
	..\bin\x64\%CONFIG%\kernel\kbase.dll^
	..\bin\x64\%CONFIG%\kernel\regman.dll^
	..\bin\x64\%CONFIG%\dlls\irpmondll.dll^
	..\bin\x64\%CONFIG%\dlls\requests.dll^
	..\bin\x64\%CONFIG%\parser\hexer.dll^
	..\bin\x64\%CONFIG%\parser\secdesc.dll^
	..\bin\x64\%CONFIG%\parser\pnp-ids.dll^
	..\bin\x64\%CONFIG%\parser\pnp-devcaps.dll^
	..\bin\x64\%CONFIG%\parser\pnp-interface.dll^
	..\bin\x64\%CONFIG%\parser\keyboard.dll^
	..\bin\x64\%CONFIG%\parser\mouse.dll^
	..\bin\x64\%CONFIG%\dlls\pbase.dll^
	..\bin\x64\%CONFIG%\dlls\network-connector.dll^
	..\bin\x64\%CONFIG%\dlls\device-connector.dll^
	..\bin\Win32\%CONFIG%\irpmon.exe^
	..\bin\Win32\%CONFIG%\kernel\irpmndrv.sys^
	..\bin\Win32\%CONFIG%\kernel\kbase.dll^
	..\bin\Win32\%CONFIG%\kernel\regman.dll^
	..\bin\Win32\%CONFIG%\dlls\irpmondll.dll^
	..\bin\Win32\%CONFIG%\dlls\requests.dll^
	..\bin\Win32\%CONFIG%\parser\hexer.dll^
	..\bin\Win32\%CONFIG%\parser\secdesc.dll^
	..\bin\Win32\%CONFIG%\parser\pnp-ids.dll^
	..\bin\Win32\%CONFIG%\parser\pnp-devcaps.dll^
	..\bin\Win32\%CONFIG%\parser\pnp-interface.dll^
	..\bin\Win32\%CONFIG%\parser\keyboard.dll^
	..\bin\Win32\%CONFIG%\parser\mouse.dll^
	..\bin\Win32\%CONFIG%\dlls\pbase.dll^
	..\bin\Win32\%CONFIG%\dlls\network-connector.dll^
	..\bin\Win32\%CONFIG%\dlls\device-connector.dll^
	..\bin\Win32\%CONFIG%\parser\ea.dll^
	..\bin\x64\%CONFIG%\parser\ea.dll^
	..\bin\Win32\%CONFIG%\server\libserver.dll^
	..\bin\x64\%CONFIG%\server\libserver.dll^
	..\bin\Win32\%CONFIG%\server\server-app.exe^
	..\bin\x64\%CONFIG%\server\server-app.exe^
	..\bin\Win32\%CONFIG%\server\server-svc.exe^
	..\bin\x64\%CONFIG%\server\server-svc.exe

set CROSSCERT="m:\cert\codesigning2017\crosssigning.crt"
%SIGNEXE% sign /n "Martin Drab" /a /fd sha1 /t http://time.certum.pl /ac %CROSSCERT% /v %FILES%
%SIGNEXE% sign /as /n "Martin Drab" /a /fd sha256 /tr http://time.certum.pl /ac %CROSSCERT% /v %FILES%
