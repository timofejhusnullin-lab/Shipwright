@echo off
rem Fast build: only changed files are recompiled, then the game starts.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\localbuild.ps1" -Mode dev %*
if errorlevel 1 pause
