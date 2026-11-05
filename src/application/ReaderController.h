#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QHash>
#include <QImage>
#include <QList>
#include <QString>
#include <QStringList>

#include <memory>

namespace Lumina::Infrastructure {
class LibraryStore;
class TesseractOcrProvider;
}

namespace Lumina::Domain::Reader {
class DocumentEngine;
}

class QPagedPaintDevice;

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
    Q_PROPERTY(QString continuousTextContent READ continuousTextContent NOTIFY stateChanged)
    Q_PROPERTY(QString imageSource READ imageSource NOTIFY stateChanged)
    Q_PROPERTY(bool imageDocument READ isImageDocument NOTIFY stateChanged)
    Q_PROPERTY(bool textDocument READ isTextDocument NOTIFY stateChanged)
    Q_PROPERTY(bool pageTextAvailable READ pageTextAvailable NOTIFY stateChanged)
    Q_PROPERTY(QString engineName READ engineName NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(bool ocrAvailable READ ocrAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool ocrRunning READ ocrRunning NOTIFY stateChanged)
    Q_PROPERTY(bool ocrDocumentRunning READ ocrDocumentRunning NOTIFY stateChanged)
    Q_PROPERTY(double ocrProgress READ ocrProgress NOTIFY stateChanged)
    Q_PROPERTY(QString ocrStatus READ ocrStatus NOTIFY stateChanged)
    Q_PROPERTY(bool showOcrText READ showOcrText WRITE setShowOcrText NOTIFY stateChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY stateChanged)
    Q_PROPERTY(int currentPage READ currentPage NOTIFY stateChanged)
    Q_PROPERTY(int pageRotation READ pageRotation NOTIFY stateChanged)
    Q_PROPERTY(int thumbnailRevision READ thumbnailRevision NOTIFY stateChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery NOTIFY stateChanged)
    Q_PROPERTY(int searchMatches READ searchMatches NOTIFY stateChanged)
    Q_PROPERTY(int searchCurrent READ searchCurrent NOTIFY stateChanged)
    Q_PROPERTY(int searchStart READ searchStart NOTIFY stateChanged)
    Q_PROPERTY(int searchLength READ searchLength NOTIFY stateChanged)

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
    [[nodiscard]] QString continuousTextContent() const;
    [[nodiscard]] QString imageSource() const;
    [[nodiscard]] bool isImageDocument() const;
    [[nodiscard]] bool isTextDocument() const;
    [[nodiscard]] bool pageTextAvailable() const;
    [[nodiscard]] QString engineName() const;
    [[nodiscard]] QString errorMessage() const;
    [[nodiscard]] bool ocrAvailable() const;
    [[nodiscard]] bool ocrRunning() const;
    [[nodiscard]] bool ocrDocumentRunning() const;
    [[nodiscard]] double ocrProgress() const;
    [[nodiscard]] QString ocrStatus() const;
    Q_INVOKABLE QStringList availableOcrLanguages() const;
    [[nodiscard]] bool showOcrText() const;
    void setShowOcrText(bool show);
    [[nodiscard]] int pageCount() const;
    [[nodiscard]] int currentPage() const;
    [[nodiscard]] int pageRotation() const;
    [[nodiscard]] int thumbnailRevision() const;
    [[nodiscard]] QString searchQuery() const;
    [[nodiscard]] int searchMatches() const;
    [[nodiscard]] int searchCurrent() const;
    [[nodiscard]] int searchStart() const;
    [[nodiscard]] int searchLength() const;

    Q_INVOKABLE bool openDocument(const QString &documentId);
    Q_INVOKABLE void closeDocument();
    Q_INVOKABLE bool nextPage();
    Q_INVOKABLE bool previousPage();
    Q_INVOKABLE bool goToPage(int page);
    Q_INVOKABLE QString pageName(int page) const;
    Q_INVOKABLE QString pageImageSource(int page) const;
    Q_INVOKABLE bool exportText(const QString &path);
    Q_INVOKABLE bool exportPdf(const QString &path);
    Q_INVOKABLE bool printDocument();
    Q_INVOKABLE QString thumbnailSource(int page) const;
    Q_INVOKABLE bool rotatePage(int degrees = 90);
    Q_INVOKABLE bool updateProgress(double progress);
    Q_INVOKABLE bool findInDocument(const QString &query);
    Q_INVOKABLE bool nextSearchMatch();
    Q_INVOKABLE bool previousSearchMatch();
    Q_INVOKABLE bool startOcr(const QString &language = QStringLiteral("eng"));
    Q_INVOKABLE bool startDocumentOcr(const QString &language = QStringLiteral("eng"));
    Q_INVOKABLE void cancelOcr();

signals:
    void stateChanged();

private:
    void clearState();
    void fail(const QString &message);
    [[nodiscard]] bool refreshPage();
    void rebuildContinuousText();
    void applyCachedOcr();
    [[nodiscard]] QImage pageImage(int page) const;
    [[nodiscard]] QImage currentPageImage() const;
    [[nodiscard]] bool renderDocument(QPagedPaintDevice &device) const;
    void processNextDocumentOcrPage();
    void rebuildSearchMatches();
    [[nodiscard]] bool selectSearchMatch(int index);
    void persistProgress();
    void persistReadingTime();

    Lumina::Infrastructure::LibraryStore *library_ = nullptr;
    std::unique_ptr<Lumina::Domain::Reader::DocumentEngine> documentEngine_;
    std::unique_ptr<Lumina::Infrastructure::TesseractOcrProvider> ocrProvider_;
    bool open_ = false;
    QString documentId_;
    QString title_;
    QString filePath_;
    QString format_;
    QString contentType_;
    QString textContent_;
    QString continuousTextContent_;
    QString imageSource_;
    QString engineName_;
    QString errorMessage_;
    QString ocrStatus_;
    QString ocrDocumentId_;
    QString ocrLanguage_;
    int ocrPage_ = -1;
    bool ocrDocumentRunning_ = false;
    QString ocrBatchDocumentId_;
    QString ocrBatchLanguage_;
    QList<int> ocrBatchPages_;
    int ocrBatchIndex_ = 0;
    int ocrBatchSucceeded_ = 0;
    int ocrBatchFailed_ = 0;
    bool showOcrText_ = false;
    int pageCount_ = 0;
    int currentPage_ = 0;
    int pageRotation_ = 0;
    mutable QHash<int, QString> thumbnailCache_;
    mutable QHash<int, QString> pageImageCache_;
    int thumbnailRevision_ = 0;
    QString searchQuery_;
    QList<int> searchStarts_;
    int searchCurrent_ = -1;
    int searchStart_ = -1;
    int searchLength_ = 0;
    QElapsedTimer readingTimer_;

};

} // namespace Lumina::Application
