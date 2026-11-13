#include <QtTest>

#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QMediaPlayer>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QVideoSink>
#include <cstring>
#include <memory>

#include "qvapplication.h"
#include "qvgraphicsview.h"

class VideoPlaybackTests : public QObject
{
    Q_OBJECT

private slots:
    void validMp4BecomesReady();
    void videoLoopsAndSupportsPlaybackControls();
    void videoUsesNativeSceneGeometryAndRotation();
    void decodedFrameRotationIsNotAppliedTwice();
    void videoScaleSurvivesLoopBoundary();
    void replacingMediaDiscardsStaleVideo();
    void corruptVideoCanBeReplaced();
    void videoClipboardContainsOnlyItsUrl();
    void gifStillReportsAnimationControls();
};

static QString fixture(const QString &name)
{
    return QStringLiteral(TEST_FIXTURE_DIR "/") + name;
}

static QGraphicsVideoItem *videoItemIn(const QVGraphicsView &view)
{
    for (QGraphicsItem *item : view.scene()->items()) {
        if (auto *videoItem = dynamic_cast<QGraphicsVideoItem *>(item))
            return videoItem;
    }
    return nullptr;
}

void VideoPlaybackTests::validMp4BecomesReady()
{
    QVGraphicsView view;
    view.resize(640, 480);
    view.show();
    view.loadFile(fixture(QStringLiteral("loop.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().baseImageSize == QSize(320, 180), 10000);
    QVERIFY(view.getCurrentFileDetails().isVideoLoaded);
    QVERIFY(view.getCurrentFileDetails().isPixmapLoaded);
    QVERIFY(!view.getCurrentFileDetails().errorData.hasError);
    QVERIFY(view.isAnimated());
}

void VideoPlaybackTests::videoLoopsAndSupportsPlaybackControls()
{
    QVGraphicsView view;
    view.resize(640, 480);
    view.show();
    view.loadFile(fixture(QStringLiteral("loop.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().isVideoLoaded, 10000);

    auto *player = view.findChild<QMediaPlayer *>(QStringLiteral("activeVideoPlayer"));
    QVERIFY(player);
    QCOMPARE(player->loops(), QMediaPlayer::Infinite);
    QVERIFY(view.isMuted());

    view.setPaused(true);
    QTRY_VERIFY_WITH_TIMEOUT(view.isPaused(), 3000);
    view.setSpeed(150);
    QCOMPARE(view.getSpeed(), 150);
    view.setMuted(false);
    QVERIFY(!view.isMuted());

    view.setPaused(false);
    QTRY_VERIFY_WITH_TIMEOUT(!view.isPaused(), 3000);
    view.setSpeed(100);
    const qint64 duration = player->duration();
    QVERIFY(duration > 300);
    player->setPosition(duration - 120);
    QTRY_VERIFY_WITH_TIMEOUT(player->position() > duration / 2, 3000);

    bool reachedEnd = false;
    bool wrapped = false;
    QObject::connect(player, &QMediaPlayer::positionChanged, &view,
                     [&reachedEnd, &wrapped, duration](qint64 position) {
        if (position > duration * 3 / 4)
            reachedEnd = true;
        if (reachedEnd && position < duration / 4)
            wrapped = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(wrapped, 5000);
}

void VideoPlaybackTests::videoUsesNativeSceneGeometryAndRotation()
{
    QVGraphicsView view;
    view.resize(640, 480);
    view.show();
    view.loadFile(fixture(QStringLiteral("portrait.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().isVideoLoaded, 10000);
    QCOMPARE(view.getCurrentFileDetails().baseImageSize, QSize(120, 200));
    QCOMPARE(view.getCurrentFileDetails().loadedPixmapSize, QSize(120, 200));

    auto *videoItem = videoItemIn(view);
    QVERIFY(videoItem);
    QVERIFY(videoItem->isVisible());
    QCOMPARE(videoItem->sceneBoundingRect().size().toSize(), QSize(120, 200));

    view.loadFile(fixture(QStringLiteral("rotated.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().isVideoLoaded, 10000);
    QTRY_COMPARE_WITH_TIMEOUT(view.getCurrentFileDetails().loadedPixmapSize, QSize(180, 320), 10000);
    QCOMPARE(videoItem->sceneBoundingRect().size().toSize(), QSize(180, 320));

    view.rotateImage(90);
    QCOMPARE(view.getCurrentFileDetails().loadedPixmapSize, QSize(320, 180));
    QCOMPARE(videoItem->sceneBoundingRect().size().toSize(), QSize(320, 180));
}

void VideoPlaybackTests::decodedFrameRotationIsNotAppliedTwice()
{
    QVGraphicsView view;
    view.resize(640, 480);
    view.show();
    view.loadFile(fixture(QStringLiteral("loop.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().isVideoLoaded, 10000);

    auto *videoItem = videoItemIn(view);
    QVERIFY(videoItem);
    view.setPaused(true);
    QTRY_VERIFY_WITH_TIMEOUT(view.isPaused(), 3000);

    QVideoFrame frame(QVideoFrameFormat(QSize(320, 180), QVideoFrameFormat::Format_ARGB8888));
    QVERIFY(frame.isValid());
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    frame.setRotation(QtVideo::Rotation::Clockwise90);
#else
    frame.setRotationAngle(QVideoFrame::Rotation90);
#endif
    QVERIFY(frame.map(QVideoFrame::WriteOnly));
    memset(frame.bits(0), 0, size_t(frame.mappedBytes(0)));
    frame.unmap();
    videoItem->videoSink()->setVideoFrame(frame);

    QTRY_COMPARE_WITH_TIMEOUT(view.getCurrentFileDetails().loadedPixmapSize, QSize(180, 320), 3000);
    QCOMPARE(videoItem->rotation(), qreal(0));
    QCOMPARE(videoItem->sceneBoundingRect().size().toSize(), QSize(180, 320));
}

void VideoPlaybackTests::videoScaleSurvivesLoopBoundary()
{
    QVGraphicsView view;
    view.resize(640, 480);
    view.show();
    view.loadFile(fixture(QStringLiteral("loop.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().isVideoLoaded, 10000);

    view.zoom(1.25);
    const QTransform transformBeforeLoop = view.transform();
    auto *player = view.findChild<QMediaPlayer *>(QStringLiteral("activeVideoPlayer"));
    QVERIFY(player);
    const qint64 duration = player->duration();
    QVERIFY(duration > 300);
    player->setPosition(duration - 120);
    QTRY_VERIFY_WITH_TIMEOUT(player->position() > duration / 2, 3000);

    bool reachedEnd = false;
    bool wrapped = false;
    QObject::connect(player, &QMediaPlayer::positionChanged, &view,
                     [&reachedEnd, &wrapped, duration](qint64 position) {
        if (position > duration * 3 / 4)
            reachedEnd = true;
        if (reachedEnd && position < duration / 4)
            wrapped = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(wrapped, 5000);
    QCOMPARE(view.transform(), transformBeforeLoop);
}

void VideoPlaybackTests::replacingMediaDiscardsStaleVideo()
{
    QVGraphicsView view;
    view.resize(640, 480);
    view.show();
    view.loadFile(fixture(QStringLiteral("loop.mp4")));
    view.loadFile(fixture(QStringLiteral("portrait.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().isVideoLoaded, 10000);
    QTRY_COMPARE_WITH_TIMEOUT(view.getCurrentFileDetails().baseImageSize, QSize(120, 200), 10000);
    QCOMPARE(view.getCurrentFileDetails().fileInfo.fileName(), QStringLiteral("portrait.mp4"));

    view.loadFile(fixture(QStringLiteral("still.png")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().isPixmapLoaded, 10000);
    QVERIFY(!view.getCurrentFileDetails().isVideoLoaded);
    QVERIFY(!view.isAnimated());
    QCOMPARE(view.getCurrentFileDetails().baseImageSize, QSize(80, 60));

    view.loadFile(fixture(QStringLiteral("loop.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().isVideoLoaded, 10000);
    QCOMPARE(view.getCurrentFileDetails().fileInfo.fileName(), QStringLiteral("loop.mp4"));
    QTest::qWait(250);
    QCOMPARE(view.getCurrentFileDetails().fileInfo.fileName(), QStringLiteral("loop.mp4"));
    QCOMPARE(view.getCurrentFileDetails().baseImageSize, QSize(320, 180));
}

void VideoPlaybackTests::corruptVideoCanBeReplaced()
{
    QVGraphicsView view;
    view.resize(640, 480);
    view.show();
    view.loadFile(fixture(QStringLiteral("corrupt.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().errorData.hasError, 10000);
    QVERIFY(!view.getCurrentFileDetails().isVideoLoaded);

    view.loadFile(fixture(QStringLiteral("loop.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().isVideoLoaded, 10000);
    QVERIFY(!view.getCurrentFileDetails().errorData.hasError);
}

void VideoPlaybackTests::videoClipboardContainsOnlyItsUrl()
{
    QVGraphicsView view;
    view.resize(640, 480);
    view.show();
    view.loadFile(fixture(QStringLiteral("loop.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().isVideoLoaded, 10000);

    std::unique_ptr<QMimeData> mimeData(view.getMimeData());
    QVERIFY(mimeData->hasUrls());
    QCOMPARE(mimeData->urls().constFirst().toLocalFile(),
             view.getCurrentFileDetails().fileInfo.absoluteFilePath());
    QVERIFY(!mimeData->hasImage());
}

void VideoPlaybackTests::gifStillReportsAnimationControls()
{
    QVGraphicsView view;
    view.loadFile(fixture(QStringLiteral("animated.gif")));
    QTRY_VERIFY_WITH_TIMEOUT(view.getCurrentFileDetails().isMovieLoaded, 10000);
    QVERIFY(view.isAnimated());
    view.setPaused(true);
    QVERIFY(view.isPaused());
    view.setSpeed(150);
    QCOMPARE(view.getSpeed(), 150);
}

int main(int argc, char *argv[])
{
    QVApplication app(argc, argv);
    VideoPlaybackTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_videoplayback.moc"
