# Interactive one-run sender + log capture. Receiver must be started separately.
# Usage: .\benchmarks\run_benchmark.ps1 -File benchmark_files\text_repeat.txt -Mode huff -Trial 1
param(
  [Parameter(Mandatory=$true)][string]$File,
  [ValidateSet('raw','huff')][string]$Mode = 'raw',
  [ValidateRange(1,100)][int]$Trial = 1,
  [string]$HostIP = '127.0.0.1',
  [int]$Port = 5000
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path '.\textlink.exe')) { throw 'Run mingw32-make first.' }
if (-not (Test-Path $File)) { throw "File not found: $File" }
New-Item -ItemType Directory -Force -Path '.\benchmarks\logs' | Out-Null
$base = [IO.Path]::GetFileName($File)
$log = ".\benchmarks\logs\${base}_${Mode}_${Trial}.txt"

# Temporarily allow native stderr output without treating it as a terminating error.
$previousErrorActionPreference = $ErrorActionPreference

try {
    $ErrorActionPreference = 'Continue'

    & .\textlink.exe send $HostIP $Port $File "--$Mode" 2>&1 |
        ForEach-Object { $_.ToString() } |
        Tee-Object -FilePath $log

    $exitCode = $LASTEXITCODE
}
finally {
    $ErrorActionPreference = $previousErrorActionPreference
}

if ($exitCode -ne 0) {
    throw "Transfer failed: exit $exitCode"
}

Write-Host "Saved sender output to $log; verify receiver output and file hash separately."
