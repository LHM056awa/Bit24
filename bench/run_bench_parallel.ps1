# ============================================================================
#  Bit24 parallel benchmark — 8 threads, one per (language x -O level) combo
#
#  Same measurement model as run_bench.ps1 (--loop mode, median of N rounds),
#  except the 8 combinations (c/cpp x O0/Os/O2/O3) run CONCURRENTLY as 8
#  background jobs. Each job sequentially measures its 3 inputs, so at most
#  8 search processes run at the same time (machine has 20 logical threads,
#  so they mostly land on separate cores).
#
#  Usage:
#    powershell -NoProfile -ExecutionPolicy Bypass -File bench\run_bench_parallel.ps1
#
#  Options:
#    -Cases 10000   identical cases per process run (default 10000; the serial
#                   run_bench.ps1 uses 1000. We use 10x here because 8
#                   concurrent searches contend for CPU and each sample is
#                   noisier, so a larger per-process batch amortizes startup
#                   cost and stabilizes the median.)
#    -Rounds 3      timed rounds; median taken (default 3)
#    -SkipCompile   reuse already-built bench\*.exe
#
#  Caveat: with 8 concurrent searches, absolute ms values will differ from
#  the serial run_bench.ps1 (scheduling / thermal effects). Use this script
#  for C-vs-C++ and -O-vs--O comparisons within the same run, and the serial
#  script for absolute numbers.
# ============================================================================
[CmdletBinding()]
param(
    [int]$Cases = 10000,
    [int]$Rounds = 3,
    [switch]$SkipCompile
)

$ErrorActionPreference = "Stop"

$projRoot = Split-Path -Parent $PSScriptRoot
Set-Location $projRoot

$CC   = "gcc"
$CXX  = "g++"
$base = @("-Wall", "-Wextra")
$levels    = @("O0", "Os", "O2", "O3")
$languages = @("c", "cpp")

$inputs = @(
    @{ Label = "solved (1 2 3 4)";             Data = "1 2 3 4" },
    @{ Label = "nosol-dup (1 1 1 9)";         Data = "1 1 1 9" },
    @{ Label = "nosol-dist (-2 -7 -12 -19)";  Data = "-2 -7 -12 -19" }
)

# ---- compile the 8 combinations first (sequential, cheap) ----
if (-not $SkipCompile) {
    Write-Host ""
    Write-Host "=== Compiling 8 combinations ==="
    foreach ($lvl in $levels) {
        $flag = if ($lvl -eq "O0") { "-O0" } else { "-$lvl" }
        & $CC  $base $flag -o "bench\c_${lvl}.exe"   "Bit24.c"   2>&1 | Out-Null
        & $CXX $base $flag -o "bench\cpp_${lvl}.exe" "Bit24.cpp" 2>&1 | Out-Null
    }
    Write-Host "done."
}

# ---- one job per (language x level); each measures its 3 inputs in order ----
$jobScript = {
    param($root, $lang, $lvl, $cases, $rounds, $inputs)
    Set-Location $root

    function Build-Input {
        param([string]$data, [int]$cases)
        $lines = New-Object System.Collections.Generic.List[string]
        for ($i = 0; $i -lt $cases; $i++) { $lines.Add($data) }
        $path = [System.IO.Path]::GetTempFileName()
        [System.IO.File]::WriteAllLines($path, $lines)
        return $path
    }

    # NOTE: cannot name this "Measure" — that is an alias of the built-in
    # Measure-Command, and aliases take precedence over user functions.
    function Ms-PerCase {
        param([string]$exe, [string]$data, [int]$cases, [int]$rounds)
        $file = Build-Input -data $data -cases $cases
        try {
            $content = Get-Content $file -Raw
            $content | & $exe --loop | Out-Null    # warm-up, discarded
            $samples = @()
            for ($r = 0; $r -lt $rounds; $r++) {
                $sw = [System.Diagnostics.Stopwatch]::StartNew()
                $content | & $exe --loop | Out-Null
                $sw.Stop()
                $samples += ($sw.Elapsed.TotalMilliseconds / $cases)
            }
            $sorted = $samples | Sort-Object
            $mid = [int][math]::Floor($sorted.Count / 2.0)
            if ($sorted.Count % 2 -eq 0) {
                return (($sorted[$mid - 1] + $sorted[$mid]) / 2.0)
            }
            return $sorted[$mid]
        } finally {
            Remove-Item $file -Force -ErrorAction SilentlyContinue
        }
    }

    $exe = "bench\${lang}_${lvl}.exe"
    $rows = @()
    foreach ($inp in $inputs) {
        $ms = Ms-PerCase -exe $exe -data $inp.Data -cases $cases -rounds $rounds
        $rows += [pscustomobject]@{
            Input = $inp.Label
            Opt   = "-" + $lvl
            Lang  = $lang
            Ms    = [math]::Round($ms, 3)
        }
    }
    $rows
}

Write-Host ""
Write-Host ("=== {0} parallel measurement jobs ({1} levels x {2} languages) ===" -f
             ($levels.Count * $languages.Count), $levels.Count, $languages.Count)
$jobs = @()
foreach ($lvl in $levels) {
    foreach ($lg in $languages) {
        $jobs += Start-Job -Name ("{0}_{1}" -f $lg, $lvl) -ScriptBlock $jobScript `
                       -ArgumentList @($projRoot, $lg, $lvl, $Cases, $Rounds, $inputs)
    }
}
# Note: -Parallel flag is PS 7.2+ only; on Windows PowerShell 5.1 plain
# Wait-Job still blocks until ALL jobs finish while the jobs themselves
# run concurrently — exactly what we want.
Wait-Job $jobs | Out-Null

# collect, then clean up jobs
$flat = @()
foreach ($j in $jobs) { $flat += (Receive-Job $j) }
Stop-Job $jobs | Out-Null
Remove-Job $jobs | Out-Null

# pivot to Input x Opt rows with C/C++ side by side
$results = @()
foreach ($inp in $inputs) {
    foreach ($lvl in $levels) {
        $cRow   = $flat | Where-Object { $_.Input -eq $inp.Label -and $_.Opt -eq ("-$lvl") -and $_.Lang -eq "c" }
        $cppRow = $flat | Where-Object { $_.Input -eq $inp.Label -and $_.Opt -eq ("-$lvl") -and $_.Lang -eq "cpp" }
        $results += [pscustomobject]@{
            Input    = $inp.Label
            Opt      = "-" + $lvl
            C_ms     = $cRow.Ms
            Cpp_ms   = $cppRow.Ms
            Cpp_vs_C = if ($cRow.Ms -gt 0) { [math]::Round($cppRow.Ms / $cRow.Ms, 3) } else { 0 }
        }
    }
}

Write-Host ""
Write-Host "=== Results (lower = faster; run with 8 concurrent searches) ==="
$results | Format-Table -AutoSize

$csv = Join-Path $PSScriptRoot "results_parallel.csv"
$results | Export-Csv -Path $csv -NoTypeInformation -Encoding UTF8
Write-Host "Saved to $csv"
