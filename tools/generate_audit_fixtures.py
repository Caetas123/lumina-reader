#!/usr/bin/env python3
"""Generate legal, deterministic cross-format fixtures for a Lumina audit.

This script deliberately writes to a caller-provided temporary directory. It
does not use network resources and does not touch the user's library.
"""

from __future__ import annotations

import argparse
import io
import os
import tarfile
import zipfile
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont
from reportlab.lib.pagesizes import A4, landscape
from reportlab.lib.utils import ImageReader
from reportlab.pdfgen import canvas


TEXT = (
    "Lumina Reader audit fixture. Democracy, privacy, and reading are "
    "searchable in Portuguese, English, and Español. 日本語、한국어、中文.\n"
)


def write_text(path: Path, text: str, encoding: str, bom: bool = False) -> None:
    data = text.encode(encoding)
    if bom and encoding.lower().replace("-", "") in {"utf8", "utf"}:
        data = b"\xef\xbb\xbf" + data
    path.write_bytes(data)


def make_image(path: Path, label: str, size: tuple[int, int] = (960, 640)) -> None:
    image = Image.new("RGB", size, (17, 28, 56))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((28, 28, size[0] - 28, size[1] - 28), 24, fill=(25, 40, 76), outline=(139, 92, 246), width=4)
    draw.text((64, 72), "Lumina Reader", fill=(245, 247, 255))
    draw.text((64, 132), label, fill=(167, 123, 255))
    draw.text((64, 210), TEXT, fill=(232, 238, 255))
    draw.text((64, 260), "OCR democracy 123", fill=(104, 225, 194))
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)


def create_epub(out: Path, cover: Path) -> None:
    files: dict[str, bytes] = {
        "META-INF/container.xml": b'''<?xml version="1.0" encoding="UTF-8"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles><rootfile full-path="OEBPS/content.opf" media-type="application/oebps-package+xml"/></rootfiles>
</container>''',
        "OEBPS/chapter1.xhtml": ("""<?xml version="1.0" encoding="utf-8"?>
<html xmlns="http://www.w3.org/1999/xhtml"><head><title>Morning Light</title></head>
<body><h1>Chapter One</h1><p>The morning light makes every idea readable. Democracy and privacy matter.</p>
<p><a href="chapter2.xhtml">Continue to chapter two</a></p></body></html>""").encode("utf-8"),
        "OEBPS/chapter2.xhtml": ("""<?xml version="1.0" encoding="utf-8"?>
<html xmlns="http://www.w3.org/1999/xhtml"><head><title>Knowledge</title></head>
<body><h1>Chapter Two</h1><p>Knowledge grows when notes, highlights, and context stay together.</p></body></html>""").encode("utf-8"),
        "OEBPS/toc.ncx": ("""<?xml version="1.0" encoding="UTF-8"?>
<ncx xmlns="http://www.daisy.org/z3986/2005/ncx/"><navMap>
<navPoint id="one" playOrder="1"><navLabel><text>Chapter One</text></navLabel><content src="chapter1.xhtml"/></navPoint>
<navPoint id="two" playOrder="2"><navLabel><text>Chapter Two</text></navLabel><content src="chapter2.xhtml"/></navPoint>
</navMap></ncx>""").encode("utf-8"),
        "OEBPS/cover.png": cover.read_bytes(),
        "mimetype": b"application/epub+zip",
    }
    files["OEBPS/content.opf"] = ("""<?xml version="1.0" encoding="utf-8"?>
<package xmlns="http://www.idpf.org/2007/opf" unique-identifier="bookid" version="2.0">
<metadata xmlns:dc="http://purl.org/dc/elements/1.1/"><dc:identifier id="bookid">lumina-audit-epub</dc:identifier>
<dc:title>Morning Light</dc:title><dc:creator>Auditoria Lumina</dc:creator><dc:language>en</dc:language></metadata>
<manifest><item id="ch1" href="chapter1.xhtml" media-type="application/xhtml+xml"/><item id="ch2" href="chapter2.xhtml" media-type="application/xhtml+xml"/>
<item id="ncx" href="toc.ncx" media-type="application/x-dtbncx+xml"/><item id="cover" href="cover.png" media-type="image/png"/></manifest>
<spine toc="ncx"><itemref idref="ch1"/><itemref idref="ch2"/></spine></package>""").encode("utf-8")
    with zipfile.ZipFile(out, "w") as archive:
        archive.writestr("mimetype", files["mimetype"], compress_type=zipfile.ZIP_STORED)
        for name, data in files.items():
            if name != "mimetype":
                archive.writestr(name, data, compress_type=zipfile.ZIP_DEFLATED)


def create_pdf(out: Path, image: Path, scanned: bool = False, rotated: bool = False) -> None:
    page_size = landscape(A4) if rotated else A4
    pdf = canvas.Canvas(str(out), pagesize=page_size)
    width, height = page_size
    for page in range(1, 4):
        if scanned:
            pdf.drawImage(ImageReader(str(image)), 36, 80, width=width - 72, height=height - 140, preserveAspectRatio=True, anchor="c")
        else:
            pdf.setFillColorRGB(0.08, 0.13, 0.27)
            pdf.setFont("Helvetica-Bold", 20)
            pdf.drawString(48, height - 64, f"Lumina audit PDF page {page}")
            pdf.setFont("Helvetica", 12)
            pdf.setFillColorRGB(0, 0, 0)
            pdf.drawString(48, height - 98, TEXT)
            pdf.drawString(48, height - 122, "Search target: democracy, privacy, and knowledge.")
            pdf.drawString(width / 2 + 12, height - 98, "Second column: reading position persists.")
        pdf.showPage()
    pdf.save()


def archive_images(folder: Path) -> list[Path]:
    pages = []
    for index, name in enumerate(("01-cover.jpg", "02-página.png", "03-ocr.webp"), 1):
        target = folder / name
        make_image(target, f"Comic page {index}", (800, 1100))
        pages.append(target)
    return pages


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    root = args.output.resolve()
    root.mkdir(parents=True, exist_ok=True)

    write_text(root / "audit-utf8.txt", TEXT, "utf-8")
    write_text(root / "audit-utf8-bom.txt", TEXT, "utf-8", bom=True)
    write_text(root / "audit-utf16.txt", TEXT, "utf-16")
    write_text(root / "audit-long-lines.txt", (TEXT.replace("\n", " ") * 3000), "utf-8")
    (root / "audit.rtf").write_text(r"{\rtf1\ansi\deff0 {\fonttbl {\f0 Calibri;}}\f0\fs28 Lumina Reader audit: \b democracy\b0, privacy, and grifos com acentos: ação, coração.\par Chapter two.}", encoding="latin-1")
    (root / "audit.html").write_text("<html><head><script>alert('must not run')</script><style>.secret{display:none}</style></head><body><h1>Lumina audit</h1><p>Searchable democracy and privacy.</p></body></html>", encoding="utf-8")
    (root / "audit.htm").write_text("<html><body><h1>HTM fixture</h1><p>Knowledge center and annotation.</p></body></html>", encoding="utf-8")
    (root / "audit.fb2").write_text("""<?xml version="1.0" encoding="utf-8"?><FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0"><description><title-info><book-title>Morning Light</book-title><author><first-name>Lumina</first-name><last-name>Audit</last-name></author><lang>en</lang></title-info></description><body><section><title><p>Chapter One</p></title><p>Democracy, privacy, and knowledge are searchable.</p></section><section><title><p>Chapter Two</p></title><p>Notas com ação e coração.</p></section></body></FictionBook>""", encoding="utf-8")

    image = root / "audit-ocr.png"
    make_image(image, "OCR fixture")
    make_image(root / "audit.jpg", "JPEG fixture", (640, 480))
    (root / "audit.jpeg").write_bytes((root / "audit.jpg").read_bytes())
    make_image(root / "audit.webp", "WEBP fixture", (640, 480))
    make_image(root / "audit.avif", "AVIF fixture", (640, 480))
    frames = [Image.new("RGB", (320, 200), color) for color in ((30, 50, 90), (86, 55, 150), (22, 100, 110))]
    for index, frame in enumerate(frames):
        ImageDraw.Draw(frame).text((24, 80), f"GIF frame {index + 1}", fill="white")
    frames[0].save(root / "audit-animated.gif", save_all=True, append_images=frames[1:], duration=250, loop=0)
    frames[0].save(root / "audit-static.gif")

    create_epub(root / "audit.epub", image)
    create_pdf(root / "audit.pdf", image)
    create_pdf(root / "audit-scanned.pdf", image, scanned=True)
    create_pdf(root / "audit-rotated.pdf", image, rotated=True)
    (root / "audit-corrupt.pdf").write_bytes(b"%PDF-1.7\ncorrupt Lumina audit fixture\n")

    pages = archive_images(root / "comic-pages")
    for archive_name in ("audit.cbz", "audit.zip"):
        with zipfile.ZipFile(root / archive_name, "w", zipfile.ZIP_DEFLATED) as archive:
            for page in pages:
                archive.write(page, page.name)
    with tarfile.open(root / "audit.cbt", "w") as archive:
        for page in pages:
            archive.add(page, arcname=page.name)

    manifest = root / "FIXTURES-MANIFEST.txt"
    manifest.write_text("\n".join(sorted(path.name for path in root.rglob("*") if path.is_file())), encoding="utf-8")
    print(f"Generated fixtures in {root}")


if __name__ == "__main__":
    main()
