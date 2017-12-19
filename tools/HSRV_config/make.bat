@echo off

@echo STC
if "%%~1" neq "" (
    set "OUT_DIR=%~1"
) else (
    set "OUT_DIR=deploy"
)

@echo 1. Compiling to exe...
python "..\..\..\pyinstaller-develop\pyinstaller.py" ./HSRV_config.py --onefile --windowed
@echo 1. Done.

@echo 2. Copying resources to %OUT_DIR%...
mkdir "%OUT_DIR%"
copy dist\HSRV_config.exe "%OUT_DIR%\"
mkdir "%OUT_DIR%\res"
copy res\HSRV_config.ui "%OUT_DIR%\res"
mkdir "%OUT_DIR%\conf"
copy conf\HSRV_config.ini "%OUT_DIR%\conf"
copy README.txt "%OUT_DIR%\"
@echo 2. Done.
