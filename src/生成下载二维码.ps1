# 生成 APK 下载二维码：电脑开一个小 HTTP 服务，手机同 Wi-Fi 扫码下载
# 双击运行（右键“使用 PowerShell 运行”），或终端执行  .\生成下载二维码.ps1
$ErrorActionPreference = 'Stop'

$py    = 'C:\Users\97603\.dsh\dsh-runtimes\dsh-primary-runtime\dependencies\python\python.exe'
$share = 'C:\Users\97603\AndroidDev\share'
$apk   = 'C:\Users\97603\Desktop\自用备忘录\学习备忘录-v1.0-debug.apk'
$qr    = 'C:\Users\97603\Desktop\自用备忘录\扫码下载.png'
$port  = 8000

New-Item -ItemType Directory -Force $share | Out-Null
Copy-Item $apk "$share\memo-v1.0.apk" -Force

# 取默认路由网卡的 IPv4（换了网络会自动变）
$idx = (Get-NetRoute -DestinationPrefix '0.0.0.0/0' -ErrorAction Stop |
        Sort-Object RouteMetric | Select-Object -First 1).InterfaceIndex
$ip  = (Get-NetIPAddress -AddressFamily IPv4 -InterfaceIndex $idx -ErrorAction Stop |
        Select-Object -First 1).IPAddress
if (-not $ip) { throw '没找到局域网 IP，请确认已连上 Wi-Fi/热点' }

if (-not (Get-NetTCPConnection -LocalPort $port -State Listen -ErrorAction SilentlyContinue)) {
    Start-Process -FilePath $py `
        -ArgumentList @('-m', 'http.server', "$port", '--bind', '0.0.0.0', '--directory', $share) `
        -WindowStyle Hidden
    Start-Sleep -Seconds 2
}

$url = "http://${ip}:${port}/memo-v1.0.apk"
& $py 'C:\Users\97603\AndroidDev\memo-app\make_qr.py' $url $qr

Write-Host ''
Write-Host "下载地址: $url"
Write-Host "二维码  : $qr"
Write-Host '手机连同一个 Wi-Fi，扫码后点“下载”。'
Write-Host '若提示无法访问：Windows 弹窗点“允许访问”，或手机开热点让电脑连上后重跑本脚本。'
Start-Process $qr
