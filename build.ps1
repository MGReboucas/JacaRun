param(
    [switch]$Test,
    [string]$Compiler = "g++"
)

$ErrorActionPreference = "Stop"
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Path "output" -Force | Out-Null
    $sources = @("src/GameManager.cpp", "src/Player.cpp", "src/Level.cpp", "src/Profile.cpp")
    $flags = @("-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Iinclude")
    & $Compiler @flags "main.cpp" @sources "-o" "output/jacarun.exe"
    if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar o jogo." }
    Write-Output "Jogo compilado: output/jacarun.exe"
    if ($Test) {
        & $Compiler @flags "tests/mechanics_tests.cpp" @sources "-o" "output/mechanics_tests.exe"
        if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar os testes." }
        & ".\output\mechanics_tests.exe"
        if ($LASTEXITCODE -ne 0) { throw "Falha nos testes de mecanicas." }
    }
} finally {
    Pop-Location
}
