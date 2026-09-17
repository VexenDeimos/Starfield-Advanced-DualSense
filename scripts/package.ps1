[CmdletBinding()]
param(
    [ValidateSet('debug','releasedbg')]
    [string]$Mode = 'releasedbg',
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

$version = '0.1.0'
$dist = Join-Path $repoRoot 'dist'
$runtimeStage = Join-Path $dist 'runtime-stage'
$sourceStage = Join-Path $dist 'source-stage'
$runtimeZip = Join-Path $dist "StarfieldDualSense-v$version.zip"
$sourceZip = Join-Path $dist "StarfieldDualSense-v$version-source.zip"

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot 'build.ps1') -Mode $Mode
}

$dll = Get-ChildItem -Path (Join-Path $repoRoot 'build') -Filter 'StarfieldDualSense.dll' -File -Recurse -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1
if (-not $dll) {
    throw 'StarfieldDualSense.dll was not found under build\. Build the plugin first.'
}

Remove-Item $runtimeStage, $sourceStage -Force -Recurse -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path (Join-Path $runtimeStage 'Data\SFSE\Plugins') | Out-Null
New-Item -ItemType Directory -Force -Path $sourceStage | Out-Null

Copy-Item $dll.FullName (Join-Path $runtimeStage 'Data\SFSE\Plugins\StarfieldDualSense.dll')
Copy-Item (Join-Path $repoRoot 'config\StarfieldDualSense.toml') (Join-Path $runtimeStage 'Data\SFSE\Plugins\StarfieldDualSense.toml')
Copy-Item (Join-Path $repoRoot 'README.md') $runtimeStage
Copy-Item (Join-Path $repoRoot 'CHANGELOG.md') $runtimeStage
Copy-Item (Join-Path $repoRoot 'LICENSE') $runtimeStage

# Corresponding source bundle: tracked project files plus the exact CommonLibSF tree
# used for the build (including its recursively cloned commonlib-shared submodule).
$tracked = git ls-files
foreach ($relative in $tracked) {
    if ($relative -like '.worktrees/*' -or $relative -like 'dist/*' -or $relative -like 'build/*') {
        continue
    }
    $source = Join-Path $repoRoot $relative
    if (Test-Path $source -PathType Leaf) {
        $destination = Join-Path $sourceStage $relative
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
        Copy-Item $source $destination
    }
}

$commonLib = Join-Path $repoRoot 'external\CommonLibSF'
if (Test-Path (Join-Path $commonLib 'xmake.lua')) {
    $thirdParty = Join-Path $sourceStage 'external\CommonLibSF'
    New-Item -ItemType Directory -Force -Path $thirdParty | Out-Null
    Get-ChildItem $commonLib -Force | Where-Object { $_.Name -ne '.git' } | ForEach-Object {
        Copy-Item $_.FullName $thirdParty -Recurse -Force
    }
    Get-ChildItem $thirdParty -Recurse -Force -Filter '.git' -ErrorAction SilentlyContinue |
        Remove-Item -Force -Recurse -ErrorAction SilentlyContinue
} else {
    throw 'CommonLibSF source is missing. Run bootstrap before creating a distributable package.'
}

Remove-Item $runtimeZip, $sourceZip -Force -ErrorAction SilentlyContinue
Compress-Archive -Path (Join-Path $runtimeStage '*') -DestinationPath $runtimeZip -CompressionLevel Optimal
Compress-Archive -Path (Join-Path $sourceStage '*') -DestinationPath $sourceZip -CompressionLevel Optimal

Remove-Item $runtimeStage, $sourceStage -Force -Recurse
Write-Host "Created $runtimeZip"
Write-Host "Created $sourceZip"
