@echo off
if not exist build mkdir build
pushd build
cl /nologo /Z7 /FC /W4 /WX /wd4100 /wd4189 ..\main.c /link /INCREMENTAL:NO user32.lib
popd
