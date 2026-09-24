[CmdletBinding()]
param(
    [string]$AndroidSdk = $env:ANDROID_HOME,
    [string]$AndroidNdk,
    [string]$JavaHome = $env:JAVA_HOME,
    [switch]$SkipSetup
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'scripts/VisualTools.ps1')
$tools = Find-VisualTools
if (!$SkipSetup) { & (Join-Path $PSScriptRoot 'scripts/setup-visual.ps1') }
if (!$AndroidSdk) {
    $candidates = @(
        (Join-Path $env:LOCALAPPDATA 'Android/Sdk'),
        (Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Android/android-sdk')
    )
    $AndroidSdk = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (!$AndroidNdk -and $AndroidSdk) {
    $candidate = Join-Path $AndroidSdk 'ndk/27.2.12479018'
    if (Test-Path $candidate) { $AndroidNdk = $candidate }
}
if (!$AndroidNdk) {
    $candidate = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Android/AndroidNDK/android-ndk-r27c'
    if (Test-Path $candidate) { $AndroidNdk = $candidate }
}
if (!$JavaHome) {
    $candidate = Join-Path $env:ProgramFiles 'Android/Android Studio/jbr'
    if (Test-Path $candidate) { $JavaHome = $candidate }
}
foreach ($required in @($AndroidSdk,$AndroidNdk,$JavaHome,$tools.Ninja)) {
    if (!$required -or !(Test-Path $required)) {
        throw 'Informe -AndroidSdk, -AndroidNdk (r27c), -JavaHome (JDK 17+) e instale Ninja/CMake.'
    }
}
foreach ($component in @('platforms/android-36','build-tools/36.0.0','platform-tools')) {
    if (!(Test-Path (Join-Path $AndroidSdk $component))) { throw "Falta no SDK: $component. Instale pelo Android Studio." }
}
$revision = Get-Content (Join-Path $AndroidNdk 'source.properties') | Select-String '^Pkg.Revision\s*=\s*27\.2\.12479018\s*$'
if (!$revision) { throw 'Esta prova usa NDK r27c (27.2.12479018).' }

# Local SDK view: do not move or change a system installation.
$overlay = Join-Path $PSScriptRoot '.deps/android-sdk'
New-Item -ItemType Directory -Path (Join-Path $overlay 'ndk') -Force | Out-Null
function Ensure-Junction([string]$Link, [string]$Target) {
    $targetPath = (Resolve-Path $Target).Path
    if (Test-Path $Link) {
        $item = Get-Item $Link
        if (!$item.Target -or [IO.Path]::GetFullPath($item.Target[0]).TrimEnd('\') -ne $targetPath.TrimEnd('\')) {
            throw "Ja existe outro caminho em $Link. Preserve-o antes de configurar um SDK diferente."
        }
    } else { New-Item -ItemType Junction -Path $Link -Target $targetPath | Out-Null }
}
foreach ($folder in @('platforms','build-tools','platform-tools')) {
    Ensure-Junction (Join-Path $overlay $folder) (Join-Path $AndroidSdk $folder)
}
Ensure-Junction (Join-Path $overlay 'ndk/27.2.12479018') $AndroidNdk
$cmakeDir = Split-Path (Split-Path $tools.CMake)
$properties = 'sdk.dir=' + $overlay.Replace('\','/') + [Environment]::NewLine +
              'cmake.dir=' + $cmakeDir.Replace('\','/') + [Environment]::NewLine
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'visual/proj.android/local.properties'), $properties)

$previous = @{}
foreach ($name in @('AX_ROOT','JAVA_HOME','ANDROID_HOME','GRADLE_USER_HOME','PATH','PSExecutionPolicyPreference','DEBUG')) {
    $previous[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
try {
    $env:AX_ROOT = Join-Path $PSScriptRoot '.deps/axmol'
    $env:JAVA_HOME = $JavaHome
    $env:ANDROID_HOME = $overlay.Replace('\','/')
    $env:GRADLE_USER_HOME = Join-Path $PSScriptRoot '.deps/gradle'
    $env:PSExecutionPolicyPreference = 'Bypass'
    $env:DEBUG = $null
    $env:PATH = (Join-Path $JavaHome 'bin') + ';' + (Split-Path $tools.CMake) + ';' +
                (Split-Path $tools.Ninja) + ';' + $env:PATH
    & (Join-Path $PSScriptRoot 'visual/proj.android/gradlew.bat') -p (Join-Path $PSScriptRoot 'visual/proj.android') :JacaRun:assembleDebug --no-daemon --max-workers=4
    if ($LASTEXITCODE -ne 0) { throw 'Falha no build Android; consulte o erro do Gradle acima.' }
    $apk = Join-Path $PSScriptRoot 'visual/proj.android/app/build/outputs/apk/debug/JacaRun-debug.apk'
    New-Item -ItemType Directory -Path (Join-Path $PSScriptRoot 'output') -Force | Out-Null
    Copy-Item -LiteralPath $apk -Destination (Join-Path $PSScriptRoot 'output/JacaRun-debug.apk') -Force
    Write-Host ('APK: ' + (Join-Path $PSScriptRoot 'output/JacaRun-debug.apk'))
} finally {
    foreach ($name in $previous.Keys) { [Environment]::SetEnvironmentVariable($name, $previous[$name], 'Process') }
}
