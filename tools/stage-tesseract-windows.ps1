param(
    [Parameter(Mandatory = $true)]
    [string]$Destination,

    [string]$InstallerUrl = "https://github.com/tesseract-ocr/tesseract/releases/download/5.5.0/tesseract-ocr-w64-setup-5.5.0.20241111.exe",

    [string]$SevenZipPath
)

$ErrorActionPreference = "Stop"

$destination = [IO.Path]::GetFullPath($Destination)
$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ("lumina-tesseract-" + [Guid]::NewGuid().ToString("N"))
$installer = Join-Path $tempRoot "tesseract-installer.exe"
$extractRoot = Join-Path $tempRoot "extracted"
$languageManifest = Join-Path $PSScriptRoot "tesseract-languages.txt"
$tessdataFastRefPath = Join-Path $PSScriptRoot "tessdata-fast-ref.txt"
$tessdataFastRef = (Get-Content -LiteralPath $tessdataFastRefPath |
    Where-Object { -not [string]::IsNullOrWhiteSpace($_) -and -not $_.Trim().StartsWith('#') } |
    Select-Object -First 1).Trim()
$languages = @(Get-Content -LiteralPath $languageManifest |
    ForEach-Object { $_.Trim() } |
    Where-Object { -not [string]::IsNullOrWhiteSpace($_) -and -not $_.StartsWith('#') })
if ([string]::IsNullOrWhiteSpace($tessdataFastRef) -or $languages.Count -eq 0) {
    throw "The Tesseract language manifest is empty or invalid."
}

$sevenZip = if ([string]::IsNullOrWhiteSpace($SevenZipPath)) {
    (Get-Command 7z.exe -ErrorAction SilentlyContinue)?.Source
} else {
    (Resolve-Path -LiteralPath $SevenZipPath).Path
}
if ([string]::IsNullOrWhiteSpace($sevenZip)) {
    $sevenZip = (Get-Command 7za.exe -ErrorAction SilentlyContinue)?.Source
}
if ([string]::IsNullOrWhiteSpace($sevenZip)) {
    throw "7-Zip is required to extract the Tesseract installer."
}

try {
    New-Item -ItemType Directory -Path $tempRoot, $extractRoot, $destination -Force | Out-Null
    Invoke-WebRequest -Uri $InstallerUrl -OutFile $installer -UseBasicParsing
    & $sevenZip x $installer "-o$extractRoot" -y | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "7-Zip failed to extract Tesseract with exit code $LASTEXITCODE."
    }

    $tesseract = Get-ChildItem -LiteralPath $extractRoot -Filter "tesseract.exe" -File -Recurse |
        Select-Object -First 1
    if ($null -eq $tesseract) {
        throw "The Tesseract installer did not contain tesseract.exe."
    }
    $installRoot = $tesseract.Directory.FullName
    $tessdata = Join-Path $installRoot "tessdata"
    if (-not (Test-Path -LiteralPath $tessdata -PathType Container)) {
        throw "The extracted Tesseract installation has no tessdata directory."
    }

    Copy-Item -LiteralPath $tesseract.FullName -Destination (Join-Path $destination "tesseract.exe") -Force
    Get-ChildItem -LiteralPath $installRoot -Filter "*.dll" -File |
        Copy-Item -Destination $destination -Force

    $destinationTessdata = Join-Path $destination "tessdata"
    New-Item -ItemType Directory -Path $destinationTessdata -Force | Out-Null
    $configs = Join-Path $tessdata "configs"
    if (-not (Test-Path -LiteralPath (Join-Path $configs "tsv") -PathType Leaf)) {
        throw "The extracted Tesseract installation has no tessdata\\configs\\tsv file."
    }
    Copy-Item -LiteralPath $configs -Destination $destinationTessdata -Recurse -Force
    foreach ($language in $languages) {
        $target = Join-Path $destinationTessdata "$language.traineddata"
        $languageUrl = "https://raw.githubusercontent.com/tesseract-ocr/tessdata_fast/$tessdataFastRef/$language.traineddata"
        Invoke-WebRequest -Uri $languageUrl -OutFile $target -UseBasicParsing
        if ((Get-Item -LiteralPath $target).Length -le 0) {
            throw "Tesseract language data is empty: $language"
        }
    }

    Get-ChildItem -LiteralPath $installRoot -File -Filter "LICENSE*" -ErrorAction SilentlyContinue |
        Copy-Item -Destination $destination -Force
    "Tesseract Windows runtime staged from $InstallerUrl" |
        Set-Content -LiteralPath (Join-Path $destination "TESSERACT_SOURCE.txt") -Encoding ascii
    "tessdata_fast commit: $tessdataFastRef; models: $($languages.Count)" |
        Add-Content -LiteralPath (Join-Path $destination "TESSERACT_SOURCE.txt") -Encoding ascii
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}
