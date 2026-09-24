function Find-VisualTools {
    $vswhere = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Microsoft Visual Studio/Installer/vswhere.exe'
    $installation = $null
    if (Test-Path $vswhere) {
        $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    }
    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmake) { $cmakePath = $cmake.Source }
    elseif ($installation) {
        $cmakePath = Join-Path $installation 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
    } else { throw 'Instale Visual Studio com C++ e CMake, ou disponibilize cmake no PATH.' }
    if (!(Test-Path $cmakePath)) { throw 'CMake nao encontrado.' }
    $ninja = Get-Command ninja -ErrorAction SilentlyContinue
    if ($ninja) { $ninjaPath = $ninja.Source }
    elseif ($installation) {
        $ninjaPath = Join-Path $installation 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'
    } else { $ninjaPath = $null }
    return @{ CMake = $cmakePath; Ninja = $ninjaPath }
}
