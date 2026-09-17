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

xmake f -m $Mode -y
xmake build StarfieldDualSense
if ($Install) {
    xmake install StarfieldDualSense
}
