#!/usr/bin/env bash
set -euo pipefail

version="${1:?Usage: build-appimage.sh <version> [build-directory] [output-directory]}"
build_directory="${2:-build/release}"
output_directory="${3:-dist/linux}"
project_directory="$(pwd)"
app_directory="${output_directory}/LuminaReader.AppDir"
linuxdeployqt_path="${output_directory}/linuxdeployqt.AppImage"
appimage_path="${output_directory}/LuminaReader-linux-x86_64.AppImage"

mkdir -p "${output_directory}"
rm -rf "${app_directory}"
rm -f "${appimage_path}"
mkdir -p "${app_directory}"

cmake --install "${build_directory}" --prefix "${app_directory}/usr"
install -Dm644 assets/lumina-logo.png \
    "${app_directory}/usr/share/icons/hicolor/256x256/apps/lumina-reader.png"
install -Dm644 packaging/linux/lumina-reader.desktop \
    "${app_directory}/usr/share/applications/lumina-reader.desktop"
sed -i "s/@VERSION@/${version}/g" \
    "${app_directory}/usr/share/applications/lumina-reader.desktop"

curl --fail --location --retry 3 --silent --show-error \
    --output "${linuxdeployqt_path}" \
    "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod +x "${linuxdeployqt_path}"

# Qt ships optional SQL drivers that are not needed by Lumina Reader. The Mimer
# driver depends on a proprietary library that is not part of the Qt package,
# and linuxdeployqt aborts when ldd cannot resolve it. Temporarily hide only
# that optional plugin; SQLite remains available for the application.
optional_mimer_plugin="${QT_ROOT_DIR:-}/plugins/sqldrivers/libqsqlmimer.so"
optional_mimer_backup="${optional_mimer_plugin}.lumina-disabled"
restore_optional_plugin() {
    if [[ -n "${optional_mimer_plugin}" && -f "${optional_mimer_backup}" ]]; then
        mv "${optional_mimer_backup}" "${optional_mimer_plugin}"
    fi
}
trap restore_optional_plugin EXIT
if [[ -f "${optional_mimer_plugin}" ]]; then
    mv "${optional_mimer_plugin}" "${optional_mimer_backup}"
fi

"${linuxdeployqt_path}" --appimage-extract-and-run \
    "${app_directory}/usr/share/applications/lumina-reader.desktop" \
    -appimage \
    -no-translations \
    -qmldir="${project_directory}/src/ui/qml"

generated_appimage="$(find "${project_directory}" -maxdepth 1 -type f -name '*.AppImage' \
    ! -name 'linuxdeployqt.AppImage' -print -quit)"
if [[ -z "${generated_appimage}" ]]; then
    echo "linuxdeployqt did not produce an AppImage" >&2
    exit 1
fi

mv "${generated_appimage}" "${appimage_path}"
rm -f "${linuxdeployqt_path}"
echo "Created ${appimage_path}"
