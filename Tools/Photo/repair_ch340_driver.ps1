# Run elevated. Require a single connected CH340; back up first.
# Windows also updates saved instances with the same hardware ID.
param([Parameter(Mandatory=$true)][string]$DriverRoot)
$ErrorActionPreference = 'Stop'
$DriverRoot = (Resolve-Path -LiteralPath $DriverRoot).Path
$resultFile = Join-Path $DriverRoot 'repair_result.json'
$result = [ordered]@{ success=$false; rebootRequired=$false; error=$null }
$transcribing = $false
try {
    $principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'Administrator rights are required to install a Windows driver.'
    }
    Start-Transcript -LiteralPath (Join-Path $DriverRoot 'repair.log') -Append | Out-Null
    $transcribing = $true
    $hardware = 'USB\VID_1A86&PID_7523'
    $devices = @(Get-PnpDevice -Class Ports -PresentOnly | Where-Object { $_.InstanceId -like "$hardware\*" })
    if ($devices.Count -ne 1) { throw "Expected exactly one connected CH340; found $($devices.Count)." }
    $deviceId = $devices[0].InstanceId
    $result.device = $deviceId
    # Win32_PnPSignedDriver can retain a stale version after installation.
    $driverInf = (Get-PnpDeviceProperty -InstanceId $deviceId -KeyName 'DEVPKEY_Device_DriverInfPath').Data
    $driverVersion = (Get-PnpDeviceProperty -InstanceId $deviceId -KeyName 'DEVPKEY_Device_DriverVersion').Data
    if (-not $driverInf) { throw 'Cannot determine current driver for backup.' }
    $result.previousVersion = $driverVersion
    $backup = Join-Path $DriverRoot ('backup_' + $driverVersion)
    New-Item -ItemType Directory -Path $backup -Force | Out-Null
    & pnputil.exe /export-driver $driverInf $backup
    if ($LASTEXITCODE -ne 0) { throw 'Driver backup failed; no installation attempted.' }
    $result.backup = $backup
    $inf = Join-Path $DriverRoot 'v3.7\WIN 1X\CH341SER.INF'
    $catalog = Join-Path $DriverRoot 'v3.7\WIN 1X\CH341SER.CAT'
    $signature = Get-AuthenticodeSignature -LiteralPath $catalog
    if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notlike '*Microsoft Windows Hardware Compatibility Publisher*') {
        throw 'Driver catalog signature validation failed.'
    }
    if (-not (Select-String -LiteralPath $inf -Pattern 'DriverVer\s*=\s*01/18/2022,\s*3\.7\.2022\.01')) {
        throw 'Unexpected driver version; expected 3.7.2022.01.'
    }
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class Ch340DriverRepair {
    [DllImport("newdev.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool UpdateDriverForPlugAndPlayDevicesW(
        IntPtr hwnd, string hardwareId, string infPath, uint flags,
        [MarshalAs(UnmanagedType.Bool)] out bool reboot);
}
'@
    $reboot = $false
    # INSTALLFLAG_FORCE selects the older, signed driver. Never reboot automatically.
    $ok = [Ch340DriverRepair]::UpdateDriverForPlugAndPlayDevicesW([IntPtr]::Zero, $hardware, $inf, 1, [ref]$reboot)
    if (-not $ok) {
        $code = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw [ComponentModel.Win32Exception]::new($code)
    }
    $result.rebootRequired = $reboot
    $result.installedVersion = (Get-PnpDeviceProperty -InstanceId $deviceId -KeyName 'DEVPKEY_Device_DriverVersion').Data
    if ($result.installedVersion -ne '3.7.2022.1') {
        throw "Driver installation returned success, but active version is $($result.installedVersion)."
    }
    $result.success = $true
} catch {
    $result.error = $_.Exception.Message
} finally {
    $result | ConvertTo-Json | Set-Content -LiteralPath $resultFile -Encoding UTF8
    if ($transcribing) { Stop-Transcript | Out-Null }
}
if (-not $result.success) { exit 1 }
