#include "infrastructure/archive/ArchiveReader.h"

#include <QRegularExpression>
#include <QTextDocument>

#include <algorithm>

#ifdef LUMINA_HAS_QT_ZIP
#include <QtCore/private/qzipreader_p.h>
#endif

namespace {

constexpr int kMaximumArchiveEntries = 4096;
constexpr qint64 kMaximumArchiveBytes = 512LL * 1024LL * 1024LL;

bool isImagePath(const QString &path)
{
    const QString suffix = path.section('.', -1).toLower();
    return suffix == QStringLiteral("jpg") || suffix == QStringLiteral("jpeg")
        || suffix == QStringLiteral("png") || suffix == QStringLiteral("webp")
        || suffix == QStringLiteral("gif") || suffix == QStringLiteral("avif");
}

QString normalizedArchivePath(QString path)
{
    path.replace(QChar('\\'), QChar('/'));
    if (path.startsWith(QChar('/')) || path.contains(QRegularExpression(QStringLiteral(R"(^[A-Za-z]:)")))) {
        return {};
    }
    const QStringList parts = path.split(QChar('/'), Qt::SkipEmptyParts);
    QStringList normalized;
    for (const QString &part : parts) {
        if (part == QStringLiteral(".")) {
            continue;
        }
        if (part == QStringLiteral("..")) {
            return {};
        }
        normalized.append(part);
    }
    return normalized.join(QChar('/'));
}

QString archiveDirectory(const QString &path)
{
    const qsizetype slash = path.lastIndexOf(QChar('/'));
    return slash < 0 ? QString() : path.left(slash);
}

QString archiveJoin(const QString &directory, const QString &path)
{
    return normalizedArchivePath(directory.isEmpty() ? path : directory + QChar('/') + path);
}

QString xmlAttribute(const QString &tag, const QString &name)
{
    const QRegularExpression expression(
        QStringLiteral(R"(\b%1\s*=\s*["']([^"']*)["'])").arg(name),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = expression.match(tag);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString decodeXmlEntities(QString value)
{
    value.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    value.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    value.replace(QStringLiteral("&apos;"), QStringLiteral("'"));
    value.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    value.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    return value;
}

QString htmlToText(const QByteArray &data)
{
    QString html = QString::fromUtf8(data);
    const QRegularExpression script(
        QStringLiteral(R"(<(script|style)\b[^>]*>.*?</\1\s*>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    html.remove(script);

    QTextDocument document;
    document.setHtml(html);
    return document.toPlainText().simplified();
}

} // namespace

namespace Lumina::Infrastructure {

ArchiveReader::ArchiveReader() = default;

ArchiveReader::~ArchiveReader() = default;

void ArchiveReader::clear()
{
#ifdef LUMINA_HAS_QT_ZIP
    zip_.reset();
#endif
    pages_.clear();
    format_.clear();
}

bool ArchiveReader::open(const QString &path, const QString &format)
{
    clear();
    format_ = format.toLower();

#ifdef LUMINA_HAS_QT_ZIP
    if (format_ != QStringLiteral("cbz") && format_ != QStringLiteral("zip")
        && format_ != QStringLiteral("epub")) {
        return false;
    }

    zip_ = std::make_unique<QZipReader>(path);
    if (!zip_->isReadable() || zip_->status() != QZipReader::NoError) {
        clear();
        return false;
    }

    const QList<QZipReader::FileInfo> entries = zip_->fileInfoList();
    if (entries.size() > kMaximumArchiveEntries) {
        clear();
        return false;
    }
    qint64 totalBytes = 0;
    for (const QZipReader::FileInfo &entry : entries) {
        if (entry.isSymLink || normalizedArchivePath(entry.filePath).isEmpty()
            || entry.size < 0 || entry.size > kMaximumArchiveBytes - totalBytes) {
            clear();
            return false;
        }
        totalBytes += entry.size;
    }

    const bool success = format_ == QStringLiteral("epub") ? openEpub() : openComic();
    if (!success) {
        clear();
    }
    return success;
#else
    Q_UNUSED(path)
    Q_UNUSED(format)
    return false;
#endif
}

bool ArchiveReader::isOpen() const
{
#ifdef LUMINA_HAS_QT_ZIP
    return zip_ != nullptr && !pages_.isEmpty();
#else
    return false;
#endif
}

int ArchiveReader::pageCount() const
{
    return pages_.size();
}

bool ArchiveReader::pageIsImage(int page) const
{
    return page >= 0 && page < pages_.size() && pages_.at(page).image;
}

QString ArchiveReader::pageName(int page) const
{
    return page >= 0 && page < pages_.size() ? pages_.at(page).name : QString();
}

QByteArray ArchiveReader::pageData(int page) const
{
#ifdef LUMINA_HAS_QT_ZIP
    if (zip_ == nullptr || page < 0 || page >= pages_.size()) {
        return {};
    }
    return zip_->fileData(pages_.at(page).name);
#else
    Q_UNUSED(page)
    return {};
#endif
}

QString ArchiveReader::pageText(int page) const
{
    if (!isOpen() || pageIsImage(page)) {
        return {};
    }
    return htmlToText(pageData(page));
}

bool ArchiveReader::openComic()
{
#ifdef LUMINA_HAS_QT_ZIP
    for (const QZipReader::FileInfo &entry : zip_->fileInfoList()) {
        if (entry.isFile && !entry.isSymLink && !normalizedArchivePath(entry.filePath).isEmpty()
            && isImagePath(entry.filePath)) {
            pages_.append(Page{entry.filePath, true});
        }
    }
    std::sort(pages_.begin(), pages_.end(), [](const Page &left, const Page &right) {
        return left.name.toLower() < right.name.toLower();
    });
    return !pages_.isEmpty();
#else
    return false;
#endif
}

bool ArchiveReader::openEpub()
{
#ifdef LUMINA_HAS_QT_ZIP
    const QByteArray container = zip_->fileData(QStringLiteral("META-INF/container.xml"));
    const QRegularExpression rootfileExpression(
        QStringLiteral(R"(<rootfile\b[^>]*\bfull-path\s*=\s*["']([^"']+)["'])"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch rootfile = rootfileExpression.match(QString::fromUtf8(container));
    if (!rootfile.hasMatch()) {
        return false;
    }

    const QString opfPath = normalizedArchivePath(decodeXmlEntities(rootfile.captured(1)));
    const QString opf = QString::fromUtf8(zip_->fileData(opfPath));
    if (opf.isEmpty()) {
        return false;
    }

    QHash<QString, QString> manifest;
    const QRegularExpression itemExpression(
        QStringLiteral(R"(<item\b[^>]*>)"),
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator itemIterator = itemExpression.globalMatch(opf);
    const QString opfDirectory = archiveDirectory(opfPath);
    while (itemIterator.hasNext()) {
        const QString tag = itemIterator.next().captured(0);
        const QString id = xmlAttribute(tag, QStringLiteral("id"));
        const QString href = decodeXmlEntities(xmlAttribute(tag, QStringLiteral("href")));
        const QString mediaType = xmlAttribute(tag, QStringLiteral("media-type")).toLower();
        if (!id.isEmpty() && !href.isEmpty()
            && (mediaType == QStringLiteral("application/xhtml+xml")
                || mediaType == QStringLiteral("text/html"))) {
            const QString path = archiveJoin(opfDirectory, href);
            if (!path.isEmpty()) {
                manifest.insert(id, path);
            }
        }
    }

    const QRegularExpression itemrefExpression(
        QStringLiteral(R"(<itemref\b[^>]*\bidref\s*=\s*["']([^"']+)["'])"),
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator itemrefIterator = itemrefExpression.globalMatch(opf);
    while (itemrefIterator.hasNext()) {
        const QString id = itemrefIterator.next().captured(1);
        const QString path = manifest.value(id);
        if (!path.isEmpty()) {
            pages_.append(Page{path, false});
        }
    }

    if (pages_.isEmpty()) {
        for (auto iterator = manifest.cbegin(); iterator != manifest.cend(); ++iterator) {
            pages_.append(Page{iterator.value(), false});
        }
        std::sort(pages_.begin(), pages_.end(), [](const Page &left, const Page &right) {
            return left.name.toLower() < right.name.toLower();
        });
    }
    return !pages_.isEmpty();
#else
    return false;
#endif
}

} // namespace Lumina::Infrastructure
