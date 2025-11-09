@echo off
REM Quick script to make scripted modules visible immediately for Alice
REM - Adds SLICER_QTSCRIPTEDMODULES_PATHS into launcher INI (idempotent)
REM - Starts Alice with a clean settings dir and explicit additional-module-paths

setlocal
set INST=C:\S\rs-install
set MODPATH=%INST%\lib\Slicer-5.8\qt-scripted-modules
set CLEANSET=C:\S\tmp\alice-clean

if not exist "%MODPATH%" (
  echo [ERROR] Module path not found: %MODPATH%
  exit /b 1
)

if not exist "%CLEANSET%" mkdir "%CLEANSET%" >nul 2>&1

REM Patch launcher INI to add environment variable for scripted modules
set INI=%INST%\bin\AliceLauncherSettings.ini
if not exist "%INI%" (
  echo [ERROR] Launcher settings not found: %INI%
  exit /b 1
)

powershell -NoProfile -Command ^
  "$ini='%INI%'; ^
   $c=Get-Content -Raw -Path $ini; ^
   if($c -notmatch '(?m)^SLICER_QTSCRIPTEDMODULES_PATHS\s*='){ ^
     Add-Content -Path $ini -Value \"`r`n[EnvironmentVariables]`r`nSLICER_QTSCRIPTEDMODULES_PATHS=<APPLAUNCHER_SETTINGS_DIR>/../lib/Slicer-5.8/qt-scripted-modules`r`n\"; ^
     Write-Host 'Patched launcher INI.' ^
   } else { Write-Host 'Launcher INI already has SLICER_QTSCRIPTEDMODULES_PATHS.' }"

echo Launching Alice with clean settings and explicit module path ...
"%INST%\Alice.exe" ^
  --settings-path "%CLEANSET%" ^
  --no-splash --verbose --launcher-verbose ^
  --additional-module-paths "%MODPATH%"

endlocal

