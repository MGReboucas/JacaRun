[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$engine = Join-Path $repo '.deps/axmol'
$commit = 'b14941e6f50a0ce12489bd8f57041e093fb58819'
New-Item -ItemType Directory -Path (Join-Path $repo '.deps') -Force | Out-Null
if (!(Test-Path (Join-Path $engine '.git'))) {
    & git clone --depth 1 --branch v2.11.4 https://github.com/axmolengine/axmol.git $engine
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao obter Axmol.' }
}
$actual = & git -C $engine rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $actual.Trim() -ne $commit) {
    throw "Axmol diferente da revisao esperada ($commit). Preserve sua copia antes de preparar outra."
}
$archive = Join-Path $repo '.deps/axslcc-1.14.0-win64.zip'
$expected = '5ea1ae25517a14a81b578836563837fddcb3feca005581761446f3e75986e828'
if (!(Test-Path $archive)) {
    Invoke-WebRequest -UseBasicParsing -Uri 'https://github.com/axmolengine/axslcc/releases/download/v1.14.0/axslcc-1.14.0-win64.zip' -OutFile $archive
}
if ((Get-FileHash -Algorithm SHA256 $archive).Hash.ToLowerInvariant() -ne $expected) {
    throw 'SHA256 do axslcc nao corresponde ao pacote oficial fixado.'
}
$shaderTools = Join-Path $engine 'tools/external/axslcc'
if (!(Test-Path (Join-Path $shaderTools 'axslcc.exe'))) {
    Expand-Archive -LiteralPath $archive -DestinationPath $shaderTools -Force
}
Write-Host "Axmol 2.11.4 e axslcc 1.14.0 preparados em $engine"
