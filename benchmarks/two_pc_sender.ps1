
param(
    [string]$HostIP = '192.168.0.140',
    [int]$Port = 5000,
    [int]$DelayMs = 1500
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path '.\textlink.exe')) {
    throw 'textlink.exe not found. Run mingw32-make first.'
}

$files = @(
    'benchmark_real\real_chinese.txt',
    'benchmark_real\real_english.txt',
    'benchmark_real\audio_music_20s.wav',
    'benchmark_real\audio_noise.wav'
)

$modes = @('raw', 'huff')
$trials = 1..5

$logDir = '.\benchmarks\logs_two_pc_sender'

if (Test-Path $logDir) {
    $existing = @(Get-ChildItem $logDir -File)
    if ($existing.Count -gt 0) {
        throw "Sender log directory is not empty: $logDir"
    }
}

New-Item -ItemType Directory -Force -Path $logDir | Out-Null

foreach ($file in $files) {
    if (-not (Test-Path $file)) {
        throw "Missing benchmark file: $file"
    }
}

$index = 0
$total = $files.Count * $modes.Count * $trials.Count

Write-Host "Two-PC sender benchmark"
Write-Host "Receiver: ${HostIP}:${Port}"
Write-Host "Total transfers: $total"
Write-Host "Log directory: $logDir"
Write-Host ""

foreach ($file in $files) {
    foreach ($mode in $modes) {
        foreach ($trial in $trials) {

            $index++
            $number = '{0:D2}' -f $index
            $base = [IO.Path]::GetFileName($file)

            $logName = "${number}_${base}_${mode}_${trial}.txt"
            $logPath = Join-Path $logDir $logName

            Write-Host "[$index/$total] $base mode=$mode trial=$trial"

            $oldPreference = $ErrorActionPreference

            try {
                $ErrorActionPreference = 'Continue'

                & .\textlink.exe send $HostIP $Port $file "--$mode" 2>&1 |
                    ForEach-Object { $_.ToString() } |
                    Tee-Object -FilePath $logPath

                $exitCode = $LASTEXITCODE
            }
            finally {
                $ErrorActionPreference = $oldPreference
            }

            if ($exitCode -ne 0) {
                Write-Host "FAIL: transfer $index, exit=$exitCode"
                Write-Host "See log: $logPath"
                exit 1
            }

            if (-not (Select-String -Path $logPath -Pattern 'STATS role=send' -Quiet)) {
                Write-Host "FAIL: sender STATS not found: $logPath"
                exit 1
            }

            Write-Host "[$index/$total] PASS"
            Write-Host ""

            Start-Sleep -Milliseconds $DelayMs
        }
    }
}

Write-Host "All $total sender transfers completed."
