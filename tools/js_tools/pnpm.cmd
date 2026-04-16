@echo off
setlocal enableextensions
setlocal enabledelayedexpansion

REM pnpm Windows shim that forwards all arguments to pnpm_wrapper.py

set "SCRIPT_DIR=%~dp0"
set "WRAPPER=%PNPM_WRAPPER%"
if not defined WRAPPER set "WRAPPER=%SCRIPT_DIR%pnpm_wrapper.py"

REM Choose Python interpreter
set "PY_EXE=%PYTHON%"
if not defined PY_EXE set "PY_EXE=%PYTHON3%"

if not defined PY_EXE (
  where py >nul 2>nul
  if %ERRORLEVEL%==0 (
    set "PY_IS_PY=1"
    set "PY_EXE=py"
  )
)
if not defined PY_EXE (
  where python >nul 2>nul
  if %ERRORLEVEL%==0 set "PY_EXE=python"
)
if not defined PY_EXE (
  where python3 >nul 2>nul
  if %ERRORLEVEL%==0 set "PY_EXE=python3"
)

if not defined PY_EXE (
  echo Error: no Python interpreter found. Set PYTHON or PYTHON3.
  exit /b 1
)

if defined PY_IS_PY (
  py -3 "%WRAPPER%" %*
) else (
  "%PY_EXE%" "%WRAPPER%" %*
)

exit /b %ERRORLEVEL%