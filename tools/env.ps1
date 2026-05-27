# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
# cspell:ignore LASTEXITCODE
[CmdletBinding()]
param(
  [Parameter(ValueFromRemainingArguments = $true)]
  [string[]]$CommandArgs
)

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$LynxOssEnvAndroidNdkVersion = "21.1.6352462"
$LynxOssEnvHarmonySdkVersion = "6.0.0.868"
$LynxOssEnvCommonPathSuffixes = @(
  "tools_shared",
  "tools\gn_tools"
)
$LynxOssEnvBuildtoolsPathSuffixes = @(
  "llvm\bin",
  "llvm",
  "gn",
  "ninja",
  "sccache",
  "emsdk",
  "emsdk\upstream\emscripten",
  "node\bin",
  "node",
  "ktlint"
)
$LynxOssEnvPythonPathSuffixes = @(
  "third_party\py_deps"
)
$LynxOssEnvToolCandidates = @(
  "gn",
  "ninja",
  "sccache",
  "node",
  "npm",
  "npx",
  "pnpm",
  "corepack",
  "python3",
  "pip3",
  "clang",
  "clang++",
  "emcc",
  "em++",
  "ktlint",
  "java",
  "adb",
  "git"
)

$Script:LynxOssEnvIsDotSourced = $MyInvocation.InvocationName -eq "."
$Script:LynxOssEnvScriptPath = if ($PSCommandPath) { $PSCommandPath } else { $MyInvocation.MyCommand.Path }

function Show-LynxOssEnvUsage {
  @"
Usage:
  tools\env.ps1 --list-tools
  tools\env.ps1 -- <command> [args...]
  tools\env.ps1 <command> [args...]

Description:
  Sets up the Lynx no-side-effect development environment.
  When executed directly, it runs the given command with the environment applied.
  When dot-sourced, it only exports the environment variables.

Examples:
  .\tools\env.ps1 --list-tools
  .\tools\env.ps1 node -v
  .\tools\env.ps1 pnpm --version
  .\tools\env.ps1 -- powershell -Command '$env:LYNX_DIR'
"@
}

function Add-LynxOssPathEntry {
  param(
    [string]$PathEntry
  )

  if ([string]::IsNullOrWhiteSpace($PathEntry) -or -not (Test-Path -LiteralPath $PathEntry)) {
    return
  }

  $normalizedCandidate = [System.IO.Path]::GetFullPath($PathEntry).TrimEnd('\')
  $existingEntries = @()
  if ($env:PATH) {
    $existingEntries = $env:PATH -split ';' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
  }

  foreach ($existingEntry in $existingEntries) {
    $normalizedEntry = $existingEntry.TrimEnd('\')
    try {
      $normalizedEntry = [System.IO.Path]::GetFullPath($existingEntry).TrimEnd('\')
    } catch {
    }

    if ($normalizedEntry.Equals($normalizedCandidate, [System.StringComparison]::OrdinalIgnoreCase)) {
      return
    }
  }

  if ($env:PATH) {
    $env:PATH = "$PathEntry;$env:PATH"
  } else {
    $env:PATH = $PathEntry
  }
}

function Add-LynxOssPythonPathEntry {
  param(
    [string]$PathEntry
  )

  if ([string]::IsNullOrWhiteSpace($PathEntry) -or -not (Test-Path -LiteralPath $PathEntry)) {
    return
  }

  $normalizedCandidate = [System.IO.Path]::GetFullPath($PathEntry).TrimEnd('\')
  $existingEntries = @()
  if ($env:PYTHONPATH) {
    $existingEntries = $env:PYTHONPATH -split ';' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
  }

  foreach ($existingEntry in $existingEntries) {
    $normalizedEntry = $existingEntry.TrimEnd('\')
    try {
      $normalizedEntry = [System.IO.Path]::GetFullPath($existingEntry).TrimEnd('\')
    } catch {
    }

    if ($normalizedEntry.Equals($normalizedCandidate, [System.StringComparison]::OrdinalIgnoreCase)) {
      return
    }
  }

  if ($env:PYTHONPATH) {
    $env:PYTHONPATH = "$PathEntry;$env:PYTHONPATH"
  } else {
    $env:PYTHONPATH = $PathEntry
  }
}

function Set-LynxOssCommonEnvironment {
  $toolsPath = Split-Path -Parent $Script:LynxOssEnvScriptPath
  $lynxDir = Split-Path -Parent $toolsPath

  $env:LYNX_DIR = $lynxDir
  if (-not $env:LYNX_ROOT_DIR) {
    $env:LYNX_ROOT_DIR = $lynxDir
  }
  if (-not $env:BUILDTOOLS_DIR) {
    $env:BUILDTOOLS_DIR = Join-Path $lynxDir "buildtools"
  }
  if (-not $env:COREPACK_HOME) {
    $env:COREPACK_HOME = Join-Path $env:BUILDTOOLS_DIR "corepack"
  }
  # create pnpm shim in $COREPACK_HOME/pnpm (configured in dependencies/DEPS, "prepare_pnpm_shim")
  Add-LynxOssPathEntry (Join-Path $env:COREPACK_HOME "pnpm")

  foreach ($pathSuffix in $LynxOssEnvBuildtoolsPathSuffixes) {
    Add-LynxOssPathEntry (Join-Path $env:BUILDTOOLS_DIR $pathSuffix)
  }
  foreach ($pathSuffix in $LynxOssEnvCommonPathSuffixes) {
    Add-LynxOssPathEntry (Join-Path $lynxDir $pathSuffix)
  }
  foreach ($pathSuffix in $LynxOssEnvPythonPathSuffixes) {
    Add-LynxOssPythonPathEntry (Join-Path $lynxDir $pathSuffix)
  }
}

function Set-LynxOssAndroidEnvironment {
  $androidHome = $env:ANDROID_HOME
  if (-not $androidHome) {
    return
  }

  if (-not $env:ANDROID_SDK) {
    $env:ANDROID_SDK = $androidHome
  }

  if ($env:ANDROID_NDK_PATH) {
    $env:ANDROID_NDK = $env:ANDROID_NDK_PATH
  } elseif (-not $env:ANDROID_NDK) {
    $env:ANDROID_NDK = Join-Path $androidHome "ndk\$LynxOssEnvAndroidNdkVersion"
  }

  $env:ANDROID_NDK_21 = Join-Path $androidHome "ndk\$LynxOssEnvAndroidNdkVersion"
}

function Set-LynxOssHarmonyEnvironment {
  $harmonyHome = $env:HARMONY_HOME

  if (-not $harmonyHome -and $env:COMMANDLINE_TOOL_BASE_DIR) {
    $versionRoot = Join-Path $env:COMMANDLINE_TOOL_BASE_DIR $LynxOssEnvHarmonySdkVersion
    $commandlineToolDir = Join-Path $versionRoot "command-line-tools"
    $sdkDir = Join-Path $commandlineToolDir "sdk"
    if (Test-Path -LiteralPath $sdkDir) {
      $env:COMMANDLINE_TOOL_DIR = $commandlineToolDir
      $env:HARMONY_HOME = $sdkDir
      $harmonyHome = $sdkDir
    }
  }

  if (-not $harmonyHome) {
    return
  }

  $toolParent = Split-Path -Parent $harmonyHome
  Add-LynxOssPathEntry (Join-Path $toolParent "bin")

  if (-not $env:HOS_SDK_HOME) {
    $env:HOS_SDK_HOME = $harmonyHome
  }
  if (-not $env:DEVECO_SDK_HOME) {
    $env:DEVECO_SDK_HOME = $harmonyHome
  }
  if (-not $env:DEVECO_NODE_HOME) {
    $env:DEVECO_NODE_HOME = Join-Path $env:BUILDTOOLS_DIR "node"
  }
}

function Set-LynxOssEnvironment {
  Set-LynxOssCommonEnvironment
  Set-LynxOssAndroidEnvironment
  Set-LynxOssHarmonyEnvironment
}

function Get-LynxOssResolvedToolPath {
  param(
    [string]$ToolName
  )

  $commandInfo = Get-Command -Name $ToolName -ErrorAction SilentlyContinue | Select-Object -First 1
  if (-not $commandInfo) {
    return $null
  }

  if ($commandInfo.Path) {
    return $commandInfo.Path
  }

  if ($commandInfo.Source -and (Test-Path -LiteralPath $commandInfo.Source)) {
    return $commandInfo.Source
  }

  return $commandInfo.Definition
}

function Show-LynxOssResolvedTools {
  $lynxDir = $env:LYNX_DIR
  $repoRoot = $env:LYNX_ROOT_DIR
  $buildtoolsDir = $env:BUILDTOOLS_DIR
  $normalizedLynxDir = [System.IO.Path]::GetFullPath($lynxDir).TrimEnd('\')
  $normalizedRepoRoot = [System.IO.Path]::GetFullPath($repoRoot).TrimEnd('\')
  $foundAny = $false

  Write-Output "LYNX_DIR=$lynxDir"
  Write-Output "LYNX_ROOT_DIR=$repoRoot"
  Write-Output "BUILDTOOLS_DIR=$buildtoolsDir"
  Write-Output "Available tools:"

  foreach ($toolName in $LynxOssEnvToolCandidates) {
    $toolPath = Get-LynxOssResolvedToolPath -ToolName $toolName
    if (-not $toolPath) {
      continue
    }

    $toolOrigin = "system"
    try {
      $normalizedToolPath = [System.IO.Path]::GetFullPath($toolPath).TrimEnd('\')
      if ($normalizedToolPath.StartsWith("$normalizedLynxDir\", [System.StringComparison]::OrdinalIgnoreCase) -or
          $normalizedToolPath.StartsWith("$normalizedRepoRoot\", [System.StringComparison]::OrdinalIgnoreCase) -or
          $normalizedToolPath.Equals($normalizedLynxDir, [System.StringComparison]::OrdinalIgnoreCase) -or
          $normalizedToolPath.Equals($normalizedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $toolOrigin = "repo"
      }
    } catch {
    }

    "{0,-10} [{1}] {2}" -f $toolName, $toolOrigin, $toolPath
    $foundAny = $true
  }

  if (-not $foundAny) {
    Write-Output "  (no known tools were resolved)"
  }
}

$listTools = $false

while ($CommandArgs.Count -gt 0 -and $CommandArgs[0] -eq "--list-tools") {
  $listTools = $true
  if ($CommandArgs.Count -eq 1) {
    $CommandArgs = @()
  } else {
    $CommandArgs = $CommandArgs[1..($CommandArgs.Count - 1)]
  }
}

if ($CommandArgs.Count -gt 0 -and $CommandArgs[0] -eq "--") {
  if ($CommandArgs.Count -eq 1) {
    $CommandArgs = @()
  } else {
    $CommandArgs = $CommandArgs[1..($CommandArgs.Count - 1)]
  }
}

if ($CommandArgs.Count -eq 1 -and ($CommandArgs[0] -eq "-h" -or $CommandArgs[0] -eq "--help")) {
  Show-LynxOssEnvUsage
  if (-not $Script:LynxOssEnvIsDotSourced) {
    exit 0
  }
  return
}

Set-LynxOssEnvironment

if ($listTools) {
  Show-LynxOssResolvedTools
  if (-not $Script:LynxOssEnvIsDotSourced) {
    exit 0
  }
  return
}

if ($CommandArgs.Count -eq 0) {
  return
}

$command = $CommandArgs[0]
$commandArguments = @()
if ($CommandArgs.Count -gt 1) {
  $commandArguments = $CommandArgs[1..($CommandArgs.Count - 1)]
}

& $command @commandArguments
$exitCode = if ($LASTEXITCODE -ne $null) { $LASTEXITCODE } elseif ($?) { 0 } else { 1 }

if (-not $Script:LynxOssEnvIsDotSourced) {
  exit $exitCode
}
