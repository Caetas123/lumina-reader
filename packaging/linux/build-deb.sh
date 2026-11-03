#!/usr/bin/env bash
set -euo pipefail

version="${1:?Usage: build-deb.sh <version> [appdir] [output-directory]}"
app_directory="${2:-dist/linux/LuminaReader.AppDir}"
output_directory="${3:-dist/linux}"
project_directory="$(pwd)"
package_root="${output_directory}/LuminaReader-deb-root"
deb_path="${output_directory}/LuminaReader-linux-x86_64.deb"

if [[ ! -x "${app_directory}/usr/bin/lumina_reader" ]]; then
    echo "The deployed AppDir is missing ${app_directory}/usr/bin/lumina_reader" >&2
    echo "Build the AppImage first so the Qt runtime is available for the Debian package." >&2
    exit 1
fi

mkdir -p "${output_directory}"
rm -rf "${package_root}" "${deb_path}"
install -d "${package_root}/DEBIAN" "${package_root}/opt/lumina-reader" \
    "${package_root}/usr/bin" "${package_root}/usr/share/applications" \
    "${package_root}/usr/share/icons/hicolor/256x256/apps"
cp -a "${app_directory}/usr/." "${package_root}/opt/lumina-reader/usr"
sed "s/@VERSION@/${version}/g" \
    "${project_directory}/packaging/linux/debian/DEBIAN/control.in" \
    > "${package_root}/DEBIAN/control"
install -m755 "${project_directory}/packaging/linux/lumina-reader-launcher" \
    "${package_root}/usr/bin/lumina-reader"
sed -e "s/@VERSION@/${version}/g" -e 's/^Exec=lumina_reader$/Exec=lumina-reader/' \
    "${project_directory}/packaging/linux/lumina-reader.desktop" \
    > "${package_root}/usr/share/applications/lumina-reader.desktop"
install -m644 "${project_directory}/assets/lumina-logo.png" \
    "${package_root}/usr/share/icons/hicolor/256x256/apps/lumina-reader.png"

dpkg-deb --build --root-owner-group "${package_root}" "${deb_path}"
test -s "${deb_path}"
echo "Created ${deb_path}"
