param(
    [Parameter(Mandatory = $true)]
    [string]$ApplicationPackage,

    [string]$OutputDirectory = "dist/windows"
)

$ErrorActionPreference = "Stop"

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$projectPath = Join-Path $PSScriptRoot "installer\LuminaReaderInstaller.csproj"
$logoPath = Join-Path $repositoryRoot "assets\lumina-logo.png"
$packagePath = (Resolve-Path $ApplicationPackage).Path
$outputPath = Join-Path $repositoryRoot $OutputDirectory

New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
$publishPath = Join-Path $outputPath "installer"

$dotnet = (Get-Command dotnet -ErrorAction Stop).Source
& $dotnet publish $projectPath `
    --configuration Release `
    --runtime win-x64 `
    --self-contained true `
    "-p:ApplicationPackage=$packagePath" `
    "-p:LogoPath=$logoPath" `
    --output $publishPath

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Output (Join-Path $publishPath "LuminaReaderInstaller.exe")
