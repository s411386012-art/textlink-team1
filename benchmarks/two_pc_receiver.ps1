
param(
    [int]$Count = 40,
    [int]$Port = 5000
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path '.\textlink.exe')) {
    throw 'Please run mingw32-make first.'
}

$logDir = '.\benchmarks\logs_two_pc_receiver'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

Write-Host "Two-PC receiver benchmark"
Write-Host "Port: $Port"
Write-Host "Expected transfers: $Count"
Write-Host "Receiver logs: $logDir"
Write-Host ""

for ($i = 1; $i -le $Count; $i++) {

    $trialName = '{0:D2}' -f $i
    $logPath = Join-Path $logDir "receiver_$trialName.txt"

    if (Test-Path $logPath) {
        throw "Log already exists: $logPath. Use a fresh log directory."
    }

    Write-Host "[$i/$Count] Waiting for sender..."

    $previousPreference = $ErrorActionPreference

    try {
        $ErrorActionPreference = 'Continue'

        & .\textlink.exe recv $Port out 2>&1 |
            ForEach-Object { $_.ToString() } |
            Tee-Object -FilePath $logPath

        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousPreference
    }

    if ($exitCode -ne 0) {
        Write-Host "Receiver failed at transfer $i, exit=$exitCode"
        Write-Host "Check: $logPath"
        exit 1
    }

    if (-not (Select-String -Path $logPath -Pattern 'STATS role=recv' -Quiet)) {
        Write-Host "Missing receiver STATS in $logPath"
        exit 1
    }

    Write-Host "[$i/$Count] PASS - log saved"
    Write-Host ""

    Start-Sleep -Milliseconds 300
}

Write-Host "All $Count receiver transfers completed."
