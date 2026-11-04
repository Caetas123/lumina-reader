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

# Keep OCR self-contained in the AppImage and the Debian package.  The
# application first looks next to itself at usr/bin/ocr/tesseract, so the
# release does not depend on a user's PATH or on a system-specific tessdata
# directory.
if command -v tesseract >/dev/null 2>&1; then
    tessdata_directory="$(tesseract --print-tessdata-dir 2>/dev/null || true)"
    if [[ ! -d "${tessdata_directory}" ]]; then
        for candidate in \
            "${TESSDATA_PREFIX:-}" \
            "/usr/share/tesseract-ocr/5/tessdata" \
            "/usr/share/tesseract-ocr/4.00/tessdata" \
            "/usr/share/tessdata"; do
            if [[ -n "${candidate}" && -d "${candidate}" ]]; then
                tessdata_directory="${candidate}"
                break
            fi
        done
    fi
    if [[ -z "${tessdata_directory}" || ! -d "${tessdata_directory}" ]]; then
        echo "Could not locate a usable Tesseract tessdata directory" >&2
        exit 1
    fi
    install -Dm755 "$(command -v tesseract)" \
        "${app_directory}/usr/bin/ocr/tesseract"
    for language in eng por spa jpn kor chi_sim chi_tra osd; do
        if [[ ! -f "${tessdata_directory}/${language}.traineddata" ]]; then
            echo "Missing Tesseract language data: ${language}" >&2
            exit 1
        fi
        install -Dm644 "${tessdata_directory}/${language}.traineddata" \
            "${app_directory}/usr/bin/ocr/tessdata/${language}.traineddata"
    done
    if [[ ! -f "${tessdata_directory}/configs/tsv" ]]; then
        echo "Missing Tesseract TSV configuration: ${tessdata_directory}/configs/tsv" >&2
        exit 1
    fi
    cp -a "${tessdata_directory}/configs" \
        "${app_directory}/usr/bin/ocr/tessdata/"

    # linuxdeployqt follows the main Qt executable.  Tesseract is a child
    # process, so explicitly copy its non-glibc runtime dependencies too.
    declare -A copied_ocr_libraries=()
    copy_ocr_dependencies() {
        local binary="$1"
        while read -r dependency; do
            [[ -z "${dependency}" || "${dependency}" == "linux-vdso*" ]] && continue
            local library_path="${dependency#*=> }"
            library_path="${library_path%% (*}"
            [[ "${library_path}" != /* || ! -f "${library_path}" ]] && continue
            local library_name
            library_name="$(basename "${library_path}")"
            case "${library_name}" in
                libc.so*|libm.so*|libpthread.so*|libdl.so*|librt.so*|libutil.so*|libresolv.so*|ld-linux*.so*)
                    continue
                    ;;
            esac
            [[ -n "${copied_ocr_libraries[${library_path}]:-}" ]] && continue
            copied_ocr_libraries["${library_path}"]=1
            install -Dm755 "${library_path}" "${app_directory}/usr/lib/${library_name}"
            copy_ocr_dependencies "${library_path}"
        done < <(ldd "${binary}" 2>/dev/null || true)
    }
    copy_ocr_dependencies "${app_directory}/usr/bin/ocr/tesseract"
else
    echo "Tesseract is required for the full Linux package" >&2
    exit 1
fi

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

linuxdeployqt_extra_args=()
if [[ "${LUMINA_ALLOW_NEW_GLIBC:-0}" == "1" ]]; then
    # linuxdeployqt protects the portable default by rejecting hosts newer
    # than Ubuntu 22.04. This opt-in is only for local validation on newer
    # distributions; CI still runs the default portable path on Ubuntu 22.04.
    linuxdeployqt_extra_args+=(-unsupported-allow-new-glibc)
fi
"${linuxdeployqt_path}" --appimage-extract-and-run \
    "${app_directory}/usr/share/applications/lumina-reader.desktop" \
    -appimage \
    -no-translations \
    -qmldir="${project_directory}/src/ui/qml" \
    "${linuxdeployqt_extra_args[@]}"

generated_appimage="$(find "${project_directory}" -maxdepth 1 -type f -name '*.AppImage' \
    ! -name 'linuxdeployqt.AppImage' -print -quit)"
if [[ -z "${generated_appimage}" ]]; then
    echo "linuxdeployqt did not produce an AppImage" >&2
    exit 1
fi

mv "${generated_appimage}" "${appimage_path}"
rm -f "${linuxdeployqt_path}"
echo "Created ${appimage_path}"
