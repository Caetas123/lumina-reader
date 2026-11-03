#pragma once

#include <QObject>
#include <QString>

#include <memory>

#ifdef LUMINA_HAS_QT_PDF
class QPdfDocument;
#endif

namespace Lumina::Infrastructure {
class LibraryStore;
}

namespace Lumina::Application {

class ReaderController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool open READ isOpen NOTIFY stateChanged)
    Q_PROPERTY(QString documentId READ documentId NOTIFY stateChanged)
    Q_PROPERTY(QString title READ title NOTIFY stateChanged)
    Q_PROPERTY(QString filePath READ filePath NOTIFY stateChanged)
    Q_PROPERTY(QString format READ format NOTIFY stateChanged)
    Q_PROPERTY(QString contentType READ contentType NOTIFY stateChanged)
    Q_PROPERTY(QString textContent READ textContent NOTIFY stateChanged)
    Q_PROPERTY(QString imageSource READ imageSource NOTIFY stateChanged)
    Q_PROPERTY(bool imageDocument READ isImageDocument NOTIFY stateChanged)
    Q_PROPERTY(bool textDocument READ isTextDocument NOTIFY stateChanged)
    Q_PROPERTY(QString engineName READ engineName NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY stateChanged)
    Q_PROPERTY(int currentPage READ currentPage NOTIFY stateChanged)

public:
    explicit ReaderController(Lumina::Infrastructure::LibraryStore *library,
                              QObject *parent = nullptr);
    ~ReaderController() override;

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] QString documentId() const;
    [[nodiscard]] QString title() const;
    [[nodiscard]] QString filePath() const;
    [[nodiscard]] QString format() const;
    [[nodiscard]] QString contentType() const;
    [[nodiscard]] QString textContent() const;
    [[nodiscard]] QString imageSource() const;
    [[nodiscard]] bool isImageDocument() const;
    [[nodiscard]] bool isTextDocument() const;
    [[nodiscard]] QString engineName() const;
    [[nodiscard]] QString errorMessage() const;
    [[nodiscard]] int pageCount() const;
    [[nodiscard]] int currentPage() const;

    Q_INVOKABLE bool openDocument(const QString &documentId);
    Q_INVOKABLE void closeDocument();
    Q_INVOKABLE bool nextPage();
    Q_INVOKABLE bool previousPage();
    Q_INVOKABLE bool goToPage(int page);
    Q_INVOKABLE bool updateProgress(double progress);

signals:
    void stateChanged();

private:
    void clearState();
    void fail(const QString &message);
    [[nodiscard]] QString loadText(const QString &path) const;
    void refreshTextPage();
    void persistProgress();
    [[nodiscard]] bool renderPdfPage();

    Lumina::Infrastructure::LibraryStore *library_ = nullptr;
    bool open_ = false;
    QString documentId_;
    QString title_;
    QString filePath_;
    QString format_;
    QString contentType_;
    QString fullTextContent_;
    QString textContent_;
    QString imageSource_;
    QString engineName_;
    QString errorMessage_;
    int pageCount_ = 0;
    int currentPage_ = 0;

#ifdef LUMINA_HAS_QT_PDF
    std::unique_ptr<QPdfDocument> pdfDocument_;
#endif
};

} // namespace Lumina::Application
