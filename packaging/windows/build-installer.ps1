param(
    [Parameter(Mandatory = $true)]
    [string]$ApplicationPackage,

    [string]$OutputDirectory = "dist/windows",

    [string]$ApplicationVersion
)

$ErrorActionPreference = "Stop"

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$projectPath = Join-Path $PSScriptRoot "installer\LuminaReaderInstaller.csproj"
$uninstallerProjectPath = Join-Path $PSScriptRoot "installer\LuminaReaderUninstaller.csproj"
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
$uninstallerPublishPath = Join-Path ([IO.Path]::GetTempPath()) ("lumina-reader-uninstaller-" + [Guid]::NewGuid().ToString("N"))
$exitCode = 0

try {
    New-Item -ItemType Directory -Path $uninstallerPublishPath -Force | Out-Null

    & $dotnet publish $uninstallerProjectPath `
        --configuration Release `
        --runtime win-x64 `
        --self-contained true `
        "-p:ApplicationVersion=$ApplicationVersion" `
        --output $uninstallerPublishPath

    if ($LASTEXITCODE -ne 0) {
        $exitCode = $LASTEXITCODE
    }

    $uninstallerPath = Join-Path $uninstallerPublishPath "LuminaReaderUninstaller.exe"
    if ($exitCode -eq 0 -and -not (Test-Path -LiteralPath $uninstallerPath -PathType Leaf)) {
        throw "The uninstaller build did not produce $uninstallerPath."
    }

    if ($exitCode -eq 0) {
        & $dotnet publish $projectPath `
            --configuration Release `
            --runtime win-x64 `
            --self-contained true `
            "-p:ApplicationPackage=$packagePath" `
            "-p:ApplicationVersion=$ApplicationVersion" `
            "-p:LogoPath=$logoPath" `
            "-p:UninstallerPath=$uninstallerPath" `
            --output $publishPath

        if ($LASTEXITCODE -ne 0) {
            $exitCode = $LASTEXITCODE
        }
    }

    if ($exitCode -eq 0) {
        Write-Output (Join-Path $publishPath "LuminaReaderInstaller.exe")
    }
}
finally {
    if (Test-Path -LiteralPath $uninstallerPublishPath) {
        Remove-Item -LiteralPath $uninstallerPublishPath -Recurse -Force
    }
}

if ($exitCode -ne 0) {
    exit $exitCode
}
