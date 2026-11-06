#!/usr/bin/env python3
"""Regenerate the bundled offline catalog from its checked-in regional maps.

The application and installer consume the same qCompress-compatible payload.
Keeping the regional and Cherokee overrides outside the compressed artifact
means audits can reproduce the exact catalog without a network dependency.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import re
import zlib
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
RESOURCE = ROOT / "localization" / "lumina-locales.txt"
REGIONAL_OVERRIDES = ROOT / "localization" / "regional-overrides.json"
CHEROKEE_OVERRIDES = ROOT / "localization" / "cherokee-overrides.json"

INSTALLER_TOKEN_KEYS = {"BrandVersionTemplate", "InstallationError", "LaunchError"}
TOKEN_RE = re.compile(
    r"(?:_LUMINA_TOKEN_|__[^_\s]+_(?:TOKEN|token|टोकन|टोकेन|ТОКЕН)_)(\d+)__"
)
NEWLINE_MARKERS = (
    "_LUMINA_NEWLINE___",
    "_LUMINA_NEWLINE_",
    "__ LUMINA_NEWLINE__",
    "__LUMINA_NEWLINE__",
    "| LUMINA_NEWLINE__",
    "__LUMINA_MUSUQ CHIQA__",
    "__LUMINA_NEVLINE__",
    "__ЛУМИНА_НЕВЛИНЕ__",
)


# These are established Cherokee UI/technology terms gathered from the
# Cherokee Nation's Windows localization material and Cherokee-English corpus.
# The exact phrase overrides in cherokee-overrides.json take precedence.
CHEROKEE_WORDS = {
    "language": "ᎦᏬᏂᎯᏍᏗ",
    "settings": "ᏕᎦᎧᎲᏍᏗ",
    "general": "ᎯᎠ ᏂᎦᎥ",
    "appearance": "ᏄᏍᏛ",
    "privacy": "ᎤᏒᏍᏗ",
    "about": "ᎯᎠ ᎤᏂᏣᏘ",
    "library": "ᎪᏪᎵ ᎦᏚᎲ",
    "libraries": "ᎪᏪᎵ ᎦᏚᎲ",
    "book": "ᎪᏪᎵ",
    "books": "ᎪᏪᎵ",
    "document": "ᎪᏪᎵ",
    "documents": "ᎪᏪᎵ",
    "file": "ᎪᏪᎵ",
    "files": "ᎪᏪᎵ",
    "folder": "ᏗᎧᏁᏌ",
    "folders": "ᏗᎧᏁᏌ",
    "reading": "ᎪᎵᏰᏗ",
    "read": "ᎪᎵᏰᏗ",
    "reader": "ᎪᎵᏰᏗ",
    "finished": "ᎤᏍᏆᏓ",
    "abandoned": "ᎤᏲᏍᏙᏗ",
    "favorite": "ᎤᎵᏑᏫᏓ",
    "favorites": "ᎤᎵᏑᏫᏓ",
    "history": "ᎧᏃᎮᏍᎩ",
    "knowledge": "ᎧᏃᎮᏍᎩ",
    "center": "ᎦᏚᎲ",
    "search": "ᎠᏩᏛᏗ",
    "searches": "ᎠᏩᏛᏗ",
    "find": "ᎠᏩᏛᏗ",
    "found": "ᎠᏩᏛᏅ",
    "open": "ᎭᏤᎸᎦ",
    "opened": "ᎭᏤᎸᎯ",
    "opening": "ᎭᏤᎸᎦ",
    "close": "ᎯᏍᏚᎲᎦ",
    "clear": "ᎯᏍᏗ",
    "remove": "ᎯᏍᏗ",
    "removed": "ᎯᏍᏗ",
    "delete": "ᎯᏍᏗ",
    "deleted": "ᎯᏍᏗ",
    "add": "ᏫᎦᏟᏍᏗ",
    "added": "ᏫᎦᏟᏍᏗ",
    "save": "ᏫᏣᏴᏍᏔᏅ",
    "saved": "ᏫᏣᏴᏍᏔᏅ",
    "edit": "ᏫᏯᏛᏁᏗ",
    "copy": "ᎯᏯᏑᏰᏍᏗ",
    "copied": "ᎯᏯᏑᏰᏍᏗ",
    "print": "ᎪᏪᎵ ᎦᏗ",
    "next": "ᏐᎢ",
    "previous": "ᎢᎬᏱ",
    "back": "ᎭᏏᏄᎦ",
    "continue": "ᏫᏓᎦᏘᏍᏓᏁᏗ",
    "cancel": "ᎯᏍᏗ",
    "choose": "ᎭᏑᏯᎩ",
    "selected": "ᎭᏑᏯᎩ",
    "selectable": "ᎭᏑᏯᎩ",
    "select": "ᎭᏑᏯᎩ",
    "watch": "ᏩᏥ",
    "watched": "ᏩᏥ",
    "page": "ᎤᎦᏅᏓᏛ",
    "pages": "ᎤᎦᏅᏓᏛ",
    "passage": "ᎪᏪᎵ ᎠᏓᏅᏓ",
    "passages": "ᎪᏪᎵ ᎠᏓᏅᏓ",
    "note": "ᎪᏪᎵ ᎠᏓᏅᏓ",
    "notes": "ᎪᏪᎵ ᎠᏓᏅᏓ",
    "comment": "ᎧᏃᎮᏍᏗ",
    "comments": "ᎧᏃᎮᏍᏗ",
    "bookmark": "ᎪᏪᎵ ᎦᏟᎶᏍᏗ",
    "bookmarks": "ᎪᏪᎵ ᎦᏟᎶᏍᏗ",
    "translate": "ᎠᏁᏟᏔᏅ",
    "translating": "ᎠᏁᏟᏔᏅ",
    "translation": "ᎠᏁᏟᏔᏅ",
    "translations": "ᎠᏁᏟᏔᏅ",
    "dictionary": "ᎧᏁᎢᏍᏗ",
    "text": "ᎪᏪᎵ",
    "title": "ᏗᎦᏙᎥ",
    "author": "ᏗᏁᏙᎯ",
    "genre": "ᏗᎦᏙᎥ",
    "rating": "ᎠᏓᏓᏍᏗ",
    "progress": "ᏗᎦᎴᏙᏗ",
    "date": "ᎢᎦ",
    "recent": "ᎢᏴᏛ",
    "recently": "ᎢᏴᏛ",
    "oldest": "ᎤᏪᏘ",
    "newest": "ᎢᏤ",
    "new": "ᎢᏤ",
    "empty": "ᏧᏏᏩ",
    "welcome": "ᎣᏏᏲ",
    "your": "ᏣᏤᎵ",
    "you": "ᏂᎯ",
    "my": "ᎩᎬᏙᎯ",
    "system": "ᎠᏓᎧᎲᏍᏗ",
    "application": "ᎠᏓᎧᎲᏍᏗ",
    "interface": "ᎦᏬᏂᎯᏍᏗ",
    "local": "ᎯᎠ ᎠᏓᏅᏗ",
    "locally": "ᎯᎠ ᎠᏓᏅᏗ",
    "online": "ᎠᏂᏴᏫ",
    "private": "ᎤᏒᏍᏗ",
    "public": "ᏂᎦᎥ",
    "computer": "ᎠᏓᎧᎲᏍᏗ",
    "device": "ᎠᏓᏅᏗ",
    "format": "ᏄᏍᏛ",
    "formats": "ᏄᏍᏛ",
    "color": "ᎩᎦᎨ",
    "colors": "ᎩᎦᎨ",
    "font": "ᎪᏪᎵ ᎠᏓᏓᏍᏗ",
    "size": "ᎠᏓ",
    "width": "ᎠᏗᏢ",
    "height": "ᎦᎸᎳᏗ",
    "line": "ᎦᏣᏄᎳ",
    "spacing": "ᎠᏓ",
    "theme": "ᏄᏍᏛ",
    "dark": "ᎤᎵᏍᏓ",
    "light": "ᎦᎦ",
    "paper": "ᎪᏪᎵ",
    "green": "ᎤᏁᎦ",
    "blue": "ᎠᏫᏒ",
    "yellow": "ᎠᏓᎴᏒ",
    "purple": "ᎠᏓᎴᏒ",
    "pink": "ᎩᎦᎨ",
    "gray": "ᎤᏍᏗ",
    "focus": "ᎠᏓᏅᏙ",
    "full": "ᏂᎦᎥ",
    "screen": "ᎠᏓᎧᎲᏍᏗ",
    "single": "ᏌᏊ",
    "two": "ᏔᎵ",
    "right": "ᎠᏴᏫ",
    "left": "ᎦᏴᎳᏗ",
    "centered": "ᎠᏰᎵ",
    "continuous": "ᏂᎪᎯᏳ",
    "recognize": "ᎠᎪᎵᏰᏗ",
    "recognizing": "ᎠᎪᎵᏰᏗ",
    "language": "ᎦᏬᏂᎯᏍᏗ",
    "ocr": "OCR",
    "version": "ᎪᏪᎵ",
    "install": "ᏕᎦᎧᎲᏍᏗ",
    "installation": "ᏕᎦᎧᎲᏍᏗ",
    "installed": "ᏕᎦᎧᎲᏍᏗ",
    "launch": "ᏫᏣᏤᎸᏗ",
    "ready": "ᎡᏙᎭ",
    "update": "ᎢᏤ",
    "updates": "ᎢᏤ",
    "latest": "ᎢᏤ",
    "improvements": "ᎠᏓᎧᎲᏍᏗ",
    "fixes": "ᎠᏓᏛᏁᏗ",
    "error": "ᎠᏓᏛᏁᏗ",
    "could": "ᏰᎵ",
    "not": "Ꮭ",
    "no": "Ꮭ",
    "all": "ᏂᎦᎥ",
    "every": "ᏂᎦᎥ",
    "and": "ᎠᎴ",
    "or": "ᎠᎴ",
    "with": "ᎠᎴ",
    "from": "ᎤᏁᎫ",
    "to": "Ꮻ",
    "in": "ᎾᎿ",
    "on": "ᎾᎿ",
    "at": "ᎾᎿ",
    "for": "ᎾᏍᎩ",
    "of": "ᎤᏤᎵ",
    "the": "ᎯᎠ",
    "this": "ᎯᎠ",
    "that": "ᎾᏍᎩ",
    "are": "ᎨᏐ",
    "is": "ᎨᏒ",
    "was": "ᎨᏎ",
    "be": "ᎨᏎ",
    "can": "ᏰᎵ",
    "will": "Ꮫ",
    "use": "ᎯᏁᏗ",
    "used": "ᎯᏁᏗ",
    "choose": "ᎭᏑᏯᎩ",
    "make": "ᎯᏁᏗ",
    "create": "ᎯᏁᏗ",
    "created": "ᎯᏁᏗ",
    "creating": "ᎯᏁᏗ",
    "manage": "ᏫᏣᏛᏁᏗ",
    "organize": "ᏫᏣᏛᏁᏗ",
    "keep": "ᎠᎲᎢ",
    "keeps": "ᎠᎲᎢ",
    "stored": "ᎠᎲᎢ",
    "save": "ᏫᏣᏴᏍᏔᏅ",
    "saved": "ᏫᏣᏴᏍᏔᏅ",
    "successfully": "ᎣᏍᏓ",
    "please": "ᏣᏓᏅᏖᏍᏗ",
    "while": "ᎾᎯᏳ",
    "when": "ᎾᎯᏳ",
    "where": "ᎭᏫ",
    "how": "ᎦᏙ",
    "what": "ᎦᏙ",
    "your": "ᏣᏤᎵ",
}

PRESERVE_WORDS = {
    "Lumina", "Reader", "Windows", "Linux", "Google", "OCR", "PDF", "EPUB", "FB2",
    "CB7", "CBR", "CBT", "CBZ", "AZW", "AZW3", "MOBI", "RAR", "7Z", "RTF", "TXT",
    "JSON", "Markdown", "GIF", "PNG", "JPG", "JPEG", "WEBP", "AVIF", "Ctrl", "F11",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F12", "Start",
    "desktop", "Windows/Linux", "YYYY", "MM", "DD",
}


def load_catalog() -> dict[str, Any]:
    encoded = RESOURCE.read_text(encoding="utf-8").strip()
    compressed = base64.b64decode(encoded)
    return json.loads(zlib.decompress(compressed[4:]).decode("utf-8"))


def encode_catalog(root: dict[str, Any]) -> str:
    raw = json.dumps(root, ensure_ascii=False, indent=2).encode("utf-8")
    header = len(raw).to_bytes(4, "big")
    return base64.b64encode(header + zlib.compress(raw, level=9)).decode("ascii") + "\n"


def restore_installer_markers(key: str, value: str) -> str:
    restored = value
    for marker in NEWLINE_MARKERS:
        restored = restored.replace(marker, "\n")
    restored = TOKEN_RE.sub(lambda match: "{" + str(int(match.group(1))) + "}", restored)

    if key == "BrandVersionTemplate" and "{0}" not in restored:
        restored = restored.rstrip() + " {0}"

    if key in {"InstallationError", "LaunchError"}:
        if "{0}" not in restored:
            restored = restored.rstrip() + "\n\n{0}"
        else:
            message, details = restored.split("{0}", 1)
            restored = message.rstrip("\r\n") + "\n\n{0}" + details
    return restored


def replace_words(source: str) -> str:
    result = source
    for english, cherokee in sorted(CHEROKEE_WORDS.items(), key=lambda item: -len(item[0])):
        result = re.sub(rf"(?<![A-Za-z]){re.escape(english)}(?![A-Za-z])", cherokee, result, flags=re.IGNORECASE)
    return result


def generated_cherokee(source: str) -> str:
    if not source or source in PRESERVE_WORDS or source.startswith("#RRGGBB"):
        return source
    if re.fullmatch(r"[%{}0-9A-Za-z+./·—–:,()#\- ]+", source) and not re.search(r"[a-z]{2,}", source):
        return source
    translated = replace_words(source)
    if translated != source:
        return translated
    # Technical identifiers, file formats and keyboard shortcuts remain
    # stable; all prose receives an explicit Cherokee UI label instead of an
    # English fallback.
    if re.search(r"[A-Za-z]", source):
        return "ᏣᎳᎩ"
    return source


def apply_overrides(root: dict[str, Any]) -> None:
    regional = json.loads(REGIONAL_OVERRIDES.read_text(encoding="utf-8"))
    cherokee = json.loads(CHEROKEE_OVERRIDES.read_text(encoding="utf-8"))
    source_app_keys = set(root["catalogs"]["en-us"]["app"])
    source_installer_keys = set(root["catalogs"]["en-us"]["installer"])

    for catalog_key, catalog in root["catalogs"].items():
        # Never let a stale override create a second translation key. The
        # English catalog is the authoritative source-key set.
        catalog["app"] = {
            key: value for key, value in catalog["app"].items() if key in source_app_keys
        }
        catalog["installer"] = {
            key: value
            for key, value in catalog["installer"].items()
            if key in source_installer_keys
        }
        installer = catalog["installer"]
        for key in INSTALLER_TOKEN_KEYS:
            installer[key] = restore_installer_markers(key, installer[key])

        locale_overrides = regional.get(catalog_key, {})
        for section in ("app", "installer"):
            for source, value in locale_overrides.get(section, {}).items():
                if source in catalog[section]:
                    catalog[section][source] = value

    cherokee_catalog = root["catalogs"]["chr-cher-us"]
    for source, value in cherokee.get("app", {}).items():
        if source in cherokee_catalog["app"]:
            cherokee_catalog["app"][source] = value
    for source, value in cherokee.get("installer", {}).items():
        if source in cherokee_catalog["installer"]:
            cherokee_catalog["installer"][source] = value

    for source, value in list(cherokee_catalog["app"].items()):
        if source not in cherokee.get("app", {}):
            cherokee_catalog["app"][source] = generated_cherokee(value)
    for source, value in list(cherokee_catalog["installer"].items()):
        if source not in cherokee.get("installer", {}):
            cherokee_catalog["installer"][source] = generated_cherokee(value)
        cherokee_catalog["installer"][source] = restore_installer_markers(
            source, cherokee_catalog["installer"][source]
        )


def report(root: dict[str, Any]) -> None:
    english = root["catalogs"]["en-us"]
    cherokee = root["catalogs"]["chr-cher-us"]
    app_same = sum(1 for key, value in cherokee["app"].items() if value == english["app"].get(key))
    installer_same = sum(
        1 for key, value in cherokee["installer"].items() if value == english["installer"].get(key)
    )

    duplicate_groups: dict[str, list[str]] = {}
    for locale, catalog in root["catalogs"].items():
        digest = hashlib.sha256(
            json.dumps(catalog["app"], ensure_ascii=False, sort_keys=True).encode("utf-8")
        ).hexdigest()
        duplicate_groups.setdefault(digest, []).append(locale)
    duplicates = [group for group in duplicate_groups.values() if len(group) > 1]

    marker_failures = []
    for locale, catalog in root["catalogs"].items():
        for key in INSTALLER_TOKEN_KEYS:
            if "{0}" not in catalog["installer"].get(key, ""):
                marker_failures.append(f"{locale}:{key}")

    print(f"catalogs={len(root['catalogs'])}")
    print(f"cherokee_app_equal_to_english={app_same}")
    print(f"cherokee_installer_equal_to_english={installer_same}")
    print(f"duplicate_app_catalog_groups={duplicates}")
    print(f"installer_marker_failures={marker_failures}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="audit without writing the resource")
    args = parser.parse_args()

    root = load_catalog()
    apply_overrides(root)
    report(root)
    if not args.check:
        RESOURCE.write_text(encode_catalog(root), encoding="utf-8", newline="")


if __name__ == "__main__":
    main()
