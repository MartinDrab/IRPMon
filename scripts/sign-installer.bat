set SIGNEXE="c:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\signtool.exe"
set FILES="..\bin\irpmon-1.0-setup.exe" 
set CROSSCERT="m:\cert\codesigning2017\crosssigning.crt"
%SIGNEXE% sign /n "Martin Drab" /a /fd sha1 /t http://time.certum.pl /ac %CROSSCERT% /v %FILES%
%SIGNEXE% sign /as /n "Martin Drab" /a /fd sha256 /tr http://time.certum.pl /ac %CROSSCERT% /v %FILES%
