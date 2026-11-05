@echo off
rem Final build with full link-time optimization (slower link, fastest exe).
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\localbuild.ps1" -Mode release %*
pause
