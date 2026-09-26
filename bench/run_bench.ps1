# ============================================================================
#  Bit24 benchmark
#
#  Compares C (Bit24.c) vs C++ (Bit24.cpp) runtime across four -O levels,
#  isolating the -O level as the single variable (both always carry
#  -Wall -Wextra; we deliberately drop -static/-s/gc-sections since those
#  affect binary size/linking, not runtime).
#
#  Timing model: each binary is run ONCE in --loop mode over N identical
#  cases, so the ~3-10 ms process-startup cost is amortized to ~0.03 ms per
#  case and the measurement reflects pure search computation. The median of
#  several timed rounds is taken to suppress noise.
#
#  Usage (from project root):
#    powershell -NoProfile -ExecutionPolicy Bypass -File bench\run_bench.ps1
#
#  Options:
#    -Cases 1000    identical cases per process run (default 1000)
#    -Rounds 3      timed rounds; median taken (default 3)
#    -SkipCompile   reuse already-built bench\*.exe
# ============================================================================
[CmdletBinding()]
param(
    [int]$Cases = 1000,
    [int]$Rounds = 3,
    [switch]$SkipCompile
)

$ErrorActionPreference = "Stop"

$projRoot = Split-Path -Parent $PSScriptRoot
Set-Location $projRoot

$CC   = "gcc"
$CXX  = "g++"
$base = @("-Wall", "-Wextra")
$levels = @("O0", "Os", "O2", "O3")

# Benchmark inputs:
#   solved        : hits early -> measures fast-path
#   nosol-dup     : no solution, repeated digit (fewer distinct permutations)
#   nosol-dist    : no solution, 4 distinct negatives -> bitwise ops all
#                   rejected by is_integer() (x>=0 guard), pure full search
$inputs = @(
    @{ Label = "solved (1 2 3 4)";             Data = "1 2 3 4" },
    @{ Label = "nosol-dup (1 1 1 9)";         Data = "1 1 1 9" },
    @{ Label = "nosol-dist (-2 -7 -12 -19)";  Data = "-2 -7 -12 -19" }
)

if (-not $SkipCompile) {
    Write-Host ""
    Write-Host "=== Compiling (single variable = -O level) ==="
    foreach ($lvl in $levels) {
        $flag = if ($lvl -eq "O0") { "-O0" } else { "-$lvl" }
        & $CC  $base $flag -o "bench\c_${lvl}.exe"   "Bit24.c"   2>&1 | Out-Null
        & $CXX $base $flag -o "bench\cpp_${lvl}.exe" "Bit24.cpp" 2>&1 | Out-Null
        Write-Host ("  -" + $lvl + "  C:" + (Test-Path "bench\c_${lvl}.exe") + "  C++:" + (Test-Path "bench\cpp_${lvl}.exe"))
    }
}

function Build-InputFile {
    param([string]$data, [int]$cases)
    $lines = New-Object System.Collections.Generic.List[string]
    for ($i = 0; $i -lt $cases; $i++) { $lines.Add($data) }
    $path = [System.IO.Path]::GetTempFileName()
    [System.IO.File]::WriteAllLines($path, $lines)
    return $path
}

# Returns the median ms-per-single-case across $rounds timed runs.
function Measure-AvgMsPerCase {
    param([string]$exe, [string]$data, [int]$cases, [int]$rounds)
    $file = Build-InputFile -data $data -cases $cases
    try {
        $content = Get-Content $file -Raw
        # warm-up (page-cache / pipeline), discarded
        $content | & $exe --loop | Out-Null
        $samples = @()
        for ($r = 0; $r -lt $rounds; $r++) {
            $sw = [System.Diagnostics.Stopwatch]::StartNew()
            $content | & $exe --loop | Out-Null
            $sw.Stop()
            $samples += ($sw.Elapsed.TotalMilliseconds / $cases)
        }
        $sorted = $samples | Sort-Object
        $mid = [int]([Math]::Floor($sorted.Count / 2.0))
        if ($sorted.Count % 2 -eq 0) {
            return (($sorted[$mid - 1] + $sorted[$mid]) / 2.0)
        } else {
            return $sorted[$mid]
        }
    } finally {
        Remove-Item $file -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ""
Write-Host "=== Timing ($Cases cases/run, median of $Rounds rounds, ms per single search) ==="
$results = @()
foreach ($inp in $inputs) {
    foreach ($lvl in $levels) {
        $cMs   = Measure-AvgMsPerCase -exe "bench\c_${lvl}.exe"   -data $inp.Data -cases $Cases -rounds $Rounds
        $cppMs = Measure-AvgMsPerCase -exe "bench\cpp_${lvl}.exe" -data $inp.Data -cases $Cases -rounds $Rounds
        $results += [pscustomobject]@{
            Input  = $inp.Label
            Opt    = "-" + $lvl
            C_ms   = [math]::Round($cMs, 3)
            Cpp_ms = [math]::Round($cppMs, 3)
            Cpp_vs_C = if ($cMs -gt 0) { [math]::Round($cppMs / $cMs, 3) } else { 0 }
        }
    }
}

Write-Host ""
Write-Host "=== Results (lower = faster; Cpp_vs_C = C++ time / C time) ==="
$results | Format-Table -AutoSize

$csv = Join-Path $PSScriptRoot "results.csv"
$results | Export-Csv -Path $csv -NoTypeInformation -Encoding UTF8
Write-Host "Saved to $csv"
