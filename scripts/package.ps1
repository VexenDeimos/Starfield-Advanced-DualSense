[CmdletBinding()]
param(
    [switch]$SkipBuild,
    [switch]$SkipSource
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $repoRoot
$version = '0.7.1'
$dist = Join-Path $repoRoot 'dist'
$pluginDir = Join-Path $repoRoot 'build\windows\x64\release'
$dll = Join-Path $pluginDir 'StarfieldDualSense.dll'
$toml = Join-Path $repoRoot 'config\StarfieldDualSense.toml'

if (-not (Get-Command xmake -ErrorAction SilentlyContinue)) {
    throw 'XMake is required to package an official release.'
}
if (-not $SkipBuild) {
    & xmake f -m release -y
    if ($LASTEXITCODE -ne 0) { throw 'xmake configure failed.' }
    & xmake build StarfieldDualSense
    if ($LASTEXITCODE -ne 0) { throw 'xmake build failed.' }
}

if (-not (Test-Path -LiteralPath $dll -PathType Leaf)) {
    throw 'Release DLL is missing. Build in release mode first.'
}
if (-not (Test-Path -LiteralPath $toml -PathType Leaf)) {
    throw 'Default TOML is missing.'
}
$dllVersion = (Get-Item -LiteralPath $dll).VersionInfo.FileVersion
if (-not $dllVersion -or -not $dllVersion.StartsWith('0.7.1')) {
    throw "DLL FileVersion is not 0.7.1: $dllVersion. Refusing to package a stale or development DLL."
}
if ((Get-Content -LiteralPath $toml -TotalCount 1) -ne '# Starfield DualSense v0.7.1') {
    throw 'Default config header is not v0.7.1.'
}

New-Item -ItemType Directory -Path $dist -Force | Out-Null
$runtimeZip = Join-Path $dist "StarfieldDualSense_v${version}_Nexus.zip"
$sourceZip = Join-Path $dist "StarfieldDualSense_v${version}_Source.zip"
$manifest = Join-Path $dist "StarfieldDualSense_v${version}_SHA256.txt"
$targets = @($runtimeZip, $manifest)
if (-not $SkipSource) { $targets += $sourceZip }
foreach ($target in $targets) {
    if (Test-Path -LiteralPath $target) {
        throw "Output already exists (will not overwrite): $target"
    }
}

$stage = Join-Path $dist ('.release-package-stage-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + $PID)
$runtimeRoot = Join-Path $stage 'runtime'
$sourceRoot = Join-Path $stage 'source'
try {
    $plugins = Join-Path $runtimeRoot 'SFSE\Plugins'
    New-Item -ItemType Directory -Path $plugins -Force | Out-Null
    Copy-Item -LiteralPath $dll -Destination (Join-Path $plugins 'StarfieldDualSense.dll')
    Copy-Item -LiteralPath $toml -Destination (Join-Path $plugins 'StarfieldDualSense.toml')
    foreach ($rel in @('README.md','CHANGELOG.md','LICENSE')) {
        Copy-Item -LiteralPath (Join-Path $repoRoot $rel) -Destination (Join-Path $runtimeRoot $rel)
    }

    if (-not $SkipSource) {
        if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
            throw 'Git is required for the curated source archive. Use -SkipSource only if needed.'
        }
        $relativePaths = @(git ls-files --cached --others --exclude-standard -- src include config docs tests scripts xmake.lua README.md CHANGELOG.md LICENSE .gitignore)
        if ($LASTEXITCODE -ne 0) { throw 'git ls-files failed.' }
        if ($relativePaths.Count -lt 30) { throw 'Source file list unexpectedly small; refusing incomplete source ZIP.' }
        if ($relativePaths -notcontains 'include/StarfieldDualSense/TouchpadBindings.h') {
            throw 'The untracked TouchpadBindings.h is missing from the source selection.'
        }
        foreach ($rel in $relativePaths) {
            if ([string]::IsNullOrWhiteSpace($rel)) { continue }
            if ($rel -match '(^|/)(\.git|build|dist|external)(/|$)' -or $rel -match '(\.bak$|\.zip$)') {
                throw "Unexpected unclean source archive path: $rel"
            }
            $source = Join-Path $repoRoot ($rel -replace '/', '\')
            if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
                throw "Git-referenced source file missing: $rel"
            }
            $destination = Join-Path $sourceRoot ($rel -replace '/', '\')
            New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
            Copy-Item -LiteralPath $source -Destination $destination
        }
    }

    Compress-Archive -Path (Join-Path $runtimeRoot '*') -DestinationPath $runtimeZip -CompressionLevel Optimal
    if (-not $SkipSource) {
        Compress-Archive -Path (Join-Path $sourceRoot '*') -DestinationPath $sourceZip -CompressionLevel Optimal
    }

    $hashes = @()
    foreach ($item in @($dll, $toml, $runtimeZip, $sourceZip)) {
        if (Test-Path -LiteralPath $item -PathType Leaf) {
            $hashes += ('{0}  {1}' -f (Get-FileHash -LiteralPath $item -Algorithm SHA256).Hash, $item)
        }
    }
    [IO.File]::WriteAllLines($manifest, $hashes, (New-Object Text.UTF8Encoding($false)))
    Write-Host "PASS: Nexus-ready archive: $runtimeZip" -ForegroundColor Green
    if (-not $SkipSource) { Write-Host "PASS: curated source archive: $sourceZip" -ForegroundColor Green }
    Write-Host "SHA256 manifest: $manifest"
    Write-Host 'No game files installed. No Git commit/tag/push performed.'
}
finally {
    if (Test-Path -LiteralPath $stage) {
        Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue
    }
}
