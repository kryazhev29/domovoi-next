param([string]$Python='python', [string]$Output=(Join-Path $PSScriptRoot 'build\desktop'))
$ErrorActionPreference='Stop'
# Install into a dedicated venv: pyinstaller==6.16.0 pyserial==3.5
& $Python -m PyInstaller --noconfirm --onefile --windowed --name DomovoiService --distpath $Output --workpath (Join-Path $PSScriptRoot 'build\desktop-work') --specpath (Join-Path $PSScriptRoot 'build') (Join-Path $PSScriptRoot 'desktop_app.py')
if($LASTEXITCODE -ne 0) {throw 'Desktop build failed'}
