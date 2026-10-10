# TextLink TCP-05 / TCP-06 stream-boundary integration tests
# Windows PowerShell 5.1 compatible. Start textlink.exe recv 5000 out separately.
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('TCP-05', 'TCP-06', 'TCP-08')]
    [string]$Case,
    [string]$HostIP = '127.0.0.1',
    [int]$Port = 5000
)
$ErrorActionPreference = 'Stop'

function New-Frame {
    param([byte]$Type, [byte[]]$Payload)
    [uint32]$length = $Payload.Length + 1
    [byte[]]$header = @(
        [byte](($length -shr 24) -band 255),
        [byte](($length -shr 16) -band 255),
        [byte](($length -shr 8) -band 255),
        [byte]($length -band 255),
        $Type
    )
    return ,([byte[]]($header + $Payload))
}

function Read-Exactly {
    param([System.IO.Stream]$Stream, [int]$Length)
    [byte[]]$bytes = New-Object byte[] $Length
    $offset = 0
    while ($offset -lt $Length) {
        $got = $Stream.Read($bytes, $offset, $Length - $offset)
        if ($got -le 0) { throw "Connection closed before $Length bytes arrived." }
        $offset += $got
    }
    return ,$bytes
}

# Distinct file names keep the output of each test separate.
$fileName = switch ($Case) {
    'TCP-05' { 'tcp_stream_05.bin' }
    'TCP-06' { 'tcp_stream_06.bin' }
    'TCP-08' { 'tcp_stream_08.bin' }
}
$sourceFile = Join-Path $env:TEMP $fileName
[byte[]]$data = New-Object byte[] 4096
for ($i = 0; $i -lt $data.Length; $i++) {
    $data[$i] = [byte](($i * 17 + 31) % 256)
}
[System.IO.File]::WriteAllBytes($sourceFile, $data)

# FILE_BEGIN payload: RAW mode(0), original size (uint64 BE), data size (uint64 BE), ASCII filename.
$beginPayload = New-Object 'System.Collections.Generic.List[byte]'
$beginPayload.Add([byte]0)
foreach ($unused in 1..2) {
    [byte[]]$bigEndianSize = [BitConverter]::GetBytes([uint64]$data.Length)
    if ([BitConverter]::IsLittleEndian) { [Array]::Reverse($bigEndianSize) }
    $beginPayload.AddRange($bigEndianSize)
}
$beginPayload.AddRange([System.Text.Encoding]::ASCII.GetBytes($fileName))

$wire = New-Object 'System.Collections.Generic.List[byte]'
$wire.AddRange((New-Frame -Type 0x10 -Payload $beginPayload.ToArray()))

# Multiple FILE_DATA frames demonstrate that application-level reassembly is correct.
$offset = 0
foreach ($count in @(1000, 1200, 1896)) {
    [byte[]]$part = New-Object byte[] $count
    [Array]::Copy($data, $offset, $part, 0, $count)
    $wire.AddRange((New-Frame -Type 0x11 -Payload $part))
    $offset += $count
}
$wire.AddRange((New-Frame -Type 0x12 -Payload ([byte[]]@())))

$client = New-Object System.Net.Sockets.TcpClient
try {
    $client.Connect($HostIP, $Port)
    $stream = $client.GetStream()
    $stream.ReadTimeout = 10000
    $stream.WriteTimeout = 10000
    [byte[]]$allBytes = $wire.ToArray()

    if ($Case -eq 'TCP-05') {
        # Send tiny pieces with pauses, including pieces smaller than the 5-byte frame header.
        [int[]]$sizes = @(1, 2, 7, 13, 31)
        $pos = 0
        $j = 0
        while ($pos -lt $allBytes.Length) {
            $n = [Math]::Min($sizes[$j % $sizes.Length], $allBytes.Length - $pos)
            $stream.Write($allBytes, $pos, $n)
            $pos += $n
            $j++
            Start-Sleep -Milliseconds 2
        }
        Write-Host "[$Case] Fragmented $($allBytes.Length) bytes across $j TCP writes."
    }
    elseif ($Case -eq 'TCP-08') {
        # Send one byte per TCP stream write.
        # TCP may still combine writes internally; this verifies application-level writes.
        for ($i = 0; $i -lt $allBytes.Length; $i++) {
            $stream.Write($allBytes, $i, 1)
        }
        Write-Host "[$Case] Sent $($allBytes.Length) bytes using one-byte writes."
    }
    else {
        # One contiguous socket write containing five complete frames.
        $stream.Write($allBytes, 0, $allBytes.Length)
        Write-Host "[$Case] Sent $($allBytes.Length) bytes, 5 complete frames, in one TCP write."
    }

    # FILE_END reply: length=2, type=0x12, status=0 (6 bytes total)
    [byte[]]$ack = Read-Exactly -Stream $stream -Length 6
    [byte[]]$expectedAck = @(0, 0, 0, 2, 0x12, 0)
    for ($i = 0; $i -lt 6; $i++) {
        if ($ack[$i] -ne $expectedAck[$i]) {
            throw "Unexpected FILE_END acknowledgment: $([BitConverter]::ToString($ack))"
        }
    }
    Write-Host "[$Case] PASS: receiver acknowledged file completion."
    Write-Host "Original: $sourceFile"
    Write-Host "Received: out\$fileName"
    Write-Host "Verify: cmd /c fc /b `"$sourceFile`" `"out\$fileName`""
}
finally {
    $client.Close()
}
