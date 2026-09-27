@echo off
setlocal enabledelayedexpansion
if not exist bin\cloth_asset_obj mkdir bin\cloth_asset_obj
set "decoder_objects="
for %%s in (deps\brotli\c\common\*.c deps\brotli\c\dec\*.c) do (
    cl /nologo /O2 /MD /c /TC /DBROTLI_STATIC /Ideps\brotli\c\include "%%s" /Fo"bin\cloth_asset_obj\%%~ns.obj"
    if errorlevel 1 exit /b 1
    set "decoder_objects=!decoder_objects! bin\cloth_asset_obj\%%~ns.obj"
)
rem Link only this invocation's inputs, never stale objects left in the directory.
lib /nologo /OUT:bin\cloth_asset_decoder.lib !decoder_objects!
exit /b %errorlevel%
