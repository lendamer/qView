#ifndef QVIMAGECORE_H
#define QVIMAGECORE_H

#include <QObject>
#include <QImageReader>
#include <QPixmap>
#include <QMovie>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QTimer>
#include <QCache>
#include <QElapsedTimer>
#include <QPointer>

class QMediaPlayer;
class QAudioOutput;
class QGraphicsVideoItem;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#  include <QColorSpace>
#else
typedef QString QColorSpace;
#endif

class QVImageCore : public QObject
{
    Q_OBJECT

public:
    struct CompatibleFile
    {
        QString absoluteFilePath;
        QString fileName;

        // Only populated if needed for sorting
        qint64 lastModified;
        qint64 lastCreated;
        qint64 size;
        QString mimeType;
    };

    struct ErrorData
    {
        bool hasError = false;
        int errorNum = 0;
        QString errorString;
    };

    struct FileDetails
    {
        QFileInfo fileInfo;
        QList<CompatibleFile> folderFileInfoList;
        int loadedIndexInFolder = -1;
        bool isLoadRequested = false;
        bool isPixmapLoaded = false;
        bool isMovieLoaded = false;
        bool isVideoLoaded = false;
        QSize baseImageSize;
        QSize loadedPixmapSize;
        QElapsedTimer timeSinceLoaded;
        ErrorData errorData;

        void updateLoadedIndexInFolder();
    };

    struct DirInfo
    {
        QString dirPath;
        qsizetype fileCount;
        int sortMode;
        bool sortDescending;

        bool operator!=(const DirInfo &other) const
        {
            return dirPath != other.dirPath || fileCount != other.fileCount
                    || sortMode != other.sortMode || sortDescending != other.sortDescending;
        }
    };

    struct ReadData
    {
        QImage image;
        QString absoluteFilePath;
        qint64 fileSize;
        QSize imageSize;
        QColorSpace targetColorSpace;
        ErrorData errorData;
    };

    explicit QVImageCore(QObject *parent = nullptr);
    ~QVImageCore() override;

    void loadFile(const QString &fileName, bool isReloading = false);
    ReadData readFile(const QString &fileName, const QColorSpace &targetColorSpace);
    void loadPixmap(const ReadData &readData);
    void closeImage();
    QList<CompatibleFile> getCompatibleFiles(const QString &dirPath) const;
    void updateFolderInfo(QString dirPath = QString());
    void requestCaching();
    void requestCachingFile(const QString &filePath, const QColorSpace &targetColorSpace);
    void addToCache(const ReadData &&readImageAndFileInfo);
    static QString getPixmapCacheKey(const QString &absoluteFilePath, const qint64 &fileSize,
                                     const QColorSpace &targetColorSpace);
    QColorSpace getTargetColorSpace() const;
    QColorSpace detectDisplayColorSpace() const;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0) && QT_VERSION < QT_VERSION_CHECK(6, 7, 2)
    static bool removeTinyDataTagsFromIccProfile(QByteArray &profile);
#endif

    void settingsUpdated();

    void jumpToNextFrame();
    void setPaused(bool desiredState);
    void setSpeed(int desiredSpeed);
    bool isAnimated() const;
    bool isPaused() const;
    int getSpeed() const;
    void setMuted(bool muted);
    bool isMuted() const;
    void releaseVideo();
    void setVideoOutput(QGraphicsVideoItem *output);

    void rotateImage(int rotation);
    QImage matchCurrentRotation(const QImage &imageToRotate);
    QPixmap matchCurrentRotation(const QPixmap &pixmapToRotate);

    QPixmap scaleExpensively(const int desiredWidth, const int desiredHeight);
    QPixmap scaleExpensively(const QSizeF desiredSize);

    // returned const reference is read-only
    const QPixmap &getLoadedPixmap() const { return loadedPixmap; }
    const QMovie &getLoadedMovie() const { return loadedMovie; }
    const FileDetails &getCurrentFileDetails() const { return currentFileDetails; }
    int getCurrentRotation() const { return currentRotation; }
    int getVideoPresentationRotation() const { return videoPresentationRotation; }

signals:
    void animatedFrameChanged(QRect rect);

    void updateLoadedPixmapItem();

    void fileChanged();

protected:
    void loadEmptyPixmap();
    FileDetails getEmptyFileDetails();

private:
    QPixmap loadedPixmap;
    QMovie loadedMovie;

    FileDetails currentFileDetails;
    int currentRotation;

    void loadVideo(const QString &fileName, quint64 requestId);
    void updateVideoGeometry(QMediaPlayer *player, quint64 requestId);
    bool isCurrentVideo(QMediaPlayer *player, quint64 requestId) const;
    static bool isVideoFile(const QString &fileName);

    QPointer<QGraphicsVideoItem> videoOutput;
    QMediaPlayer *loadedVideo = nullptr;
    QAudioOutput *audioOutput = nullptr;
    QList<QFutureWatcher<ReadData> *> readWatchers;
    quint64 loadRequestId = 0;
    int videoPresentationRotation = 0;

    int colorSpaceConversion = -1;

    static QCache<QString, ReadData> imageCache;

    DirInfo lastDirInfo;
    unsigned randomSortSeed = 0;

    QStringList lastFilesPreloaded;
    QStringList preloadFilesInProgress;

    int largestDimension;

};

#endif // QVIMAGECORE_H
