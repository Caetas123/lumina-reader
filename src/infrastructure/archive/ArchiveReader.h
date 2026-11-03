#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include <memory>

#ifdef LUMINA_HAS_QT_ZIP
class QZipReader;
#endif

namespace Lumina::Infrastructure {

class ArchiveReader final {
public:
    ArchiveReader();
    ~ArchiveReader();

    ArchiveReader(const ArchiveReader &) = delete;
    ArchiveReader &operator=(const ArchiveReader &) = delete;

    [[nodiscard]] bool open(const QString &path, const QString &format);
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] int pageCount() const;
    [[nodiscard]] bool pageIsImage(int page) const;
    [[nodiscard]] QString pageName(int page) const;
    [[nodiscard]] QByteArray pageData(int page) const;
    [[nodiscard]] QString pageText(int page) const;

private:
    struct Page {
        QString name;
        bool image = false;
    };

    void clear();
    [[nodiscard]] bool openComic();
    [[nodiscard]] bool openEpub();

#ifdef LUMINA_HAS_QT_ZIP
    std::unique_ptr<QZipReader> zip_;
#endif
    QList<Page> pages_;
    QString format_;
};

} // namespace Lumina::Infrastructure
