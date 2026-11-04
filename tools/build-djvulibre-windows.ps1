param(
    [Parameter(Mandatory = $true)]
    [string]$VcpkgRoot,

    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,

    [string]$SourceRevision = "69935aa3e82bc47623e10c4833d5044939e75474",

    [string]$CMakePath,

    [string]$NinjaPath
)

$ErrorActionPreference = "Stop"

$vcpkgRoot = (Resolve-Path -LiteralPath $VcpkgRoot).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
$toolchain = Join-Path $vcpkgRoot "scripts\buildsystems\vcpkg.cmake"
$vcpkgInstall = Join-Path $vcpkgRoot "installed\x64-windows"
$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ("lumina-djvulibre-" + [Guid]::NewGuid().ToString("N"))
$source = Join-Path $tempRoot "source"
$build = Join-Path $tempRoot "build"
$include = Join-Path $output "include\libdjvu"
$lib = Join-Path $output "lib"
$bin = Join-Path $output "bin"
$cmake = if ([string]::IsNullOrWhiteSpace($CMakePath)) {
    (Get-Command cmake.exe -ErrorAction Stop).Source
} else {
    (Resolve-Path -LiteralPath $CMakePath).Path
}
$ninja = if ([string]::IsNullOrWhiteSpace($NinjaPath)) {
    (Get-Command ninja.exe -ErrorAction Stop).Source
} else {
    (Resolve-Path -LiteralPath $NinjaPath).Path
}

try {
    New-Item -ItemType Directory -Path $tempRoot, $output, $include, $lib, $bin -Force | Out-Null

    git init $source | Out-Null
    git -C $source remote add origin https://github.com/barak/djvulibre.git
    git -C $source fetch --depth 1 origin $SourceRevision
    git -C $source checkout --detach $SourceRevision | Out-Null

    @'
/* Minimal Windows configuration for Lumina Reader's x64 DjVu build. */
#define HAVE_CONFIG_H 1
#define HAVE_NAMESPACES 1
#define HAVE_SNPRINTF 1
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_WCHAR_H 1
#define HAVE_WCTYPE_H 1
#define HAVE_MBSTATE_T 1
#define HAVE_LONG_LONG_INT 1
#define HAVE_EXCEPTIONS 1
#define HAVE_PTHREAD 0
#define HAVE_QTHREAD 0
#define NEED_GNUG_PRAGMAS 0
#define NEED_JPEG_DECODER 1
#define USE_WINTHREADS 1
#define DJVULIBRE_VERSION "3.5.30"
'@ | Set-Content -LiteralPath (Join-Path $source "libdjvu\config.h") -Encoding utf8

    @'
cmake_minimum_required(VERSION 3.24)
project(DjVuLibreX64 LANGUAGES C CXX)

find_package(JPEG REQUIRED)

file(GLOB DJVU_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libdjvu/*.cpp")
add_library(djvulibre SHARED ${DJVU_SOURCES})
target_include_directories(djvulibre
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/libdjvu"
        "${CMAKE_CURRENT_SOURCE_DIR}/libjpeg"
)
target_compile_definitions(djvulibre PRIVATE
    HAVE_CONFIG_H=1
    HAVE_NAMESPACES=1
    HAVE_PTHREAD=0
    HAVE_QTHREAD=0
    HAVE_SNPRINTF=1
    NEED_GNUG_PRAGMAS=0
    NEED_JPEG_DECODER=1
    DJVUAPI_EXPORT=1
    DDJVUAPI_EXPORT=1
    MINILISPAPI_EXPORT=1
    WIN32_LEAN_AND_MEAN
)
target_compile_options(djvulibre PRIVATE /W2 /wd4996 /wd4267 /wd4244)
target_link_libraries(djvulibre PRIVATE JPEG::JPEG)
set_target_properties(djvulibre PROPERTIES OUTPUT_NAME "djvulibre")
'@ | Set-Content -LiteralPath (Join-Path $source "CMakeLists.txt") -Encoding utf8

    & $cmake -S $source -B $build -G Ninja `
        -DCMAKE_BUILD_TYPE=Release `
        ("-DCMAKE_TOOLCHAIN_FILE=" + $toolchain) `
        ("-DCMAKE_MAKE_PROGRAM=" + $ninja) `
        "-DVCPKG_TARGET_TRIPLET=x64-windows" `
        "-DCMAKE_C_COMPILER=cl" `
        "-DCMAKE_CXX_COMPILER=cl" `
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL
    if ($LASTEXITCODE -ne 0) {
        throw "DjVuLibre configuration failed with exit code $LASTEXITCODE."
    }
    & $cmake --build $build --parallel 2
    if ($LASTEXITCODE -ne 0) {
        throw "DjVuLibre build failed with exit code $LASTEXITCODE."
    }

    Copy-Item -LiteralPath (Join-Path $build "djvulibre.dll") -Destination $bin -Force
    Copy-Item -LiteralPath (Join-Path $build "djvulibre.lib") -Destination $lib -Force
    Copy-Item -Path (Join-Path $source "libdjvu\*.h") -Destination $include -Force

    $jpegRuntime = Join-Path $vcpkgInstall "bin\jpeg62.dll"
    if (Test-Path -LiteralPath $jpegRuntime) {
        Copy-Item -LiteralPath $jpegRuntime -Destination $bin -Force
    }
    $SourceRevision | Set-Content -LiteralPath (Join-Path $output "DJVU_SOURCE_REVISION.txt") -Encoding ascii
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}
