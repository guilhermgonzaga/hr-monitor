# Obtém o IP da interface de rede ativa (ignora loopback e desconectadas)
$adapter = Get-NetIPAddress -AddressFamily IPv4 |
    Where-Object { $_.IPAddress -ne '127.0.0.1' -and $_.PrefixOrigin -ne 'WellKnown' -and $_.ValidLifetime -ne 0 } |
    Select-Object -First 1

if (-not $adapter) {
    Write-Error "Não foi possível obter o IP da interface de rede ativa."
    exit 1
}

$ip = [System.Net.IPAddress]::Parse($adapter.IPAddress)
$prefixLength = $adapter.PrefixLength

# Calcula a máscara de sub-rede
$mask = [uint32]0xFFFFFFFFu -shl (32 - $prefixLength)
$ipBytes = $ip.GetAddressBytes()
if ([BitConverter]::IsLittleEndian) {
    [Array]::Reverse($ipBytes)
}
$ipBytes = [BitConverter]::ToUInt32($ipBytes, 0)
$broadcastBytes = $ipBytes -band $mask -bor -bnot $mask
$broadcastIP = [System.Net.IPAddress]::Parse($broadcastBytes)
$broadcastPort = 3881

# Exibe IP e endereço de broadcast
Write-Host "IP Local: $($adapter.IPAddress)"
Write-Host "Endereço de Broadcast: $broadcastIP"

# Envia um pacote UDP para o broadcast (porta arbitrária)
$udpClient = New-Object System.Net.Sockets.UdpClient
$udpClient.EnableBroadcast = $true
$message = [System.Text.Encoding]::UTF8.GetBytes($(hostname))  # $ip.GetAddressBytes()
# Referência https://learn.microsoft.com/en-us/dotnet/api/system.net.sockets.udpclient.send
$numBytes = $udpClient.Send($message, $message.Length, $broadcastIP.ToString(), $broadcastPort)
$udpClient.Close()
Write-Host "Mensagem enviada para $broadcastIP`:$broadcastPort ($numBytes bytes)"
