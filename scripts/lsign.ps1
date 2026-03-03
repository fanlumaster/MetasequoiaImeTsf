# 测试证书的签名
. "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\mt.exe" -manifest .\MetasequoiaImeServer.manifest -outputresource:.\build\bin\Debug\MetasequoiaImeServer.exe; 1
."C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe" sign /fd SHA256 /v /s PrivateCertStore /n "Test Certificate - For Internal scitertest Use Only" /a .\build64\Debug\MetasequoiaImeTsf.dll

# 真实证书的签名
."C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe" sign /sha1 "<Your Certum Thumbprint>" /tr http://time.certum.pl /td sha256 /fd sha256 /v .\build64\Debug\MetasequoiaImeTsf.dll