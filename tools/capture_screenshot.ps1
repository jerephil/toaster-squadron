param(
    [Parameter(Mandatory=$true)][string]$Saver,
    [Parameter(Mandatory=$true)][string]$Output
)
# Capture pixels from the actual running saver on the hosted Windows desktop.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class CaptureWindow {
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hwnd, uint msg, IntPtr wp, IntPtr lp);
    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr context);
}
'@
[CaptureWindow]::SetProcessDpiAwarenessContext([IntPtr](-4)) | Out-Null
$process = Start-Process -FilePath (Resolve-Path $Saver) -ArgumentList '/s' -PassThru
$bitmap = $null
$graphics = $null
try {
    Start-Sleep -Milliseconds 1100
    if ($process.HasExited) { throw 'Saver exited before screen capture.' }
    $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bitmap = [System.Drawing.Bitmap]::new($bounds.Width, $bounds.Height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
    $bitmap.Save([System.IO.Path]::GetFullPath($Output), [System.Drawing.Imaging.ImageFormat]::Png)
    Write-Host "Captured actual running saver to $Output"
} finally {
    if ($graphics) { $graphics.Dispose() }
    if ($bitmap) { $bitmap.Dispose() }
    $process.Refresh()
    if (-not $process.HasExited) {
        [CaptureWindow]::PostMessage($process.MainWindowHandle, 0x0100, [IntPtr]0x1B, [IntPtr]::Zero) | Out-Null
        if (-not $process.WaitForExit(3000)) { $process.Kill(); $process.WaitForExit() }
    }
    $process.Dispose()
}
