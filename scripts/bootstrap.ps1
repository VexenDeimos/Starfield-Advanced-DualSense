[CmdletBinding()]
param(
    [switch]$InstallXMake
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

if (-not (Get-Command xmake -ErrorAction SilentlyContinue)) {
    if (-not $InstallXMake) {
        throw 'XMake 3.0.0+ was not found. Install XMake, or rerun with -InstallXMake if winget is available.'
    }
    if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
        throw 'winget is not available. Install XMake 3.0.0+ manually from xmake.io.'
    }
    winget install --id Xmake-io.Xmake --exact --accept-package-agreements --accept-source-agreements
}

$xmakeVersion = (& xmake --version | Select-Object -First 1)
Write-Host "Using $xmakeVersion"

$commonLib = Join-Path $repoRoot 'external\CommonLibSF'
if (-not (Test-Path (Join-Path $commonLib 'xmake.lua'))) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $commonLib) | Out-Null
    git clone --recurse-submodules https://github.com/libxse/commonlibsf.git $commonLib
} else {
    Write-Host 'CommonLibSF already present.'
}

Write-Host 'Bootstrap complete.'
