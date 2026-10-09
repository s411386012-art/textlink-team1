# TextLink malformed TCP input generator (Windows PowerShell 5.1+)
# Run receiver separately: .\textlink.exe recv 5000 out
# Example: powershell -ExecutionPolicy Bypass -File .\tests\test_tcp_malformed.ps1 -Case TCP-01
param(
    [ValidateSet('TCP-01','TCP-02','TCP-03','TCP-04','All')]
    [string]$Case = 'TCP-01',
    [string]$HostIp = '127.0.0.1',
    [ValidateRange(1,65535)]
    [int]$Port = 5000
)

$ErrorActionPreference = 'Stop'

function Send-MalformedFrame([string]$Id) {
    $client = $null
    try {
        $client = New-Object System.Net.Sockets.TcpClient
        $client.Connect($HostIp, $Port)
        $stream = $client.GetStream()
        switch ($Id) {
            'TCP-01' {
                # Incomplete 5-byte header: only two bytes
                [byte[]]$bytes = @(0x00,0x00)
            }
            'TCP-02' {
                # Length=11 (1 type + 10 payload), type=0x01,
                # but supply only three payload bytes (abc).
                [byte[]]$bytes = @(0x00,0x00,0x00,0x0B,0x01,0x61,0x62,0x63)
            }
            'TCP-03' {
                # Length=0x01000001 exceeds 16 MiB by one byte
                [byte[]]$bytes = @(0x01,0x00,0x00,0x01,0x01)
            }
            'TCP-04' {
                # Legal length=1, unknown type=0xFF, empty payload
                [byte[]]$bytes = @(0x00,0x00,0x00,0x01,0xFF)
            }
        }
        $stream.Write($bytes, 0, $bytes.Length)
        $stream.Flush()
        Write-Host "[$Id] Sent $($bytes.Length) bytes to ${HostIp}:${Port}; closing connection."
        Write-Host 'Check receiver output manually; successful sending is NOT a test PASS.'
    }
    finally {
        if ($null -ne $client) { $client.Close() }
    }
}

if ($Case -eq 'All') {
    Write-Host 'Start a NEW receiver before each case; the receiver exits after one connection.'
    foreach ($id in @('TCP-01','TCP-02','TCP-03','TCP-04')) {
        Read-Host "Start receiver for $id, then press Enter"
        Send-MalformedFrame $id
        Read-Host "Record receiver result for $id, then press Enter"
    }
} else {
    Send-MalformedFrame $Case
}
