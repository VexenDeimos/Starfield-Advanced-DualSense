[CmdletBinding()]
param(
    [ValidateSet('debug','releasedbg')]
    [string]$Mode = 'releasedbg',
    [switch]$Install
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

if (-not (Test-Path 'external\CommonLibSF\xmake.lua')) {
    throw 'CommonLibSF is missing. Run .\scripts\bootstrap.ps1 first.'
}

if (-not (Get-Command xmake -ErrorAction SilentlyContinue)) {
    throw 'XMake is not installed or not on PATH.'
}

& xmake f -m $Mode -y
if ($LASTEXITCODE -ne 0) { throw 'xmake configure failed.' }

& xmake build StarfieldDualSense
if ($LASTEXITCODE -ne 0) { throw 'xmake build StarfieldDualSense failed.' }

if ($Install) {
    & xmake install StarfieldDualSense
    if ($LASTEXITCODE -ne 0) { throw 'xmake install StarfieldDualSense failed.' }
}
