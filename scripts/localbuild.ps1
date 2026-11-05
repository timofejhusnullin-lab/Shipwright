<#
  Local incremental build for Ship of Harkinian on Windows.

  dev.bat      -> build\dev      Release without LTCG. Only changed files are recompiled; then the game starts.
  release.bat  -> build\release  Full Release with LTCG (final, fastest exe). Also incremental.

  Never cleans. Ninja tracks every header dependency, so only files affected by your change are rebuilt.
  Both build folders share one vcpkg (build\vcpkg), so libraries are built only once.

  Options:
    -NoRun        don't start the game after a dev build
    -Jobs N       override the automatic number of parallel compile jobs
    -Reconfigure  re-run CMake configure (normally not needed: Ninja does it automatically when CMake files change)
#>
param(
    [ValidateSet("dev", "release")] [string]$Mode = "dev",
    [switch]$NoRun,
    [int]$Jobs = 0,
    [switch]$Reconfigure
)

$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildDir = Join-Path $repo "build\$Mode"
$otherDir = Join-Path $repo ("build\" + $(if ($Mode -eq "dev") { "release" } else { "dev" }))
$vcpkgDir = Join-Path $repo "build\vcpkg"
$modsDir = Join-Path $repo "mods"

function Fail($msg) {
    Write-Host ""
    Write-Host "ERROR: $msg" -ForegroundColor Red
    exit 1
}

# ---------- 1. Visual Studio C++ environment (cl.exe, link.exe, and VS's bundled cmake/ninja) ----------
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        Fail "Visual Studio 2022 (or Build Tools 2022) not found. Install it with the 'Desktop development with C++' workload."
    }
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) {
        Fail "Visual Studio found, but without C++ tools. In Visual Studio Installer add 'Desktop development with C++'."
    }
    $vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
    Write-Host "Loading MSVC environment from $vsPath"
    # A temp .cmd avoids cmd.exe's quote stripping on paths with spaces ("Program Files")
    $envScript = Join-Path $env:TEMP "soh-vcvars-$PID.cmd"
    Set-Content -Path $envScript -Encoding ASCII -Value "@call `"$vcvars`" >nul 2>&1`r`n@set"
    cmd /c $envScript | ForEach-Object {
        if ($_ -match "^([^=]+)=(.*)$") { [Environment]::SetEnvironmentVariable($matches[1], $matches[2]) }
    }
    Remove-Item $envScript -ErrorAction SilentlyContinue
    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        Fail "Could not load the MSVC environment from $vcvars"
    }
}

foreach ($tool in @("cmake", "ninja", "git")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        Fail "'$tool' not found. cmake/ninja come with the 'C++ CMake tools for Windows' component of Visual Studio; git: https://git-scm.com"
    }
}

# ---------- 2. Submodules (only fetched if missing, never reset) ----------
if (-not (Test-Path (Join-Path $repo "libultraship\CMakeLists.txt")) -or -not (Test-Path (Join-Path $repo "torch\CMakeLists.txt"))) {
    Write-Host "Fetching submodules (first time only)..."
    git -C $repo submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) { Fail "git submodule update failed" }
}

# ---------- 3. Parallel jobs from CPU and RAM ----------
$cores = [Environment]::ProcessorCount
$ramGB = [math]::Floor((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB)
if ($Jobs -le 0) {
    # MSVC uses up to ~2 GB per compiler process on big files; more jobs than RAM allows only causes swapping.
    $Jobs = [math]::Max(1, [math]::Min($cores, [math]::Floor($ramGB / 2)))
}
Write-Host "CPU threads: $cores, RAM: $ramGB GB -> $Jobs parallel jobs"

# ---------- 4. Configure (first time only) ----------
$lto = $(if ($Mode -eq "dev") { "ON" } else { "OFF" })
if ($Reconfigure -or -not (Test-Path (Join-Path $buildDir "build.ninja"))) {
    Write-Host "Configuring $Mode build (first time: also downloads and builds libraries via vcpkg, this takes a while)..."
    cmake -S $repo -B $buildDir -G Ninja `
        -DCMAKE_BUILD_TYPE=Release `
        "-DSOH_DISABLE_LTO=$lto" `
        "-DVCPKG_ROOT=$vcpkgDir"
    if ($LASTEXITCODE -ne 0) { Fail "CMake configure failed (see the output above)." }
}

# ---------- 5. Incremental build ----------
$timer = [Diagnostics.Stopwatch]::StartNew()
cmake --build $buildDir --target soh GenerateSohOtr --parallel $Jobs
if ($LASTEXITCODE -ne 0) { Fail "Build failed (see the first 'error' line above)." }
$timer.Stop()
Write-Host ("Build finished in {0:mm\:ss}" -f $timer.Elapsed) -ForegroundColor Green

# ---------- 6. Game folder: shared mods\ and oot.o2r ----------
$gameDir = Join-Path $buildDir "soh"
$exe = Join-Path $gameDir "soh.exe"
if (-not (Test-Path $exe)) { Fail "soh.exe not found in $gameDir" }

if (-not (Test-Path $modsDir)) { New-Item -ItemType Directory -Path $modsDir | Out-Null }
$gameMods = Join-Path $gameDir "mods"
if (-not (Test-Path $gameMods)) {
    # Junction: dev and release builds see the same mods folder (repo\mods)
    New-Item -ItemType Junction -Path $gameMods -Target $modsDir | Out-Null
}

$ootArchive = Join-Path $gameDir "oot.o2r"
$otherArchive = Join-Path $otherDir "soh\oot.o2r"
if (-not (Test-Path $ootArchive) -and (Test-Path $otherArchive)) {
    Copy-Item $otherArchive $ootArchive
}
if (-not (Test-Path $ootArchive)) {
    Write-Host "No oot.o2r yet: on first launch the game will ask for your ROM and extract it (once)." -ForegroundColor Yellow
}

Write-Host "Game: $exe"
Write-Host "Mods: $modsDir"

if ($Mode -eq "dev" -and -not $NoRun) {
    Start-Process -FilePath $exe -WorkingDirectory $gameDir
}
