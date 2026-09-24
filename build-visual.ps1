[CmdletBinding()]
param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [ValidateSet('Regular','Tall')][string]$Aspect = 'Regular',
    [switch]$Run,
    [switch]$Smoke,
    [switch]$SkipSetup
)
$ErrorActionPreference = 'Stop'
if ($Smoke -and $Run) { throw 'Use -Run ou -Smoke separadamente.' }
if ($Smoke -and $Configuration -ne 'Debug') { throw 'O teste visual requer Debug.' }
. (Join-Path $PSScriptRoot 'scripts/VisualTools.ps1')
$tool = Find-VisualTools
if (!$SkipSetup) { & (Join-Path $PSScriptRoot 'scripts/setup-visual.ps1') }
$oldRoot = $env:AX_ROOT
$oldPolicy = $env:PSExecutionPolicyPreference
$oldAspect = $env:JACARUN_VISUAL_ASPECT
try {
    $env:AX_ROOT = Join-Path $PSScriptRoot '.deps/axmol'
    $env:PSExecutionPolicyPreference = 'Bypass'
    $env:JACARUN_VISUAL_ASPECT = $Aspect
    $build = Join-Path $PSScriptRoot 'visual/build-win32'
    & $tool.CMake -S (Join-Path $PSScriptRoot 'visual') -B $build -A x64
    if ($LASTEXITCODE -ne 0) { throw 'Falha na configuracao visual.' }
    & $tool.CMake --build $build --config $Configuration --target JacaRun --parallel 4
    if ($LASTEXITCODE -ne 0) { throw 'Falha na compilacao visual.' }
    $exe = Join-Path $build "bin/JacaRun/$Configuration/JacaRun.exe"
    Write-Host "Executavel: $exe"
    if ($Run) { Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) }
    if ($Smoke) {
        $oldSmoke = $env:JACARUN_VISUAL_SMOKE
        try {
            $dir = Join-Path $PSScriptRoot ('output/visual-smoke-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
            $env:JACARUN_VISUAL_SMOKE = $dir
            $process = Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) -WindowStyle Hidden -PassThru
            if (!$process.WaitForExit(45000)) { throw "Teste nao terminou em 45s. Processo $($process.Id); confira o aplicativo." }
            $result = Join-Path $dir 'result.txt'
            if (!(Test-Path $result)) { throw 'Teste nao produziu relatorio; confira se o sistema permitiu executar o aplicativo.' }
            $lines = Get-Content $result
            if ($process.ExitCode -ne 0 -or $lines[0] -ne 'PASS') { throw "Teste falhou: $($lines -join ' ')" }
            foreach ($capture in @('01-menu.png','02-jump.png','03-pause.png','04-game-over.png','05-objects-behind.png')) {
                $imagePath = Join-Path $dir $capture
                if (!(Test-Path $imagePath) -or (Get-Item $imagePath).Length -eq 0) {
                    throw "Captura ausente: $imagePath"
                }
            }
            Write-Host ($lines -join [Environment]::NewLine)
            Write-Host "Evidencias: $dir"
        } finally { $env:JACARUN_VISUAL_SMOKE = $oldSmoke }
    }
} finally {
    $env:AX_ROOT = $oldRoot
    $env:PSExecutionPolicyPreference = $oldPolicy
    $env:JACARUN_VISUAL_ASPECT = $oldAspect
}
