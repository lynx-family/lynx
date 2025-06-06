# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$tools_path = Split-Path -Parent $MyInvocation.MyCommand.Path
$lynx_dir_path = Split-Path -Parent $tools_path
$lynx_root_dir_path = Split-Path -Parent $lynx_dir_path

function Setup-Environment($environ_key, $environ_value) {
  [Environment]::SetEnvironmentVariable($environ_key, $environ_value, "User")
}

function Get-Environment($environ_key) {
  $environ_value = [Environment]::GetEnvironmentVariable($environ_key, "User")
  return $environ_value
}

function Add-Environ($key, $value) {
  $currentValue = Get-Environment $key
  if ($currentValue) {
    if ($currentValue.Contains($value)) {
      return
    }
    $new_path = "$value;$currentValue"
  } else {
    $new_path = $value
  }
  Setup-Environment $key $new_path
}

function Lynx-Env-Setup {
    Setup-Environment 'LYNX_DIR' $lynx_dir_path
    Setup-Environment 'LYNX_ROOT_DIR' $lynx_root_dir_path
    $buildtoolsDir = Join-Path $lynx_root_dir_path 'buildtools'

    Setup-Environment 'BUILDTOOLS_DIR' $buildtoolsDir
    Add-Environ 'PATH' (Join-Path $buildtoolsDir 'llvm\bin')
    Add-Environ 'PATH' (Join-Path $buildtoolsDir 'ninja')
    Add-Environ 'PATH' (Join-Path $buildtoolsDir 'gn')
    Add-Environ 'PATH' (Join-Path $buildtoolsDir 'node')
    Add-Environ 'PATH' (Join-Path $lynx_dir_path 'tools_shared')
    
    Setup-Environment 'COREPACK_HOME' (Join-Path $buildtoolsDir 'corepack')
}

function Android-Env-Setup {
  $androidHome = Get-Environment 'ANDROID_HOME'
  
  if ($androidHome) {
    $androidNdk = Join-Path $androidHome 'ndk/21.1.6352462'
    Setup-Environment 'ANDROID_NDK' $androidNdk
    Setup-Environment 'ANDROID_NDK_21' $androidNdk
    Setup-Environment 'ANDROID_SDK' $androidNdk
  } else {
    Write-Host "Please setup ANDROID_HOME environment variable for android build first."
  }
}

function Python-Env-Setup {
  python $lynx_dir_path\tools\vpython_tools\vpython_env_setup.py
  $venv_path = Join-Path $lynx_root_dir_path '.venv'
  & $venv_path\Scripts\Activate.ps1
  Add-Environ 'PATH' (Join-Path $venv_path 'bin')
}

Lynx-Env-Setup
Android-Env-Setup
Python-Env-Setup
