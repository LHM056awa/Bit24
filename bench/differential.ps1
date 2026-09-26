param(
    [int]$RandomCount = 1500
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

# rebuild fresh
gcc -std=c99 -Wall -Wextra -O2 Bit24.c -o t_c.exe  2>&1 | Out-Null
g++ -std=c++11 -Wall -Wextra -O2 Bit24.cpp -o t_cpp.exe 2>&1 | Out-Null

# generate random + boundary cases
$lines = @()
for ($i = 0; $i -lt $RandomCount; $i++) {
    $lines += ("{0} {1} {2} {3}" -f
        (Get-Random -Minimum -2000 -Maximum 2000),
        (Get-Random -Minimum -2000 -Maximum 2000),
        (Get-Random -Minimum -2000 -Maximum 2000),
        (Get-Random -Minimum -2000 -Maximum 2000))
}
foreach ($b in @(
    "2147483647 2147483647 2147483647 2147483647",
    "-2147483648 -2147483648 -2147483648 -2147483648",
    "2147483647 -2147483648 0 0",
    "12 12 12 12","0 0 0 0","1000000 1000000 2 24",
    "-12 -12 12 12","64 64 0 0","63 63 0 0","1 1 1 9")) {
    $lines += $b
}
# default Set-Content writes one line per case with a trailing newline.
# (Do NOT use -NoNewline: it concatenates all cases into one line with no
#  separator, so e.g. "…3 4" + "5 6…" glues into "45", corrupting the input.)
$lines | Set-Content t_cases.txt

Write-Host "t_cases.txt lines: $((Get-Content t_cases.txt).Count)"
if ((Get-Content t_cases.txt).Count -ne $lines.Count) {
    Write-Host "input file line count mismatch, aborting"; exit 1
}

# run both in --loop, redirect through cmd (PS has no <)
$cOut   = & cmd /c "t_c.exe --loop < t_cases.txt"
$cppOut = & cmd /c "t_cpp.exe --loop < t_cases.txt"

$diff = Compare-Object $cOut $cppOut
Write-Host ("C   : {0} result lines" -f $cOut.Count)
Write-Host ("C++ : {0} result lines" -f $cppOut.Count)
if ($diff) {
    Write-Host "=== MISMATCH FOUND ==="
    $diff | Select-Object -First 20
    exit 1
} else {
    Write-Host "=== ALL $($cOut.Count) LINES IDENTICAL ==="
    Write-Host "sample (first 3):"
    $cOut | Select-Object -First 3
}
