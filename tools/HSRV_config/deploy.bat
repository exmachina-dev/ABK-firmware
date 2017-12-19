@echo off

if "%1" neq "" (
    set "VERSION=%1"
) else (
    @echo Usage: deploy.bat VERSION
    exit /b 1
)

set "OUT_NAME=HSRV_config - v%VERSION%"
set "OUT_ARCHIVE=%OUT_NAME%.zip"

@echo 1. Building in %OUT_NAME%...
call make.bat "%OUT_NAME%"
@echo 1. Done...

@echo 2. Generating archive %OUT_ARCHIVE%...
del "%OUT_ARCHIVE%"
"C:\Program Files\7-Zip\7z.exe" a "%OUT_ARCHIVE%" "%OUT_NAME%\"
@echo 2. Done...
