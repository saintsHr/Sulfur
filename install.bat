@echo off
setlocal enabledelayedexpansion

set ICON_DIR=%APPDATA%\Sulfur\icons
set BIN_DIR=%USERPROFILE%\AppData\Local\Sulfur\bin

if not exist "%ICON_DIR%" mkdir "%ICON_DIR%"
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"

if not exist assets\sulfur.png (
    echo Error: icon assets\sulfur.png not found!
    exit /b 1
)

powershell -Command "Add-Type -AssemblyName System.Drawing; $img=[System.Drawing.Image]::FromFile('assets\sulfur.png'); $icon=[System.Drawing.Icon]::FromHandle($img.GetHicon()); $fs=New-Object IO.FileStream('%ICON_DIR%\sulfur.ico','Create'); $icon.Save($fs); $fs.Close(); $icon.Dispose(); $img.Dispose()"

if not exist slfr.exe (
    echo Error: program 'slfr.exe' not found in the current directory!
    exit /b 1
)

copy /Y slfr.exe "%BIN_DIR%\slfr.exe" >nul

echo %PATH% | find /i "%BIN_DIR%" >nul
if errorlevel 1 (
    setx PATH "%BIN_DIR%;%PATH%"
)

assoc .slfr=SulfurFile
ftype SulfurFile="%BIN_DIR%\slfr.exe" "%%1"
reg add "HKCU\Software\Classes\SulfurFile\DefaultIcon" /ve /d "%ICON_DIR%\sulfur.ico" /f

echo Installation complete!
pause
