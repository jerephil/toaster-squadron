# windows-latest can move to a newer IDE; retain the requested VS 2022 toolchain.
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$existing = & $vswhere -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($existing) {
    Write-Host "Using existing Visual Studio 2022: $existing"
    exit 0
}
$bootstrapper = Join-Path $env:RUNNER_TEMP 'vs_buildtools_2022.exe'
Invoke-WebRequest 'https://aka.ms/vs/17/release/vs_BuildTools.exe' -OutFile $bootstrapper
$signature = Get-AuthenticodeSignature $bootstrapper
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'O=Microsoft Corporation') {
    throw 'Visual Studio bootstrapper signature is not valid Microsoft code signing.'
}
$installArgs = @('--quiet', '--wait', '--norestart', '--nocache', '--installPath', 'C:\BuildTools2022',
    '--add', 'Microsoft.VisualStudio.Workload.VCTools',
    '--add', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
    '--add', 'Microsoft.VisualStudio.Component.Windows11SDK.26100')
$result = Start-Process -FilePath $bootstrapper -ArgumentList $installArgs -Wait -PassThru
if ($result.ExitCode -notin @(0, 3010)) { throw "VS 2022 installer failed: $($result.ExitCode)" }
$installed = & $vswhere -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installed) { throw 'VS 2022 C++ toolchain was not detected after installation.' }
Write-Host "Installed Visual Studio 2022: $installed"
