$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$buildRoot = Join-Path $PSScriptRoot 'build'
$packPython = Join-Path $buildRoot 'packaging/Scripts/python.exe'
if (-not (Test-Path -LiteralPath $packPython)) {
    python -m venv --system-site-packages (Join-Path $buildRoot 'packaging')
    if ($LASTEXITCODE -ne 0) { throw 'Не удалось создать окружение сборки' }
}
& $packPython -m pip install 'pyinstaller==6.22.3' 'Pillow==12.2.0'
if ($LASTEXITCODE -ne 0) { throw 'Не удалось установить зависимости сборки' }
& $packPython -m PyInstaller --noconfirm --clean --onefile --windowed --name 'Камера_COM' `
    --distpath (Join-Path $buildRoot 'dist') --workpath (Join-Path $buildRoot 'work') `
    --specpath $buildRoot --add-data "$PSScriptRoot/sample_640x480.jpg;." `
    --exclude-module matplotlib --exclude-module numpy --exclude-module IPython `
    "$PSScriptRoot/camera_com_gui.py"
if ($LASTEXITCODE -ne 0) { throw 'Сборка EXE завершилась с ошибкой' }
