$ErrorActionPreference = "Stop"

# v0.3.06 pinned decoder inputs.  These are downloaded directly instead of
# relying on Xmake package installation, which proved unreliable with the
# CommonLibSF target/package lifecycle on the Windows build host.
$StbCommit = "28d546d5eb77d4585506a20480f4de2e706dff4c"
$StbBlobSha1 = "3e5c2504c08f76b9d04e4adfbce80818631e835e"
$Ww2OggCommit = "14ed9b0dd62e815a38702b5f03c57006cbe2501b"
$Ww2OggCodebookBlobSha1 = "a405d061b50cd793e17ad99b1f9f809ee372812a"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$StbDir = Join-Path $ProjectRoot "external\stb"
$Ww2OggDir = Join-Path $ProjectRoot "external\ww2ogg"
$CodebookIncludeDir = Join-Path $Ww2OggDir "include\StarfieldDualSense"

$StbPath = Join-Path $StbDir "stb_vorbis.c"
$CodebookBinPath = Join-Path $Ww2OggDir "packed_codebooks_aoTuV_603.bin"
$CodebookIncPath = Join-Path $CodebookIncludeDir "packed_codebooks_aoTuV_603.inc"

New-Item -ItemType Directory -Force -Path $StbDir | Out-Null
New-Item -ItemType Directory -Force -Path $CodebookIncludeDir | Out-Null

# Windows PowerShell 5.1 can otherwise negotiate an older TLS version on some hosts.
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

function Get-GitBlobSha1 {
    param([Parameter(Mandatory = $true)][string]$Path)

    $data = [IO.File]::ReadAllBytes($Path)
    $header = [Text.Encoding]::ASCII.GetBytes("blob $($data.Length)`0")
    $blob = New-Object byte[] ($header.Length + $data.Length)
    [Buffer]::BlockCopy($header, 0, $blob, 0, $header.Length)
    [Buffer]::BlockCopy($data, 0, $blob, $header.Length, $data.Length)

    $sha1 = [Security.Cryptography.SHA1]::Create()
    try {
        return (($sha1.ComputeHash($blob) | ForEach-Object { $_.ToString("x2") }) -join "")
    }
    finally {
        $sha1.Dispose()
    }
}

function Get-PinnedFile {
    param(
        [Parameter(Mandatory = $true)][string]$Url,
        [Parameter(Mandatory = $true)][string]$Destination,
        [Parameter(Mandatory = $true)][string]$ExpectedGitBlobSha1
    )

    if (Test-Path -LiteralPath $Destination) {
        $existingSha1 = Get-GitBlobSha1 -Path $Destination
        if ($existingSha1 -eq $ExpectedGitBlobSha1) {
            Write-Host "Pinned dependency already valid: $Destination"
            return
        }
        Remove-Item -LiteralPath $Destination -Force
    }

    Write-Host "Downloading $Url"
    Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $Destination

    $actualSha1 = Get-GitBlobSha1 -Path $Destination
    if ($actualSha1 -ne $ExpectedGitBlobSha1) {
        Remove-Item -LiteralPath $Destination -Force -ErrorAction SilentlyContinue
        throw "Pinned dependency hash mismatch for $Destination. Expected $ExpectedGitBlobSha1, got $actualSha1."
    }
}

$StbUrl = "https://raw.githubusercontent.com/nothings/stb/$StbCommit/stb_vorbis.c"
$CodebookUrl = "https://raw.githubusercontent.com/hcs64/ww2ogg/$Ww2OggCommit/packed_codebooks_aoTuV_603.bin"

Get-PinnedFile -Url $StbUrl -Destination $StbPath -ExpectedGitBlobSha1 $StbBlobSha1
Get-PinnedFile -Url $CodebookUrl -Destination $CodebookBinPath -ExpectedGitBlobSha1 $Ww2OggCodebookBlobSha1

Write-Host "Generating $CodebookIncPath"
$bytes = [IO.File]::ReadAllBytes($CodebookBinPath)
$builder = New-Object Text.StringBuilder
for ($i = 0; $i -lt $bytes.Length; $i++) {
    [void]$builder.AppendFormat("0x{0:X2}", $bytes[$i])
    if ($i -ne ($bytes.Length - 1)) {
        [void]$builder.Append(",")
    }
    if ((($i + 1) % 16) -eq 0) {
        [void]$builder.AppendLine()
    }
    else {
        [void]$builder.Append(" ")
    }
}
if (($bytes.Length % 16) -ne 0) {
    [void]$builder.AppendLine()
}

$utf8NoBom = New-Object Text.UTF8Encoding($false)
[IO.File]::WriteAllText($CodebookIncPath, $builder.ToString(), $utf8NoBom)

Write-Host "Vorbis decoder dependencies are ready."
Write-Host "  stb:      $StbPath"
Write-Host "  codebook: $CodebookIncPath"
