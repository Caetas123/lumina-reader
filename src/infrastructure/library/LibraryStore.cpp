#include "infrastructure/library/LibraryStore.h"

#include "infrastructure/database/Database.h"
#include "infrastructure/archive/ArchiveReader.h"
#include "infrastructure/logging/Logging.h"
#include "infrastructure/text/TextDecoder.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QImageReader>
#include <QImage>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

#ifdef LUMINA_HAS_QT_PDF
#include <QPdfDocument>
#endif

#include <algorithm>

namespace {

QString normalizedPath(const QString &path)
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? info.absoluteFilePath() : canonical;
}

QString formatForPath(const QString &path)
{
    return QFileInfo(path).suffix().toLower();
}

QStringList supportedFormats()
{
    QStringList formats{
        QStringLiteral("mobi"), QStringLiteral("azw"), QStringLiteral("azw3"),
        QStringLiteral("fb2"), QStringLiteral("txt"), QStringLiteral("rtf"),
        QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("png"),
        QStringLiteral("webp"), QStringLiteral("avif"), QStringLiteral("gif"),
    };
#ifdef LUMINA_HAS_QT_PDF
    formats.append(QStringLiteral("pdf"));
#endif
#ifdef LUMINA_HAS_QT_ZIP
    formats << QStringLiteral("epub") << QStringLiteral("cbz") << QStringLiteral("zip");
#endif
#ifdef LUMINA_HAS_LIBARCHIVE
    formats << QStringLiteral("cbr") << QStringLiteral("cb7") << QStringLiteral("cbt")
            << QStringLiteral("rar") << QStringLiteral("7z");
#endif
    return formats;
}

bool isImageFormat(const QString &format)
{
    return format == QStringLiteral("jpg") || format == QStringLiteral("jpeg")
        || format == QStringLiteral("png") || format == QStringLiteral("webp")
        || format == QStringLiteral("avif") || format == QStringLiteral("gif");
}

bool isComicFormat(const QString &format)
{
    return format == QStringLiteral("cbz") || format == QStringLiteral("cbr")
        || format == QStringLiteral("cb7") || format == QStringLiteral("cbt")
        || format == QStringLiteral("zip") || format == QStringLiteral("rar")
        || format == QStringLiteral("7z");
}

QString contentTypeForFormat(const QString &format)
{
    if (isImageFormat(format)) {
        return QStringLiteral("image");
    }
    if (isComicFormat(format)) {
        return QStringLiteral("comic");
    }
    if (format == QStringLiteral("epub") || format == QStringLiteral("mobi")
        || format == QStringLiteral("azw") || format == QStringLiteral("azw3")) {
        return QStringLiteral("book");
    }
    if (format == QStringLiteral("pdf") || format == QStringLiteral("djvu")
        || format == QStringLiteral("djv")) {
        return QStringLiteral("document");
    }
    return QStringLiteral("text");
}

bool isValidContentType(const QString &contentType)
{
    static const QStringList types{
        QStringLiteral("book"), QStringLiteral("manga"), QStringLiteral("manhwa"),
        QStringLiteral("webtoon"), QStringLiteral("newspaper"), QStringLiteral("magazine"),
        QStringLiteral("article"), QStringLiteral("academic"), QStringLiteral("handout"),
        QStringLiteral("comic"), QStringLiteral("document"), QStringLiteral("image"),
        QStringLiteral("text"), QStringLiteral("other"),
    };
    return types.contains(contentType);
}

bool hasSupportedFormat(const QString &format)
{
    return supportedFormats().contains(format);
}

bool hasExpectedSignature(const QString &path, const QString &format)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray header = file.read(12);
    if (isImageFormat(format)) {
        QImageReader reader(path);
        return reader.canRead();
    }
    if (format == QStringLiteral("pdf")) {
        return header.startsWith("%PDF-");
    }
    if (format == QStringLiteral("mobi") || format == QStringLiteral("azw")
        || format == QStringLiteral("azw3")) {
        if (file.size() < 86 || !file.seek(76)) {
            return false;
        }
        const QByteArray palm = file.read(10);
        if (palm.size() < 8) {
            return false;
        }
        const quint16 records = (static_cast<quint16>(static_cast<uchar>(palm.at(0))) << 8)
            | static_cast<quint16>(static_cast<uchar>(palm.at(1)));
        const quint32 firstRecord = (static_cast<quint32>(static_cast<uchar>(palm.at(2))) << 24)
            | (static_cast<quint32>(static_cast<uchar>(palm.at(3))) << 16)
            | (static_cast<quint32>(static_cast<uchar>(palm.at(4))) << 8)
            | static_cast<quint32>(static_cast<uchar>(palm.at(5)));
        return records > 0 && firstRecord >= 86 && firstRecord < static_cast<quint32>(file.size());
    }
    if (format == QStringLiteral("rar") || format == QStringLiteral("cbr")) {
        return header.startsWith("Rar!\x1A\x07");
    }
    if (format == QStringLiteral("7z") || format == QStringLiteral("cb7")) {
        return header.startsWith("7z\xBC\xAF\x27\x1C");
    }
    if (format == QStringLiteral("cbt")) {
        if (!file.seek(257)) {
            return false;
        }
        return file.read(5) == QByteArrayLiteral("ustar");
    }
    if (format == QStringLiteral("epub") || format == QStringLiteral("cbz")
        || format == QStringLiteral("zip")) {
        return header.startsWith("PK\x03\x04") || header.startsWith("PK\x05\x06")
            || header.startsWith("PK\x07\x08");
    }
    return !header.isEmpty();
}

QString calculateHash(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        hash.addData(file.read(1024 * 1024));
    }
    return QString::fromLatin1(hash.result().toHex());
}

QString searchableText(const QString &path, const QString &format)
{
#ifdef LUMINA_HAS_QT_PDF
    if (format == QStringLiteral("pdf")) {
        QPdfDocument document;
        if (document.load(path) != QPdfDocument::Error::None) {
            return {};
        }
        QString text;
        for (int page = 0; page < document.pageCount(); ++page) {
            const auto selection = document.getAllText(page);
            if (selection.isValid()) {
                text.append(selection.text());
                text.append(QChar('\n'));
            }
            if (text.size() >= 4 * 1024 * 1024) {
                break;
            }
        }
        return text.left(4 * 1024 * 1024);
    }
#endif
    if (format == QStringLiteral("epub")) {
        Lumina::Infrastructure::ArchiveReader archive;
        if (!archive.open(path, format)) {
            return {};
        }

        QString text;
        for (int page = 0; page < archive.pageCount(); ++page) {
            text.append(archive.pageText(page));
            text.append(QChar('\n'));
            if (text.size() >= 4 * 1024 * 1024) {
                break;
            }
        }
        return text.left(4 * 1024 * 1024);
    }

    if (format != QStringLiteral("txt") && format != QStringLiteral("rtf")
        && format != QStringLiteral("fb2") && format != QStringLiteral("mobi")
        && format != QStringLiteral("azw") && format != QStringLiteral("azw3")) {
        return {};
    }

    return Lumina::Infrastructure::TextDecoder::readFile(path, format).left(4 * 1024 * 1024);
}

QString ftsPrefixQuery(const QString &value)
{
    QStringList tokens;
    const QRegularExpression tokenExpression(QStringLiteral(R"([\p{L}\p{N}_]+)"));
    QRegularExpressionMatchIterator iterator = tokenExpression.globalMatch(value);
    while (iterator.hasNext()) {
        QString token = iterator.next().captured(0);
        token.replace(QStringLiteral("\""), QStringLiteral("\"\""));
        tokens.append(QStringLiteral("\"") + token + QStringLiteral("\"*"));
    }
    return tokens.join(QStringLiteral(" AND "));
}

bool isValidViewFilter(const QString &filter)
{
    return filter.isEmpty() || filter == QStringLiteral("all")
        || filter == QStringLiteral("favorites") || filter == QStringLiteral("to_read")
        || filter == QStringLiteral("reading") || filter == QStringLiteral("read")
        || filter == QStringLiteral("abandoned") || filter == QStringLiteral("recent")
        || (filter.startsWith(QStringLiteral("collection:")) && filter.size() > 11)
        || (filter.startsWith(QStringLiteral("tag:")) && filter.size() > 4)
        || (filter.startsWith(QStringLiteral("type:")) && filter.size() > 5);
}

QString viewFilterPredicate(const QString &filter)
{
    if (filter == QStringLiteral("favorites")) {
        return QStringLiteral("d.is_favorite = 1");
    }
    if (filter == QStringLiteral("to_read") || filter == QStringLiteral("reading")
        || filter == QStringLiteral("read") || filter == QStringLiteral("abandoned")) {
        return QStringLiteral("d.status = '%1'").arg(filter);
    }
    if (filter == QStringLiteral("recent")) {
        return QStringLiteral("COALESCE(d.last_opened_at, d.added_at) >= datetime('now', '-30 days')");
    }
    return {};
}

void setError(Lumina::Domain::Error *error,
              Lumina::Domain::ErrorCode code,
              const QString &userMessage,
              const QString &technicalMessage)
{
    if (error != nullptr) {
        *error = Lumina::Domain::Error::create(code, userMessage, technicalMessage);
    }
}

QString generatedCoverPath(const QString &path, const QString &format, const QString &hash)
{
    QImage image;
    if (isImageFormat(format)) {
        image.load(path);
    }
#ifdef LUMINA_HAS_QT_PDF
    else if (format == QStringLiteral("pdf")) {
        QPdfDocument document;
        if (document.load(path) == QPdfDocument::Error::None && document.pageCount() > 0) {
            image = document.render(0, QSize(640, 900));
        }
    }
#endif
    else if (format == QStringLiteral("cbz") || format == QStringLiteral("zip")
             || format == QStringLiteral("cbr") || format == QStringLiteral("cb7")
             || format == QStringLiteral("cbt") || format == QStringLiteral("rar")
             || format == QStringLiteral("7z")) {
        Lumina::Infrastructure::ArchiveReader archive;
        if (archive.open(path, format) && archive.pageCount() > 0 && archive.pageIsImage(0)) {
            image = QImage::fromData(archive.pageData(0));
        }
    }

    if (image.isNull() || image.width() <= 0 || image.height() <= 0
        || image.width() > 20000 || image.height() > 20000) {
        return QStringLiteral("");
    }

    const QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cachePath.isEmpty()) {
        return QStringLiteral("");
    }
    QDir cacheDirectory(cachePath);
    if (!cacheDirectory.mkpath(QStringLiteral("covers"))) {
        return QStringLiteral("");
    }
    const QString outputPath = cacheDirectory.filePath(
        QStringLiteral("covers/") + hash + QStringLiteral(".png"));
    const QImage cover = image.scaled(QSize(480, 680),
                                      Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation);
    return cover.save(outputPath, "PNG") ? outputPath : QStringLiteral("");
}

} // namespace

namespace Lumina::Infrastructure {

LibraryStore::LibraryStore(QObject *parent)
    : QAbstractListModel(parent)
{
    folderWatcher_ = new QFileSystemWatcher(this);
    connect(folderWatcher_, &QFileSystemWatcher::directoryChanged, this, [this](const QString &) {
        for (const QString &root : watchedFolders_) {
            if (QDir(root).exists()) {
                importFolder(root, true);
            }
        }
    });
}

int LibraryStore::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : records_.size();
}

QVariant LibraryStore::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= records_.size()) {
        return {};
    }

    const QVariantMap &record = records_.at(index.row());
    const QByteArray roleName = roleNames().value(role);
    return record.value(QString::fromLatin1(roleName));
}

QHash<int, QByteArray> LibraryStore::roleNames() const
{
    return {
        {IdRole, "id"},
        {TitleRole, "title"},
        {AuthorRole, "author"},
        {FilePathRole, "filePath"},
        {FormatRole, "format"},
        {ContentTypeRole, "contentType"},
        {StatusRole, "status"},
        {ProgressRole, "progress"},
        {CurrentPageRole, "currentPage"},
        {PageCountRole, "pageCount"},
        {CoverPathRole, "coverPath"},
        {FileSizeRole, "fileSize"},
        {AddedAtRole, "addedAt"},
        {LastOpenedAtRole, "lastOpenedAt"},
        {DescriptionRole, "description"},
        {LanguageRole, "language"},
        {FavoriteRole, "isFavorite"},
        {RatingRole, "rating"},
        {PageRotationRole, "pageRotation"},
        {ReadingSecondsRole, "readingSeconds"},
    };
}

QString LibraryStore::query() const
{
    return query_;
}

void LibraryStore::setQuery(const QString &query)
{
    const QString trimmed = query.trimmed();
    if (trimmed == query_) {
        return;
    }
    query_ = trimmed;
    emit queryChanged();
    reload();
}

QString LibraryStore::viewFilter() const
{
    return viewFilter_.isEmpty() ? QStringLiteral("all") : viewFilter_;
}

void LibraryStore::setViewFilter(const QString &filter)
{
    const QString normalized = filter.trimmed().toLower();
    if (!isValidViewFilter(normalized) || normalized == viewFilter()) {
        return;
    }
    viewFilter_ = normalized == QStringLiteral("all") ? QString() : normalized;
    emit viewFilterChanged();
    reload();
}

int LibraryStore::count() const
{
    return records_.size();
}

void LibraryStore::setDatabase(Database *database)
{
    if (database_ == database) {
        return;
    }
    database_ = database;
    reload();
}

void LibraryStore::refresh()
{
    reload();
}

bool LibraryStore::isReady() const
{
    return database_ != nullptr && database_->isOpen();
}

void LibraryStore::setError(const QString &message)
{
    qCWarning(luminaDatabase) << message;
    emit errorOccurred(message);
}

void LibraryStore::reload()
{
    beginResetModel();
    records_.clear();

    if (!isReady()) {
        endResetModel();
        emit countChanged();
        return;
    }

    QSqlQuery query(database_->connection());
    const QString columns = QStringLiteral(
        "d.id, d.title, d.author, d.file_path, d.format, d.content_type, d.status, "
        "d.progress, d.current_page, d.page_count, d.cover_path, d.file_size, "
        "d.added_at, COALESCE(d.last_opened_at, ''), d.description, d.language, "
        "d.is_favorite, d.rating, d.page_rotation, d.reading_seconds");
    QStringList predicates;
    const QString filterPredicate = viewFilterPredicate(viewFilter_);
    if (!filterPredicate.isEmpty()) {
        predicates.append(filterPredicate);
    }
    const QString normalizedFilter = viewFilter();
    QString dynamicFilterPredicate;
    if (normalizedFilter.startsWith(QStringLiteral("collection:"))) {
        dynamicFilterPredicate = QStringLiteral(
            "EXISTS (SELECT 1 FROM document_collections dc "
            "WHERE dc.document_id = d.id AND dc.collection_id = :collection_filter)");
    } else if (normalizedFilter.startsWith(QStringLiteral("tag:"))) {
        dynamicFilterPredicate = QStringLiteral(
            "EXISTS (SELECT 1 FROM document_tags dt JOIN tags t ON t.id = dt.tag_id "
            "WHERE dt.document_id = d.id AND lower(t.name) = lower(:tag_filter))");
    } else if (normalizedFilter.startsWith(QStringLiteral("type:"))) {
        dynamicFilterPredicate = QStringLiteral("d.content_type = :type_filter");
    }
    if (!dynamicFilterPredicate.isEmpty()) {
        predicates.append(dynamicFilterPredicate);
    }
    if (!query_.isEmpty()) {
        predicates.append(QStringLiteral("document_search MATCH :match"));
    }

    QString statement = QStringLiteral("SELECT %1 FROM documents d ").arg(columns);
    if (!query_.isEmpty()) {
        statement += QStringLiteral("JOIN document_search ON document_search.document_id = d.id ");
    }
    if (!predicates.isEmpty()) {
        statement += QStringLiteral("WHERE ") + predicates.join(QStringLiteral(" AND ")) + QChar(' ');
    }
    statement += query_.isEmpty()
        ? QStringLiteral("ORDER BY COALESCE(d.last_opened_at, d.added_at) DESC, d.title COLLATE NOCASE")
        : QStringLiteral("ORDER BY rank");
    query.prepare(statement);
    if (!query_.isEmpty()) {
        query.bindValue(QStringLiteral(":match"), ftsPrefixQuery(query_));
    }
    if (normalizedFilter.startsWith(QStringLiteral("collection:"))) {
        query.bindValue(QStringLiteral(":collection_filter"), normalizedFilter.mid(11));
    } else if (normalizedFilter.startsWith(QStringLiteral("tag:"))) {
        query.bindValue(QStringLiteral(":tag_filter"), normalizedFilter.mid(4));
    } else if (normalizedFilter.startsWith(QStringLiteral("type:"))) {
        query.bindValue(QStringLiteral(":type_filter"), normalizedFilter.mid(5));
    }

    if (!query.exec()) {
        if (!query_.isEmpty()) {
            QStringList fallbackPredicates;
            if (!filterPredicate.isEmpty()) {
                fallbackPredicates.append(filterPredicate);
            }
            if (!dynamicFilterPredicate.isEmpty()) {
                fallbackPredicates.append(dynamicFilterPredicate);
            }
            fallbackPredicates.append(QStringLiteral(
                "(d.title LIKE :term OR d.author LIKE :term OR d.description LIKE :term)"));
            QSqlQuery fallback(database_->connection());
            fallback.prepare(QStringLiteral("SELECT %1 FROM documents d WHERE %2 "
                                           "ORDER BY COALESCE(d.last_opened_at, d.added_at) DESC")
                                 .arg(columns, fallbackPredicates.join(QStringLiteral(" AND "))));
            fallback.bindValue(QStringLiteral(":term"), QStringLiteral("%") + query_ + QStringLiteral("%"));
            if (normalizedFilter.startsWith(QStringLiteral("collection:"))) {
                fallback.bindValue(QStringLiteral(":collection_filter"), normalizedFilter.mid(11));
            } else if (normalizedFilter.startsWith(QStringLiteral("tag:"))) {
                fallback.bindValue(QStringLiteral(":tag_filter"), normalizedFilter.mid(4));
            } else if (normalizedFilter.startsWith(QStringLiteral("type:"))) {
                fallback.bindValue(QStringLiteral(":type_filter"), normalizedFilter.mid(5));
            }
            if (fallback.exec()) {
                while (fallback.next()) {
                    records_.append(recordFromQuery(fallback));
                }
            }
        } else {
            setError(QStringLiteral("The library could not be loaded."));
        }
    } else {
        while (query.next()) {
            records_.append(recordFromQuery(query));
        }
    }

    endResetModel();
    emit countChanged();
    emit libraryChanged();
}

QVariantMap LibraryStore::recordFromQuery(const QSqlQuery &query) const
{
    return {
        {QStringLiteral("id"), query.value(0)},
        {QStringLiteral("title"), query.value(1)},
        {QStringLiteral("author"), query.value(2)},
        {QStringLiteral("filePath"), query.value(3)},
        {QStringLiteral("format"), query.value(4)},
        {QStringLiteral("contentType"), query.value(5)},
        {QStringLiteral("status"), query.value(6)},
        {QStringLiteral("progress"), query.value(7)},
        {QStringLiteral("currentPage"), query.value(8)},
        {QStringLiteral("pageCount"), query.value(9)},
        {QStringLiteral("coverPath"), query.value(10)},
        {QStringLiteral("fileSize"), query.value(11)},
        {QStringLiteral("addedAt"), query.value(12)},
        {QStringLiteral("lastOpenedAt"), query.value(13)},
        {QStringLiteral("description"), query.value(14)},
        {QStringLiteral("language"), query.value(15)},
        {QStringLiteral("isFavorite"), query.value(16)},
        {QStringLiteral("rating"), query.value(17)},
        {QStringLiteral("pageRotation"), query.value(18)},
        {QStringLiteral("readingSeconds"), query.value(19)},
    };
}

bool LibraryStore::importFiles(const QStringList &filePaths)
{
    if (!isReady()) {
        setError(QStringLiteral("The local library is not ready yet."));
        return false;
    }

    int imported = 0;
    for (const QString &filePath : filePaths) {
        if (importSingle(filePath)) {
            ++imported;
        }
    }

    if (imported > 0) {
        reload();
        emit documentsImported(imported);
    }
    return imported > 0;
}

bool LibraryStore::importFolder(const QString &folderPath, bool recursive)
{
    if (!isReady()) {
        setError(QStringLiteral("The local library is not ready yet."));
        return false;
    }

    const QFileInfo folderInfo(folderPath);
    if (!folderInfo.exists() || !folderInfo.isDir() || !folderInfo.isReadable()) {
        setError(QStringLiteral("The selected folder could not be read."));
        return false;
    }

    const QDirIterator::IteratorFlags flags = recursive
        ? QDirIterator::Subdirectories
        : QDirIterator::NoIteratorFlags;
    QDirIterator iterator(folderInfo.absoluteFilePath(),
                          supportedExtensions(),
                          QDir::Files | QDir::Readable,
                          flags);
    QStringList files;
    while (iterator.hasNext()) {
        files.append(iterator.next());
    }
    if (files.isEmpty()) {
        return false;
    }
    return importFiles(files);
}

bool LibraryStore::watchFolder(const QString &folderPath, bool recursive)
{
    const QFileInfo folderInfo(folderPath);
    if (!folderInfo.exists() || !folderInfo.isDir() || !folderInfo.isReadable()) {
        setError(QStringLiteral("The selected folder could not be watched."));
        return false;
    }

    const QString root = normalizedPath(folderPath);
    if (!watchedFolders_.contains(root)) {
        watchedFolders_.append(root);
    }
    QStringList directories{root};
    if (recursive) {
        QDirIterator iterator(root, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            directories.append(iterator.next());
        }
    }
    folderWatcher_->addPaths(directories);
    importFolder(root, recursive);
    return true;
}

bool LibraryStore::unwatchFolder(const QString &folderPath)
{
    const QString root = normalizedPath(folderPath);
    if (!watchedFolders_.removeOne(root)) {
        return false;
    }
    const QStringList paths = folderWatcher_->directories();
    for (const QString &path : paths) {
        if (path == root || path.startsWith(root + QDir::separator())) {
            folderWatcher_->removePath(path);
        }
    }
    return true;
}

bool LibraryStore::documentAvailable(const QString &documentId) const
{
    const QVariantMap record = documentDetails(documentId);
    if (record.isEmpty()) {
        return false;
    }
    const QFileInfo info(record.value(QStringLiteral("filePath")).toString());
    return info.exists() && info.isFile() && info.isReadable();
}

bool LibraryStore::relinkDocument(const QString &documentId, const QString &filePath)
{
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return false;
    }

    const QVariantMap record = documentDetails(documentId);
    const QFileInfo info(filePath);
    if (record.isEmpty() || !info.exists() || !info.isFile() || !info.isReadable()) {
        setError(QStringLiteral("The selected file could not be read."));
        return false;
    }

    QSqlQuery stored(database_->connection());
    stored.prepare(QStringLiteral("SELECT file_hash FROM documents WHERE id = :id"));
    stored.bindValue(QStringLiteral(":id"), documentId);
    if (!stored.exec() || !stored.next()) {
        setError(QStringLiteral("The library document could not be inspected."));
        return false;
    }

    const QString path = normalizedPath(filePath);
    const QString hash = calculateHash(path);
    if (hash.isEmpty() || hash != stored.value(0).toString()) {
        setError(QStringLiteral("The selected file does not match this library document."));
        return false;
    }

    QSqlQuery update(database_->connection());
    update.prepare(QStringLiteral(
        "UPDATE documents SET file_path = :path, canonical_path = :canonical, file_size = :size, "
        "updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    update.bindValue(QStringLiteral(":path"), path);
    update.bindValue(QStringLiteral(":canonical"), path);
    update.bindValue(QStringLiteral(":size"), info.size());
    update.bindValue(QStringLiteral(":id"), documentId);
    if (!update.exec() || update.numRowsAffected() == 0) {
        setError(QStringLiteral("The document location could not be updated."));
        return false;
    }
    reload();
    return true;
}

bool LibraryStore::importSingle(const QString &filePath)
{
    const QFileInfo info(filePath);
    const QString format = formatForPath(filePath);
    if (!info.exists() || !info.isFile() || !info.isReadable()) {
        setError(QStringLiteral("The selected file could not be read."));
        return false;
    }
    if (!hasSupportedFormat(format)) {
        setError(QStringLiteral("This file format is not supported by Lumina Reader yet."));
        return false;
    }
    if (!hasExpectedSignature(filePath, format)) {
        setError(QStringLiteral("The selected file does not match its file extension."));
        return false;
    }

    const QString path = normalizedPath(filePath);
    const QString hash = calculateHash(path);
    if (hash.isEmpty()) {
        setError(QStringLiteral("The selected file could not be inspected."));
        return false;
    }

    QSqlDatabase connection = database_->connection();
    QSqlQuery existing(connection);
    existing.prepare(QStringLiteral("SELECT id, file_path FROM documents WHERE file_hash = :hash"));
    existing.bindValue(QStringLiteral(":hash"), hash);
    if (!existing.exec()) {
        setError(QStringLiteral("The library could not check for duplicate files."));
        return false;
    }
    if (existing.next()) {
        QSqlQuery update(connection);
        update.prepare(QStringLiteral(
            "UPDATE documents SET file_path = :path, canonical_path = :canonical, "
            "updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
        update.bindValue(QStringLiteral(":path"), path);
        update.bindValue(QStringLiteral(":canonical"), path);
        update.bindValue(QStringLiteral(":id"), existing.value(0));
        if (!update.exec()) {
            setError(QStringLiteral("The duplicate document location could not be updated."));
            return false;
        }
        return true;
    }

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString title = info.completeBaseName().trimmed().isEmpty()
        ? info.fileName()
        : info.completeBaseName();
    const QString contentType = contentTypeForFormat(format);
    const QString text = searchableText(path, format);
    const QString coverPath = generatedCoverPath(path, format, hash);

    if (!connection.transaction()) {
        setError(QStringLiteral("The document could not be added to the library."));
        return false;
    }

    QSqlQuery insert(connection);
    insert.prepare(QStringLiteral(
        "INSERT INTO documents (id, file_path, canonical_path, file_hash, file_size, format, "
        "content_type, title, cover_path) VALUES (:id, :path, :canonical, :hash, :size, :format, "
        ":type, :title, :cover)"));
    insert.bindValue(QStringLiteral(":id"), id);
    insert.bindValue(QStringLiteral(":path"), path);
    insert.bindValue(QStringLiteral(":canonical"), path);
    insert.bindValue(QStringLiteral(":hash"), hash);
    insert.bindValue(QStringLiteral(":size"), info.size());
    insert.bindValue(QStringLiteral(":format"), format);
    insert.bindValue(QStringLiteral(":type"), contentType);
    insert.bindValue(QStringLiteral(":title"), title);
    insert.bindValue(QStringLiteral(":cover"), coverPath);
    bool success = insert.exec();

    if (success) {
        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "INSERT INTO document_search (document_id, title, author, description, content) "
            "VALUES (:id, :title, '', '', :content)"));
        search.bindValue(QStringLiteral(":id"), id);
        search.bindValue(QStringLiteral(":title"), title);
        search.bindValue(QStringLiteral(":content"), text);
        success = search.exec();
    }

    if (!success || !connection.commit()) {
        connection.rollback();
        setError(QStringLiteral("The document could not be added to the library."));
        return false;
    }

    qCInfo(luminaDatabase) << "Imported document" << id << format;
    return true;
}

bool LibraryStore::removeDocument(const QString &documentId, bool deleteOriginal)
{
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return false;
    }

    const QVariantMap record = documentDetails(documentId);
    if (record.isEmpty()) {
        return false;
    }

    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        setError(QStringLiteral("The document could not be removed from the library."));
        return false;
    }

    QSqlQuery knowledgeSearch(connection);
    knowledgeSearch.prepare(QStringLiteral("DELETE FROM knowledge_search WHERE document_id = :id"));
    knowledgeSearch.bindValue(QStringLiteral(":id"), documentId);
    if (!knowledgeSearch.exec()) {
        connection.rollback();
        setError(QStringLiteral("The document could not be removed from knowledge search."));
        return false;
    }

    QSqlQuery knowledgeItemsSearch(connection);
    knowledgeItemsSearch.prepare(
        QStringLiteral("DELETE FROM knowledge_items_search WHERE document_id = :id"));
    knowledgeItemsSearch.bindValue(QStringLiteral(":id"), documentId);
    if (!knowledgeItemsSearch.exec()) {
        connection.rollback();
        setError(QStringLiteral("The document could not be removed from knowledge search."));
        return false;
    }

    QSqlQuery search(connection);
    search.prepare(QStringLiteral("DELETE FROM document_search WHERE document_id = :id"));
    search.bindValue(QStringLiteral(":id"), documentId);
    if (!search.exec()) {
        connection.rollback();
        setError(QStringLiteral("The document could not be removed from search."));
        return false;
    }

    QSqlQuery query(connection);
    query.prepare(QStringLiteral("DELETE FROM documents WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec()) {
        connection.rollback();
        setError(QStringLiteral("The document could not be removed from the library."));
        return false;
    }
    if (!connection.commit()) {
        connection.rollback();
        setError(QStringLiteral("The document could not be removed from the library."));
        return false;
    }

    if (deleteOriginal && !QFile::remove(record.value(QStringLiteral("filePath")).toString())) {
        setError(QStringLiteral("The original file could not be deleted."));
    }
    reload();
    return true;
}

bool LibraryStore::setStatus(const QString &documentId, const QString &status)
{
    static const QStringList validStatuses = {
        QStringLiteral("to_read"), QStringLiteral("reading"), QStringLiteral("read"),
        QStringLiteral("abandoned"),
    };
    if (!isReady() || !validStatuses.contains(status)) {
        return false;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "UPDATE documents SET status = :status, updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    query.bindValue(QStringLiteral(":status"), status);
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec() || query.numRowsAffected() == 0) {
        return false;
    }
    reload();
    return true;
}

bool LibraryStore::setPageCount(const QString &documentId, int pageCount)
{
    if (!isReady() || documentId.trimmed().isEmpty() || pageCount <= 0) {
        return false;
    }

    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "UPDATE documents SET page_count = :page_count, updated_at = CURRENT_TIMESTAMP "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":page_count"), pageCount);
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec() || query.numRowsAffected() == 0) {
        return false;
    }
    reload();
    return true;
}

bool LibraryStore::setFavorite(const QString &documentId, bool favorite)
{
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return false;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "UPDATE documents SET is_favorite = :favorite, updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    query.bindValue(QStringLiteral(":favorite"), favorite ? 1 : 0);
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec() || query.numRowsAffected() == 0) {
        return false;
    }
    reload();
    return true;
}

bool LibraryStore::setRating(const QString &documentId, int rating)
{
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return false;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "UPDATE documents SET rating = :rating, updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    query.bindValue(QStringLiteral(":rating"), std::clamp(rating, 0, 5));
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec() || query.numRowsAffected() == 0) {
        return false;
    }
    reload();
    return true;
}

bool LibraryStore::updateMetadata(const QString &documentId,
                                  const QString &title,
                                  const QString &author,
                                  const QString &description,
                                  const QString &language,
                                  const QString &contentType)
{
    if (!isReady() || documentId.trimmed().isEmpty() || title.trimmed().isEmpty()) {
        return false;
    }
    const QString normalizedContentType = contentType.trimmed().toLower();
    if (!normalizedContentType.isEmpty() && !isValidContentType(normalizedContentType)) {
        return false;
    }
    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }
    QSqlQuery query(connection);
    query.prepare(QStringLiteral(
        "UPDATE documents SET title = :title, author = :author, description = :description, "
        "language = :language, content_type = COALESCE(NULLIF(:content_type, ''), content_type), "
        "updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    query.bindValue(QStringLiteral(":title"), title.trimmed());
    query.bindValue(QStringLiteral(":author"), author.trimmed());
    query.bindValue(QStringLiteral(":description"), description.trimmed());
    query.bindValue(QStringLiteral(":language"), language.trimmed());
    query.bindValue(QStringLiteral(":content_type"), normalizedContentType);
    query.bindValue(QStringLiteral(":id"), documentId);
    bool success = query.exec() && query.numRowsAffected() > 0;
    if (success) {
        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "UPDATE document_search SET title = :title, author = :author, description = :description "
            "WHERE document_id = :id"));
        search.bindValue(QStringLiteral(":title"), title.trimmed());
        search.bindValue(QStringLiteral(":author"), author.trimmed());
        search.bindValue(QStringLiteral(":description"), description.trimmed());
        search.bindValue(QStringLiteral(":id"), documentId);
        success = search.exec();
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    reload();
    return true;
}

bool LibraryStore::setCoverPath(const QString &documentId, const QString &coverPath)
{
    if (!isReady() || documentId.trimmed().isEmpty() || documentDetails(documentId).isEmpty()) {
        return false;
    }

    QString normalizedCoverPath = QStringLiteral("");
    if (!coverPath.trimmed().isEmpty()) {
        const QFileInfo coverInfo(coverPath);
        if (!coverInfo.exists() || !coverInfo.isFile() || !coverInfo.isReadable()) {
            return false;
        }
        QImageReader reader(coverInfo.absoluteFilePath());
        if (!reader.canRead()) {
            return false;
        }
        normalizedCoverPath = normalizedPath(coverInfo.absoluteFilePath());
    }

    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "UPDATE documents SET cover_path = :cover, updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    query.bindValue(QStringLiteral(":cover"), normalizedCoverPath);
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec() || query.numRowsAffected() == 0) {
        return false;
    }
    reload();
    return true;
}

bool LibraryStore::setPageRotation(const QString &documentId, int rotation)
{
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return false;
    }
    int normalized = rotation % 360;
    if (normalized < 0) {
        normalized += 360;
    }
    if (normalized != 0 && normalized != 90 && normalized != 180 && normalized != 270) {
        return false;
    }

    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "UPDATE documents SET page_rotation = :rotation, updated_at = CURRENT_TIMESTAMP "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":rotation"), normalized);
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec() || query.numRowsAffected() == 0) {
        return false;
    }
    reload();
    return true;
}

bool LibraryStore::updateProgress(const QString &documentId, double progress, int currentPage)
{
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return false;
    }

    progress = std::clamp(progress, 0.0, 1.0);
    currentPage = std::max(currentPage, 0);
    const QString status = progress >= 0.999 ? QStringLiteral("read")
                                             : progress > 0.0 ? QStringLiteral("reading")
                                                              : QStringLiteral("to_read");

    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }
    QSqlQuery update(connection);
    update.prepare(QStringLiteral(
        "UPDATE documents SET progress = :progress, current_page = :page, status = :status, "
        "last_opened_at = CURRENT_TIMESTAMP, updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    update.bindValue(QStringLiteral(":progress"), progress);
    update.bindValue(QStringLiteral(":page"), currentPage);
    update.bindValue(QStringLiteral(":status"), status);
    update.bindValue(QStringLiteral(":id"), documentId);
    bool success = update.exec() && update.numRowsAffected() > 0;
    if (success) {
        QSqlQuery history(connection);
        history.prepare(QStringLiteral(
            "INSERT INTO reading_history (document_id, progress, page) "
            "VALUES (:id, :progress, :page)"));
        history.bindValue(QStringLiteral(":id"), documentId);
        history.bindValue(QStringLiteral(":progress"), progress);
        history.bindValue(QStringLiteral(":page"), currentPage);
        success = history.exec();
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    reload();
    return true;
}

bool LibraryStore::addReadingTime(const QString &documentId, qint64 seconds)
{
    if (!isReady() || documentId.trimmed().isEmpty() || seconds <= 0) {
        return false;
    }

    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "UPDATE documents SET reading_seconds = reading_seconds + :seconds, "
        "updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    query.bindValue(QStringLiteral(":seconds"), seconds);
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec() || query.numRowsAffected() == 0) {
        return false;
    }
    reload();
    return true;
}

QVariantMap LibraryStore::documentDetails(const QString &documentId) const
{
    if (!isReady()) {
        return {};
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT id, title, author, file_path, format, content_type, status, progress, "
        "current_page, page_count, cover_path, file_size, added_at, COALESCE(last_opened_at, ''), "
        "description, language, is_favorite, rating, page_rotation, reading_seconds "
        "FROM documents WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec() || !query.next()) {
        return {};
    }
    return recordFromQuery(query);
}

QVariantList LibraryStore::collections() const
{
    QVariantList results;
    if (!isReady()) {
        return results;
    }
    QSqlQuery query(database_->connection());
    if (!query.exec(QStringLiteral(
            "SELECT id, name, description, created_at, updated_at FROM collections "
            "ORDER BY name COLLATE NOCASE"))) {
        return results;
    }
    while (query.next()) {
        results.append(QVariantMap{
            {QStringLiteral("id"), query.value(0)},
            {QStringLiteral("name"), query.value(1)},
            {QStringLiteral("description"), query.value(2)},
            {QStringLiteral("createdAt"), query.value(3)},
            {QStringLiteral("updatedAt"), query.value(4)},
        });
    }
    return results;
}

QVariantList LibraryStore::collectionsForDocument(const QString &documentId) const
{
    QVariantList results;
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return results;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT c.id, c.name, c.description FROM collections c "
        "JOIN document_collections dc ON dc.collection_id = c.id "
        "WHERE dc.document_id = :document ORDER BY c.name COLLATE NOCASE"));
    query.bindValue(QStringLiteral(":document"), documentId);
    if (!query.exec()) {
        return results;
    }
    while (query.next()) {
        results.append(QVariantMap{
            {QStringLiteral("id"), query.value(0)},
            {QStringLiteral("name"), query.value(1)},
            {QStringLiteral("description"), query.value(2)},
        });
    }
    return results;
}

bool LibraryStore::createCollection(const QString &name, const QString &description)
{
    if (!isReady() || name.trimmed().isEmpty()) {
        return false;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "INSERT INTO collections (id, name, description) VALUES (:id, :name, :description)"));
    query.bindValue(QStringLiteral(":id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":name"), name.trimmed());
    query.bindValue(QStringLiteral(":description"), description.trimmed());
    return query.exec();
}

bool LibraryStore::deleteCollection(const QString &collectionId)
{
    if (!isReady() || collectionId.trimmed().isEmpty()) {
        return false;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral("DELETE FROM collections WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), collectionId);
    return query.exec() && query.numRowsAffected() > 0;
}

bool LibraryStore::addToCollection(const QString &documentId, const QString &collectionId)
{
    if (!isReady() || documentId.trimmed().isEmpty() || collectionId.trimmed().isEmpty()) {
        return false;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO document_collections (document_id, collection_id) "
        "VALUES (:document, :collection)"));
    query.bindValue(QStringLiteral(":document"), documentId);
    query.bindValue(QStringLiteral(":collection"), collectionId);
    return query.exec();
}

bool LibraryStore::removeFromCollection(const QString &documentId, const QString &collectionId)
{
    if (!isReady() || documentId.trimmed().isEmpty() || collectionId.trimmed().isEmpty()) {
        return false;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "DELETE FROM document_collections WHERE document_id = :document AND collection_id = :collection"));
    query.bindValue(QStringLiteral(":document"), documentId);
    query.bindValue(QStringLiteral(":collection"), collectionId);
    return query.exec() && query.numRowsAffected() > 0;
}

QVariantList LibraryStore::tags() const
{
    QVariantList results;
    if (!isReady()) {
        return results;
    }
    QSqlQuery query(database_->connection());
    if (!query.exec(QStringLiteral("SELECT id, name, created_at FROM tags ORDER BY name COLLATE NOCASE"))) {
        return results;
    }
    while (query.next()) {
        results.append(QVariantMap{
            {QStringLiteral("id"), query.value(0)},
            {QStringLiteral("name"), query.value(1)},
            {QStringLiteral("createdAt"), query.value(2)},
        });
    }
    return results;
}

QStringList LibraryStore::tagsForDocument(const QString &documentId) const
{
    QStringList results;
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return results;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT t.name FROM tags t JOIN document_tags dt ON dt.tag_id = t.id "
        "WHERE dt.document_id = :document ORDER BY t.name COLLATE NOCASE"));
    query.bindValue(QStringLiteral(":document"), documentId);
    if (!query.exec()) {
        return results;
    }
    while (query.next()) {
        results.append(query.value(0).toString());
    }
    return results;
}

bool LibraryStore::setTags(const QString &documentId, const QStringList &tags)
{
    if (!isReady() || documentId.trimmed().isEmpty() || documentDetails(documentId).isEmpty()) {
        return false;
    }
    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }
    QSqlQuery remove(connection);
    remove.prepare(QStringLiteral("DELETE FROM document_tags WHERE document_id = :document"));
    remove.bindValue(QStringLiteral(":document"), documentId);
    bool success = remove.exec();
    for (const QString &rawTag : tags) {
        const QString tag = rawTag.trimmed();
        if (!success || tag.isEmpty()) {
            continue;
        }
        QSqlQuery insertTag(connection);
        insertTag.prepare(QStringLiteral(
            "INSERT OR IGNORE INTO tags (id, name) VALUES (:id, :name)"));
        insertTag.bindValue(QStringLiteral(":id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
        insertTag.bindValue(QStringLiteral(":name"), tag);
        success = insertTag.exec();
        if (!success) {
            break;
        }

        QSqlQuery findTag(connection);
        findTag.prepare(QStringLiteral("SELECT id FROM tags WHERE name = :name"));
        findTag.bindValue(QStringLiteral(":name"), tag);
        success = findTag.exec() && findTag.next();
        if (!success) {
            break;
        }

        QSqlQuery link(connection);
        link.prepare(QStringLiteral(
            "INSERT OR IGNORE INTO document_tags (document_id, tag_id) VALUES (:document, :tag)"));
        link.bindValue(QStringLiteral(":document"), documentId);
        link.bindValue(QStringLiteral(":tag"), findTag.value(0));
        success = link.exec();
        if (!success) {
            break;
        }
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    return true;
}

QVariantList LibraryStore::knowledgeDocumentOptions() const
{
    QVariantList results;
    if (!isReady()) {
        return results;
    }
    QSqlQuery query(database_->connection());
    if (!query.exec(QStringLiteral(
            "SELECT id, title, author FROM documents ORDER BY title COLLATE NOCASE"))) {
        return results;
    }
    while (query.next()) {
        results.append(QVariantMap{
            {QStringLiteral("id"), query.value(0)},
            {QStringLiteral("title"), query.value(1)},
            {QStringLiteral("author"), query.value(2)},
        });
    }
    return results;
}

QVariantList LibraryStore::knowledgeSearch(const QString &term,
                                           const QString &format,
                                           const QString &color,
                                           const QString &documentId,
                                           const QString &author,
                                           const QString &createdAfter,
                                           const QString &createdBefore,
                                           const QString &order) const
{
    QVariantList results;
    if (!isReady() || term.trimmed().isEmpty()) {
        return results;
    }

    const QString match = ftsPrefixQuery(term);
    if (match.isEmpty()) {
        return results;
    }

    QSqlQuery query(database_->connection());
    QString statement = QStringLiteral(
        "SELECT ks.source_id, ks.source_kind, ks.document_id, d.title, d.author, d.format, ks.text, "
        "CASE WHEN ks.source_kind = 'highlight' THEN COALESCE(h.note, '') "
        "WHEN ks.source_kind = 'annotation' THEN COALESCE(a.kind, '') ELSE '' END, "
        "CASE WHEN ks.source_kind = 'highlight' THEN COALESCE(h.color, '#f6c945') ELSE '' END, "
        "CASE WHEN ks.source_kind = 'highlight' THEN COALESCE(h.style, 'highlight') ELSE '' END, "
        "ks.page, ks.locator, ks.created_at, COALESCE(a.kind, '') "
        "FROM knowledge_items_search ks JOIN documents d ON d.id = ks.document_id "
        "LEFT JOIN highlights h ON h.id = ks.source_id AND ks.source_kind = 'highlight' "
        "LEFT JOIN annotations a ON a.id = ks.source_id AND ks.source_kind = 'annotation' "
        "WHERE knowledge_items_search MATCH :match");
    const QString normalizedFormat = format.trimmed().toLower();
    const QString normalizedColor = color.trimmed().toLower();
    const QString normalizedDocumentId = documentId.trimmed();
    const QString normalizedAuthor = author.trimmed();
    const QString normalizedCreatedAfter = createdAfter.trimmed();
    const QString normalizedCreatedBefore = createdBefore.trimmed();
    if (!normalizedFormat.isEmpty()) {
        statement += QStringLiteral(" AND d.format = :format");
    }
    if (!normalizedColor.isEmpty()) {
        statement += QStringLiteral(" AND COALESCE(h.color, '') = :color");
    }
    if (!normalizedDocumentId.isEmpty()) {
        statement += QStringLiteral(" AND ks.document_id = :document");
    }
    if (!normalizedAuthor.isEmpty()) {
        statement += QStringLiteral(" AND lower(COALESCE(d.author, '')) LIKE lower(:author)");
    }
    if (!normalizedCreatedAfter.isEmpty()) {
        statement += QStringLiteral(" AND ks.created_at >= :created_after");
    }
    if (!normalizedCreatedBefore.isEmpty()) {
        statement += QStringLiteral(" AND ks.created_at <= :created_before");
    }
    const QString normalizedOrder = order.trimmed().toLower();
    if (normalizedOrder == QStringLiteral("oldest")) {
        statement += QStringLiteral(" ORDER BY ks.created_at ASC");
    } else if (normalizedOrder == QStringLiteral("newest")) {
        statement += QStringLiteral(" ORDER BY ks.created_at DESC");
    } else {
        statement += QStringLiteral(" ORDER BY rank, ks.created_at DESC");
    }
    query.prepare(statement);
    query.bindValue(QStringLiteral(":match"), match);
    if (!normalizedFormat.isEmpty()) {
        query.bindValue(QStringLiteral(":format"), normalizedFormat);
    }
    if (!normalizedColor.isEmpty()) {
        query.bindValue(QStringLiteral(":color"), normalizedColor);
    }
    if (!normalizedDocumentId.isEmpty()) {
        query.bindValue(QStringLiteral(":document"), normalizedDocumentId);
    }
    if (!normalizedAuthor.isEmpty()) {
        query.bindValue(QStringLiteral(":author"), QStringLiteral("%") + normalizedAuthor + QStringLiteral("%"));
    }
    if (!normalizedCreatedAfter.isEmpty()) {
        query.bindValue(QStringLiteral(":created_after"), normalizedCreatedAfter + QStringLiteral(" 00:00:00"));
    }
    if (!normalizedCreatedBefore.isEmpty()) {
        query.bindValue(QStringLiteral(":created_before"), normalizedCreatedBefore + QStringLiteral(" 23:59:59"));
    }
    if (!query.exec()) {
        return results;
    }
    while (query.next()) {
        results.append(QVariantMap{
            {QStringLiteral("id"), query.value(0)},
            {QStringLiteral("kind"), query.value(1)},
            {QStringLiteral("documentId"), query.value(2)},
            {QStringLiteral("documentTitle"), query.value(3)},
            {QStringLiteral("author"), query.value(4)},
            {QStringLiteral("format"), query.value(5)},
            {QStringLiteral("text"), query.value(6)},
            {QStringLiteral("note"), query.value(7)},
            {QStringLiteral("color"), query.value(8)},
            {QStringLiteral("style"), query.value(9)},
            {QStringLiteral("page"), query.value(10)},
            {QStringLiteral("locator"), query.value(11)},
            {QStringLiteral("createdAt"), query.value(12)},
            {QStringLiteral("annotationKind"), query.value(13)},
        });
    }
    return results;
}

bool LibraryStore::addHighlight(const QString &documentId,
                                const QString &text,
                                const QString &locator,
                                int page,
                                const QString &color,
                                const QString &note,
                                const QString &style)
{
    if (!isReady() || documentDetails(documentId).isEmpty() || text.trimmed().isEmpty()
        || locator.trimmed().isEmpty()) {
        return false;
    }
    const QString normalizedStyle = style.trimmed().toLower();
    if (normalizedStyle != QStringLiteral("highlight")
        && normalizedStyle != QStringLiteral("underline")
        && normalizedStyle != QStringLiteral("strike")) {
        return false;
    }
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }

    QSqlQuery query(connection);
    query.prepare(QStringLiteral(
        "INSERT INTO highlights (id, document_id, text, note, color, style, locator, page) "
        "VALUES (:id, :document, :text, :note, :color, :style, :locator, :page)"));
    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":document"), documentId);
    query.bindValue(QStringLiteral(":text"), text);
    query.bindValue(QStringLiteral(":note"), note);
    query.bindValue(QStringLiteral(":color"), color.isEmpty() ? QStringLiteral("#f6c945") : color);
    query.bindValue(QStringLiteral(":style"), normalizedStyle);
    query.bindValue(QStringLiteral(":locator"), locator);
    query.bindValue(QStringLiteral(":page"), std::max(page, 0));
    bool success = query.exec();
    if (success) {
        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "INSERT INTO knowledge_search (highlight_id, document_id, text, note) "
            "VALUES (:highlight, :document, :text, :note)"));
        search.bindValue(QStringLiteral(":highlight"), id);
        search.bindValue(QStringLiteral(":document"), documentId);
        search.bindValue(QStringLiteral(":text"), text);
        search.bindValue(QStringLiteral(":note"), note);
        success = search.exec();
    }
    if (success) {
        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "INSERT INTO knowledge_items_search "
            "(source_id, source_kind, document_id, text, note, page, locator, created_at) "
            "VALUES (:source, 'highlight', :document, :text, :note, :page, :locator, CURRENT_TIMESTAMP)"));
        search.bindValue(QStringLiteral(":source"), id);
        search.bindValue(QStringLiteral(":document"), documentId);
        search.bindValue(QStringLiteral(":text"), text);
        search.bindValue(QStringLiteral(":note"), note);
        search.bindValue(QStringLiteral(":page"), std::max(page, 0));
        search.bindValue(QStringLiteral(":locator"), locator);
        success = search.exec();
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    return true;
}

QVariantList LibraryStore::highlightsForDocument(const QString &documentId) const
{
    QVariantList results;
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return results;
    }

    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT id, text, note, color, style, locator, context_before, context_after, page, "
        "created_at, updated_at FROM highlights WHERE document_id = :id ORDER BY page, created_at"));
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec()) {
        return results;
    }
    while (query.next()) {
        results.append(QVariantMap{
            {QStringLiteral("id"), query.value(0)},
            {QStringLiteral("text"), query.value(1)},
            {QStringLiteral("note"), query.value(2)},
            {QStringLiteral("color"), query.value(3)},
            {QStringLiteral("style"), query.value(4)},
            {QStringLiteral("locator"), query.value(5)},
            {QStringLiteral("contextBefore"), query.value(6)},
            {QStringLiteral("contextAfter"), query.value(7)},
            {QStringLiteral("page"), query.value(8)},
            {QStringLiteral("createdAt"), query.value(9)},
            {QStringLiteral("updatedAt"), query.value(10)},
        });
    }
    return results;
}

bool LibraryStore::updateHighlight(const QString &highlightId,
                                   const QString &color,
                                   const QString &note,
                                   const QString &style)
{
    if (!isReady() || highlightId.trimmed().isEmpty()) {
        return false;
    }
    const QString normalizedStyle = style.trimmed().toLower();
    if (normalizedStyle != QStringLiteral("highlight")
        && normalizedStyle != QStringLiteral("underline")
        && normalizedStyle != QStringLiteral("strike")) {
        return false;
    }
    const QString normalizedColor = color.trimmed().isEmpty()
        ? QStringLiteral("#f6c945")
        : color.trimmed();
    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }

    QSqlQuery query(connection);
    query.prepare(QStringLiteral(
        "UPDATE highlights SET color = :color, note = :note, style = :style, "
        "updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    query.bindValue(QStringLiteral(":color"), normalizedColor);
    query.bindValue(QStringLiteral(":note"), note.trimmed());
    query.bindValue(QStringLiteral(":style"), normalizedStyle);
    query.bindValue(QStringLiteral(":id"), highlightId);
    bool success = query.exec() && query.numRowsAffected() > 0;
    if (success) {
        QSqlQuery legacy(connection);
        legacy.prepare(QStringLiteral(
            "UPDATE knowledge_search SET note = :note WHERE highlight_id = :id"));
        legacy.bindValue(QStringLiteral(":note"), note.trimmed());
        legacy.bindValue(QStringLiteral(":id"), highlightId);
        success = legacy.exec();
    }
    if (success) {
        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "UPDATE knowledge_items_search SET note = :note "
            "WHERE source_id = :id AND source_kind = 'highlight'"));
        search.bindValue(QStringLiteral(":note"), note.trimmed());
        search.bindValue(QStringLiteral(":id"), highlightId);
        success = search.exec();
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    return true;
}

bool LibraryStore::removeHighlight(const QString &highlightId)
{
    if (!isReady() || highlightId.trimmed().isEmpty()) {
        return false;
    }
    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }
    QSqlQuery search(connection);
    search.prepare(QStringLiteral(
        "DELETE FROM knowledge_items_search WHERE source_id = :id AND source_kind = 'highlight'"));
    search.bindValue(QStringLiteral(":id"), highlightId);
    bool success = search.exec();
    if (success) {
        QSqlQuery legacy(connection);
        legacy.prepare(QStringLiteral("DELETE FROM knowledge_search WHERE highlight_id = :id"));
        legacy.bindValue(QStringLiteral(":id"), highlightId);
        success = legacy.exec();
    }
    if (success) {
        QSqlQuery query(connection);
        query.prepare(QStringLiteral("DELETE FROM highlights WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), highlightId);
        success = query.exec() && query.numRowsAffected() > 0;
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    return true;
}

QVariantList LibraryStore::annotationsForDocument(const QString &documentId) const
{
    QVariantList results;
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return results;
    }

    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT id, text, kind, locator, page, x, y, width, height, created_at, updated_at "
        "FROM annotations WHERE document_id = :id ORDER BY page, created_at"));
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec()) {
        return results;
    }
    while (query.next()) {
        results.append(QVariantMap{
            {QStringLiteral("id"), query.value(0)},
            {QStringLiteral("text"), query.value(1)},
            {QStringLiteral("kind"), query.value(2)},
            {QStringLiteral("locator"), query.value(3)},
            {QStringLiteral("page"), query.value(4)},
            {QStringLiteral("x"), query.value(5)},
            {QStringLiteral("y"), query.value(6)},
            {QStringLiteral("width"), query.value(7)},
            {QStringLiteral("height"), query.value(8)},
            {QStringLiteral("createdAt"), query.value(9)},
            {QStringLiteral("updatedAt"), query.value(10)},
        });
    }
    return results;
}

bool LibraryStore::addAnnotation(const QString &documentId,
                                 const QString &text,
                                 const QString &kind,
                                 const QString &locator,
                                 int page,
                                 double x,
                                 double y,
                                 double width,
                                 double height)
{
    if (!isReady() || documentDetails(documentId).isEmpty() || text.trimmed().isEmpty()
        || locator.trimmed().isEmpty()) {
        return false;
    }
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }
    QSqlQuery query(connection);
    query.prepare(QStringLiteral(
        "INSERT INTO annotations (id, document_id, text, kind, locator, page, x, y, width, height) "
        "VALUES (:id, :document, :text, :kind, :locator, :page, :x, :y, :width, :height)"));
    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":document"), documentId);
    query.bindValue(QStringLiteral(":text"), text);
    query.bindValue(QStringLiteral(":kind"), kind.isEmpty() ? QStringLiteral("note") : kind);
    query.bindValue(QStringLiteral(":locator"), locator);
    query.bindValue(QStringLiteral(":page"), std::max(page, 0));
    query.bindValue(QStringLiteral(":x"), x);
    query.bindValue(QStringLiteral(":y"), y);
    query.bindValue(QStringLiteral(":width"), width);
    query.bindValue(QStringLiteral(":height"), height);
    bool success = query.exec();
    if (success) {
        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "INSERT INTO knowledge_items_search "
            "(source_id, source_kind, document_id, text, note, page, locator, created_at) "
            "VALUES (:source, 'annotation', :document, :text, :note, :page, :locator, CURRENT_TIMESTAMP)"));
        search.bindValue(QStringLiteral(":source"), id);
        search.bindValue(QStringLiteral(":document"), documentId);
        search.bindValue(QStringLiteral(":text"), text);
        search.bindValue(QStringLiteral(":note"), kind);
        search.bindValue(QStringLiteral(":page"), std::max(page, 0));
        search.bindValue(QStringLiteral(":locator"), locator);
        success = search.exec();
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    return true;
}

bool LibraryStore::updateAnnotation(const QString &annotationId,
                                    const QString &text,
                                    const QString &kind)
{
    const QString normalizedId = annotationId.trimmed();
    const QString normalizedText = text.trimmed();
    const QString normalizedKind = kind.trimmed().toLower();
    if (!isReady() || normalizedId.isEmpty() || normalizedText.isEmpty()
        || normalizedKind.isEmpty()) {
        return false;
    }

    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }

    QSqlQuery query(connection);
    query.prepare(QStringLiteral(
        "UPDATE annotations SET text = :text, kind = :kind, updated_at = CURRENT_TIMESTAMP "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":text"), normalizedText);
    query.bindValue(QStringLiteral(":kind"), normalizedKind);
    query.bindValue(QStringLiteral(":id"), normalizedId);
    bool success = query.exec() && query.numRowsAffected() > 0;
    if (success) {
        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "UPDATE knowledge_items_search SET text = :text, note = :note "
            "WHERE source_id = :id AND source_kind = 'annotation'"));
        search.bindValue(QStringLiteral(":text"), normalizedText);
        search.bindValue(QStringLiteral(":note"), normalizedKind);
        search.bindValue(QStringLiteral(":id"), normalizedId);
        success = search.exec();
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    return true;
}

bool LibraryStore::removeAnnotation(const QString &annotationId)
{
    if (!isReady() || annotationId.trimmed().isEmpty()) {
        return false;
    }
    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }
    QSqlQuery search(connection);
    search.prepare(QStringLiteral(
        "DELETE FROM knowledge_items_search WHERE source_id = :id AND source_kind = 'annotation'"));
    search.bindValue(QStringLiteral(":id"), annotationId);
    bool success = search.exec();
    if (success) {
        QSqlQuery query(connection);
        query.prepare(QStringLiteral("DELETE FROM annotations WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), annotationId);
        success = query.exec() && query.numRowsAffected() > 0;
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    return true;
}

QString LibraryStore::ocrTextForPage(const QString &documentId, int page) const
{
    if (!isReady() || documentId.trimmed().isEmpty() || page < 0) {
        return {};
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT text FROM ocr_pages WHERE document_id = :document AND page = :page "
        "AND status = 'completed'"));
    query.bindValue(QStringLiteral(":document"), documentId);
    query.bindValue(QStringLiteral(":page"), page);
    return query.exec() && query.next() ? query.value(0).toString() : QString();
}

bool LibraryStore::saveOcrPage(const QString &documentId,
                               int page,
                               const QString &language,
                               const QString &text,
                               const QString &status)
{
    if (!isReady() || documentId.trimmed().isEmpty() || documentDetails(documentId).isEmpty()
        || page < 0) {
        return false;
    }
    const QString normalizedStatus = status.trimmed().toLower();
    if (normalizedStatus != QStringLiteral("pending")
        && normalizedStatus != QStringLiteral("completed")
        && normalizedStatus != QStringLiteral("failed")) {
        return false;
    }

    const QString boundedText = text.left(4 * 1024 * 1024);
    const QString sourceId = documentId + QStringLiteral(":") + QString::number(page);
    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }

    QSqlQuery pageQuery(connection);
    pageQuery.prepare(QStringLiteral(
        "INSERT INTO ocr_pages (document_id, page, language, text, status) "
        "VALUES (:document, :page, :language, :text, :status) "
        "ON CONFLICT(document_id, page) DO UPDATE SET language = excluded.language, "
        "text = excluded.text, status = excluded.status, updated_at = CURRENT_TIMESTAMP"));
    pageQuery.bindValue(QStringLiteral(":document"), documentId);
    pageQuery.bindValue(QStringLiteral(":page"), page);
    pageQuery.bindValue(QStringLiteral(":language"), language.trimmed());
    pageQuery.bindValue(QStringLiteral(":text"), boundedText);
    pageQuery.bindValue(QStringLiteral(":status"), normalizedStatus);
    bool success = pageQuery.exec();

    if (success) {
        QSqlQuery removeSearch(connection);
        removeSearch.prepare(QStringLiteral(
            "DELETE FROM knowledge_items_search WHERE source_id = :source AND source_kind = 'ocr'"));
        removeSearch.bindValue(QStringLiteral(":source"), sourceId);
        success = removeSearch.exec();
    }

    if (success && normalizedStatus == QStringLiteral("completed") && !boundedText.trimmed().isEmpty()) {
        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "INSERT INTO knowledge_items_search "
            "(source_id, source_kind, document_id, text, note, page, locator, created_at) "
            "VALUES (:source, 'ocr', :document, :text, '', :page, :locator, CURRENT_TIMESTAMP)"));
        search.bindValue(QStringLiteral(":source"), sourceId);
        search.bindValue(QStringLiteral(":document"), documentId);
        search.bindValue(QStringLiteral(":text"), boundedText);
        search.bindValue(QStringLiteral(":page"), page);
        search.bindValue(QStringLiteral(":locator"), QStringLiteral("ocr:") + QString::number(page));
        success = search.exec();
    }

    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    return true;
}

QVariantList LibraryStore::bookmarksForDocument(const QString &documentId) const
{
    QVariantList results;
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return results;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT id, label, locator, page, created_at FROM bookmarks "
        "WHERE document_id = :id ORDER BY page, created_at"));
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec()) {
        return results;
    }
    while (query.next()) {
        results.append(QVariantMap{
            {QStringLiteral("id"), query.value(0)},
            {QStringLiteral("label"), query.value(1)},
            {QStringLiteral("locator"), query.value(2)},
            {QStringLiteral("page"), query.value(3)},
            {QStringLiteral("createdAt"), query.value(4)},
        });
    }
    return results;
}

bool LibraryStore::addBookmark(const QString &documentId,
                               const QString &label,
                               const QString &locator,
                               int page)
{
    if (!isReady() || documentDetails(documentId).isEmpty() || locator.trimmed().isEmpty()) {
        return false;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "INSERT INTO bookmarks (id, document_id, label, locator, page) "
        "VALUES (:id, :document, :label, :locator, :page)"));
    query.bindValue(QStringLiteral(":id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":document"), documentId);
    query.bindValue(QStringLiteral(":label"), label);
    query.bindValue(QStringLiteral(":locator"), locator);
    query.bindValue(QStringLiteral(":page"), std::max(page, 0));
    return query.exec();
}

bool LibraryStore::removeBookmark(const QString &bookmarkId)
{
    if (!isReady() || bookmarkId.trimmed().isEmpty()) {
        return false;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral("DELETE FROM bookmarks WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), bookmarkId);
    return query.exec() && query.numRowsAffected() > 0;
}

QStringList LibraryStore::supportedExtensions() const
{
    QStringList extensions;
    for (const QString &format : supportedFormats()) {
        extensions.append(QStringLiteral("*.") + format);
    }
    return extensions;
}

} // namespace Lumina::Infrastructure
