param(
    [Parameter(Mandatory = $true)]
    [string]$ApplicationPackage,

    [string]$OutputDirectory = "dist/windows",

    [string]$ApplicationVersion
)

$ErrorActionPreference = "Stop"

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$projectPath = Join-Path $PSScriptRoot "installer\LuminaReaderInstaller.csproj"
$logoPath = Join-Path $repositoryRoot "assets\lumina-logo.png"
$packagePath = (Resolve-Path $ApplicationPackage).Path
$outputPath = Join-Path $repositoryRoot $OutputDirectory
$versionPath = Join-Path $repositoryRoot "project-version.txt"
$projectVersion = (Get-Content -Raw -Path $versionPath).Trim()

if ($projectVersion -notmatch '^\d+\.\d+\.\d+$') {
    throw "project-version.txt must contain one semantic version such as 0.2.0."
}

if ([string]::IsNullOrWhiteSpace($ApplicationVersion)) {
    $ApplicationVersion = $projectVersion
} elseif ($ApplicationVersion -ne $projectVersion) {
    throw "Installer version '$ApplicationVersion' does not match project-version.txt '$projectVersion'."
}

New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
$publishPath = Join-Path $outputPath "installer"

$dotnet = (Get-Command dotnet -ErrorAction Stop).Source
& $dotnet publish $projectPath `
    --configuration Release `
    --runtime win-x64 `
    --self-contained true `
    "-p:ApplicationPackage=$packagePath" `
    "-p:ApplicationVersion=$ApplicationVersion" `
    "-p:LogoPath=$logoPath" `
    --output $publishPath

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Output (Join-Path $publishPath "LuminaReaderInstaller.exe")
