set SIGNEXE="c:\Program Files (x86)\Windows Kits\10\bin\10.0.15063.0\x64\signtool.exe"
set FILES=irpmon.exe irpmndrv.sys kbase.dll regman.dll irpmondll.dll hexer.dll secdesc.dll pnp-ids.dll pnp-devcaps.dll pnp-interface.dll keyboard.dll mouse.dll pbase.dll
set CROSSCERT="m:\cert\codesigning2017\crosssigning.crt"
%SIGNEXE% sign /n "Martin Drab" /a /fd sha1 /t http://time.certum.pl /ac %CROSSCERT% /v %FILES%
%SIGNEXE% sign /as /n "Martin Drab" /a /fd sha256 /tr http://time.certum.pl /ac %CROSSCERT% /v %FILES%
