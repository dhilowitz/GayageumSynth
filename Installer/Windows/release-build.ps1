# Unprivileged Windows release pipeline for GayageumSynth, run by the
# isolated MINISFORUM\claudebuild account (dispatched by
# ReleaseBuild's build-all). It holds no signing credentials and builds no
# installer. Two steps belong to the trusted side (dsrelease runs them; by
# hand they are the commands below), and this script stops and says so when
# it reaches one of them. Ported from DecentSampler's
# Installer\Windows\release-build.ps1 (by way of Equations); keep them in step.
#
#   1. Compile x64 and Win32 and publish them to
#        C:\BuildHandoff\GayageumSynth\<v>\unsigned\  (+ BUILD_INFO.txt, TO-SIGN.json)
#   2. [trusted]  C:\TrustedSigning\sign-ds-binaries.ps1 -Product GayageumSynth -Version <v>:
#                 copies exactly the known files out of it and signs the
#                 four binaries; publishes only its receipt:
#        C:\SignedHandoff\GayageumSynth\<v>\win-binaries\<run>\RECEIPT.json
#   3. [trusted]  C:\TrustedSigning\sign-ds-installer.ps1 -Product GayageumSynth -Version <v>:
#                 builds the installer from its own reviewed Inno Setup
#                 script, signs it, and makes both zips:
#        C:\SignedHandoff\GayageumSynth\<v>\win-installer\<run>\
#          GayageumSynth-<v>-Windows.zip, GayageumSynth-<v>-Windows-No-Installer.zip, RECEIPT.json
#   4. Copy those two zips, byte for byte, into %USERPROFILE%\BuildArtifacts\GayageumSynth,
#      where staging fetches them with GayageumSynth-<v>-Windows.commit.
#
#   release-build.ps1 -Version X.Y.Z [-Fresh]
#
# Each run continues from whatever state the handoff folders are in. A
# trusted step's output only counts if its receipt says it was made from this
# build: the binaries receipt must name the SHA-256 of the current
# BUILD_INFO.txt, and the installer receipt the current binaries run.
# Anything else is waited for again. (The trusted controller does not rely
# on this: it binds the zips it uploads to the receipts it fetched from
# minisforum itself.)
#
# The commit that built the published binaries is recorded in BUILD_INFO.txt.
# When HEAD moves on, or with -Fresh, the BuildHandoff folder for this version
# is cleared and everything is rebuilt.
#
# Exit status: 0 when the zips are ready, 10 while waiting for a trusted
# signing step, anything else on failure.
param(
    [Parameter(Mandatory = $true)][string]$Version,
    [switch]$Fresh
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$ExitAwaitingSigning = 10

if ($Version -notmatch '^\d+\.\d+\.\d+$') { Write-Output "error: version must be X.Y.Z, got '$Version'"; exit 1 }

# Runs a native program. PowerShell 5.1 turns a native program's stderr into
# error records when its own output is redirected (as buildrun.cmd does),
# which $ErrorActionPreference = 'Stop' would make fatal; callers check
# $LASTEXITCODE instead.
function Invoke-Native {
    $ErrorActionPreference = 'Continue'
    $exe, $rest = $args
    & $exe @rest
}

$RepoDir = (Resolve-Path "$PSScriptRoot\..\..").Path
$Commit = (Invoke-Native git -C $RepoDir rev-parse HEAD).Trim()
$Product = 'GayageumSynth'
$Prefix = 'GayageumSynth'

# The Visual Studio solution and its build tree.
$Solutions = @(
    @{ Name = 'GayageumSynth'; BuildRoot = "$RepoDir\Builds\VisualStudio2022" }
)
# Each build folder, and its name in the handoff's unsigned tree.
$Arches = @(
    @{ Build = 'x64\Release'; Handoff = 'x64\Release'; VST3 = 'x86_64-win' },
    # Win32 builds as Release here; the handoff names it as the other products do.
    @{ Build = 'Win32\Release'; Handoff = 'Win32\ReleaseWin32'; VST3 = 'x86-win' }
)

$BuildDir = "C:\BuildHandoff\$Product\$Version"
$SignedDir = "C:\SignedHandoff\$Product\$Version"
$ArtifactsDir = "$HOME\BuildArtifacts\$Product"

$BuildInfo = "$BuildDir\BUILD_INFO.txt"
$UnsignedTree = "$BuildDir\unsigned"
$InstallerZip = "$ArtifactsDir\$Prefix-$Version-Windows.zip"
$NoInstallerZip = "$ArtifactsDir\$Prefix-$Version-Windows-No-Installer.zip"
# Which trusted run the zips came from; not fetched by staging.
$ZipsSource = "$ArtifactsDir\$Prefix-$Version-Windows.source.txt"
# The commit the zips were built from, for the release handoff's provenance check.
$CommitFile = "$ArtifactsDir\$Prefix-$Version-Windows.commit"

# Everything that ships: build-tree path (under the product's BuildRoot) ->
# handoff path (under unsigned\), the layout the trusted brokers' inventory
# expects, and the binaries the trusted side signs (a plain signtool
# signature: this product has no AAX, so no PACE).
$Products = @()
$ToSign = @()
foreach ($s in $Solutions) {
    $n = $s.Name
    foreach ($a in $Arches) {
        foreach ($p in "Standalone Plugin\$n.exe", "VST3\$n.vst3") {
            $Products += @{ Root = $s.BuildRoot; Build = "$($a.Build)\$p"; Handoff = "$($a.Handoff)\$p" }
        }
        $ToSign += @{ Path = "$($a.Handoff)\Standalone Plugin\$n.exe"; Method = 'authenticode' }
        $ToSign += @{ Path = "$($a.Handoff)\VST3\$n.vst3\Contents\$($a.VST3)\$n.vst3"; Method = 'authenticode' }
    }
}

# What to run on the trusted side.
$SignBinariesAction = "run:  C:\TrustedSigning\sign-ds-binaries.ps1 -Product $Product -Version $Version   (dsrelease does this for you)"
$SignInstallerAction = "run:  C:\TrustedSigning\sign-ds-installer.ps1 -Product $Product -Version $Version   (dsrelease does this for you)"

# Finished zips stay only while this run can vouch for them: any run that
# stops before step 4 confirms them removes them, so staging can never pick
# up zips from an older build.
function Remove-Zips {
    Remove-Item -LiteralPath $InstallerZip, $NoInstallerZip, $ZipsSource, $CommitFile, "$InstallerZip.partial", "$NoInstallerZip.partial" -Force -ErrorAction SilentlyContinue
}
trap { Remove-Zips; break }

function Fail([string]$message) {
    Write-Output "error: $message"
    Remove-Zips
    exit 1
}

# The GATE: line is a fixed identifier (win-binaries or win-installer) that
# build-all records in its structured state for the trusted controller.
function Await-Signing([string]$gate, [string]$action, [string]$reason) {
    Write-Output ''
    Write-Output $reason
    Write-Output "GATE: $gate"
    Write-Output "ACTION: As dhilo on minisforum, $action"
    Remove-Zips
    exit $ExitAwaitingSigning
}

# robocopy a file or folder, keeping hidden/system files, attributes and
# empty folders.
function Copy-Product([string]$from, [string]$to) {
    if (Test-Path -LiteralPath $from -PathType Leaf) {
        $null = Invoke-Native robocopy (Split-Path $from) (Split-Path $to) (Split-Path $from -Leaf) /COPY:DAT /R:1 /W:1 /NP /NFL /NDL /NJH /NJS
    } else {
        $null = Invoke-Native robocopy $from $to /E /COPY:DAT /DCOPY:DAT /R:1 /W:1 /NP /NFL /NDL /NJH /NJS
    }
    if ($LASTEXITCODE -ge 8) { Fail "copying $from to $to failed (robocopy exit $LASTEXITCODE)" }
}

function Get-Sha256([string]$path) { (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash }

# --- 1. Compile and publish the unsigned binaries -------------------------------
if (Test-Path -LiteralPath $BuildInfo) {
    $builtCommit = ((Get-Content -LiteralPath $BuildInfo) -match '^commit=' -replace '^commit=', '') | Select-Object -First 1
    if ($Fresh) {
        Write-Output "-Fresh: discarding the published $Version build ($builtCommit)."
        Remove-Item -LiteralPath $BuildDir -Recurse -Force
    } elseif ($builtCommit -ne $Commit) {
        Write-Output "The published $Version build is from $builtCommit; HEAD is now $Commit. Rebuilding."
        Remove-Item -LiteralPath $BuildDir -Recurse -Force
    }
} elseif (Test-Path -LiteralPath $BuildDir) {
    Write-Output "$BuildDir has no BUILD_INFO.txt, so its commit is unknown. Rebuilding."
    Remove-Item -LiteralPath $BuildDir -Recurse -Force
}

if (-not (Test-Path -LiteralPath $BuildInfo)) {
    Write-Output "Building the unsigned $Version binaries from $Commit..."
    $MSBuildX64 = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe'
    $MSBuildX86 = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'

    foreach ($sln in $Solutions) {
        $solution = "$($sln.BuildRoot)\$($sln.Name).sln"
        Write-Output "Building $($sln.Name) x64 (Release)..."
        Invoke-Native $MSBuildX64 $solution /p:Configuration=Release /p:Platform=x64 /m /nr:false /nologo /v:minimal /t:Rebuild
        if ($LASTEXITCODE -ne 0) { Fail "$($sln.Name) x64 build failed" }
        Write-Output "Building $($sln.Name) Win32 (Release)..."
        Invoke-Native $MSBuildX86 $solution /p:Configuration=Release /p:Platform=Win32 /m /nr:false /nologo /v:minimal /t:Rebuild
        if ($LASTEXITCODE -ne 0) { Fail "$($sln.Name) Win32 build failed" }
    }

    foreach ($p in $Products) {
        if (-not (Test-Path -LiteralPath "$($p.Root)\$($p.Build)")) { Fail "the build did not produce $($p.Build)" }
    }
    # The installer's name and version come from the compiled version, so a
    # release branch whose version wasn't bumped must stop here.
    foreach ($p in $Products | Where-Object { $_.Build -like '*.exe' }) {
        $v = (Get-Item -LiteralPath "$($p.Root)\$($p.Build)").VersionInfo
        $built = '{0}.{1}.{2}' -f $v.FileMajorPart, $v.FileMinorPart, $v.FileBuildPart
        if ($built -ne $Version) { Fail "$($p.Build) is version $built, not $Version - is the release branch's version bumped?" }
    }

    # Assemble beside the final location and rename into place, so the
    # signing step can never pick up a half-copied tree.
    $partial = "$BuildDir.partial"
    Remove-Item -LiteralPath $partial -Recurse -Force -ErrorAction SilentlyContinue
    foreach ($p in $Products) { Copy-Product "$($p.Root)\$($p.Build)" "$partial\unsigned\$($p.Handoff)" }
    foreach ($t in $ToSign) {
        $status = (Get-AuthenticodeSignature -LiteralPath "$partial\unsigned\$($t.Path)").Status
        if ($status -ne 'NotSigned') { Fail "$($t.Path) should be unsigned, but its signature status is $status" }
    }

    $entries = foreach ($t in $ToSign) {
        $f = Get-Item -LiteralPath "$partial\unsigned\$($t.Path)"
        [ordered]@{ path = $t.Path; method = $t.Method; size = $f.Length; sha256 = (Get-Sha256 $f.FullName) }
    }
    [ordered]@{
        version = $Version; commit = $Commit
        unsigned_root = 'unsigned'
        files = @($entries)
    } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath "$partial\TO-SIGN.json" -Encoding UTF8

    # Symbols stay on this account, next to the finished zips.
    $symbols = "$ArtifactsDir\Symbols\$Version"
    Remove-Item -LiteralPath $symbols -Recurse -Force -ErrorAction SilentlyContinue
    foreach ($p in $Products) {
        $name = [IO.Path]::GetFileNameWithoutExtension($p.Build)
        Copy-Product "$($p.Root)\$(Split-Path $p.Build)\$name.pdb" "$symbols\$(Split-Path $p.Handoff)\$name.pdb"
    }

    @(
        "version=$Version"
        "commit=$Commit"
        "branch=$((Invoke-Native git -C $RepoDir rev-parse --abbrev-ref HEAD).Trim())"
        "built=$((Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ'))"
        "host=$env:COMPUTERNAME"
    ) | Set-Content -LiteralPath "$partial\BUILD_INFO.txt" -Encoding ASCII
    Rename-Item -LiteralPath $partial -NewName (Split-Path $BuildDir -Leaf)
    Write-Output "Published $UnsignedTree ($($ToSign.Count) files to sign, listed in TO-SIGN.json)"
}

function Get-CurrentRun([string]$gate) {
    $pointer = "$SignedDir\$gate.current"
    if (-not (Test-Path -LiteralPath $pointer -PathType Leaf)) { return $null }
    $run = (Get-Content -LiteralPath $pointer -Raw).Trim()
    if ($run -cnotmatch '^[0-9a-f]{32}$' -or -not (Test-Path -LiteralPath "$SignedDir\$gate\$run\RECEIPT.json" -PathType Leaf)) { return $null }
    return $run
}
function Get-Receipt([string]$gate, [string]$run) {
    Get-Content -LiteralPath "$SignedDir\$gate\$run\RECEIPT.json" -Raw | ConvertFrom-Json
}

# --- 2. Signed binaries (trusted) ---------------------------------------------------
$binariesRun = Get-CurrentRun 'win-binaries'
if (-not $binariesRun) {
    Await-Signing win-binaries $SignBinariesAction "Waiting for the binaries to be signed: no win-binaries receipt for $Version yet."
}
if ((Get-Receipt 'win-binaries' $binariesRun).inputs.build_info_sha256 -ne (Get-Sha256 $BuildInfo).ToLowerInvariant()) {
    Await-Signing win-binaries $SignBinariesAction "The newest signed binaries (run $binariesRun) were made from an earlier build, not the one now in $BuildDir."
}
Write-Output "Signed binaries: run $binariesRun, made from this build."

# --- 3. Installer (trusted) --------------------------------------------------------
$installerRun = Get-CurrentRun 'win-installer'
if (-not $installerRun) {
    Await-Signing win-installer $SignInstallerAction "Waiting for the trusted side to build and sign the installer: no win-installer receipt for $Version yet."
}
$installerReceipt = Get-Receipt 'win-installer' $installerRun
if ($installerReceipt.inputs.binaries_run_id -ne $binariesRun) {
    Await-Signing win-installer $SignInstallerAction "The newest installer (run $installerRun) was built from different signed binaries than run $binariesRun."
}
Write-Output "Installer: run $installerRun, built from signed binaries run $binariesRun."

# --- 4. Zips for staging -----------------------------------------------------------
# Copied only when missing or different, so a resume leaves them as they are.
New-Item -ItemType Directory -Force $ArtifactsDir | Out-Null
$zipsFrom = "win_installer_run=$installerRun"
if (-not (Test-Path -LiteralPath $ZipsSource) -or (Get-Content -LiteralPath $ZipsSource -Raw).Trim() -ne $zipsFrom) { Remove-Zips }
foreach ($zip in $InstallerZip, $NoInstallerZip) {
    $name = Split-Path $zip -Leaf
    $want = [string]$installerReceipt.outputs.$name.sha256
    if ($want -cnotmatch '^[0-9a-f]{64}$') { Fail "the win-installer receipt has no hash for $name" }
    if ((Test-Path -LiteralPath $zip) -and (Get-Sha256 $zip).ToLowerInvariant() -eq $want) {
        Write-Output "  $name already current"
        continue
    }
    Copy-Item -LiteralPath "$SignedDir\win-installer\$installerRun\$name" "$zip.partial" -Force
    if ((Get-Sha256 "$zip.partial").ToLowerInvariant() -ne $want) { Fail "$name does not match its receipt" }
    Move-Item -LiteralPath "$zip.partial" $zip -Force
    Write-Output "  copied $name"
}
$zipsFrom | Set-Content -LiteralPath $ZipsSource -Encoding ASCII

# Provenance: the commit the published binaries were built from. Rewritten
# only when it differs, so a resume changes nothing.
$builtFrom = ((Get-Content -LiteralPath $BuildInfo) -match '^commit=' -replace '^commit=', '') | Select-Object -First 1
if (-not (Test-Path -LiteralPath $CommitFile) -or (Get-Content -LiteralPath $CommitFile | Select-Object -First 1) -ne $builtFrom) {
    $builtFrom | Set-Content -LiteralPath "$CommitFile.partial" -Encoding ASCII
    Move-Item -LiteralPath "$CommitFile.partial" $CommitFile -Force
}

Write-Output ''
Write-Output "Windows $Version is ready for staging (built from $Commit)."
exit 0
