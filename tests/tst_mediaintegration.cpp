#include <QtTest>
#include <QSettings>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>

#include "qvrenamedialog.h"

#include "qvapplication.h"
#include "qvimagecore.h"

class MediaIntegrationTests : public QObject
{
    Q_OBJECT

private slots:
    void mixedFolderIncludesUppercaseMp4();
    void mixedFolderNavigationAndCorruptRecovery();
    void videoActionsDispatchAndReset();
    void gifAndStillActionsRemainCorrect();
    void failedVideoRenameResumesOriginal();
};

void MediaIntegrationTests::mixedFolderIncludesUppercaseMp4()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    for (const QString &name : {QStringLiteral("a.png"), QStringLiteral("b.MP4"),
                               QStringLiteral("c.txt")}) {
        QFile file(directory.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("fixture");
    }

    QVImageCore core;
    const auto files = core.getCompatibleFiles(directory.path());
    QCOMPARE(files.size(), 2);
    QStringList names;
    for (const auto &file : files)
        names << file.fileName;
    QVERIFY(names.contains(QStringLiteral("a.png")));
    QVERIFY(names.contains(QStringLiteral("b.MP4")));
}

void MediaIntegrationTests::mixedFolderNavigationAndCorruptRecovery()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(QFile::copy(QStringLiteral(TEST_FIXTURE_DIR "/still.png"), directory.filePath("01.png")));
    QVERIFY(QFile::copy(QStringLiteral(TEST_FIXTURE_DIR "/loop.mp4"), directory.filePath("02.MP4")));
    QVERIFY(QFile::copy(QStringLiteral(TEST_FIXTURE_DIR "/corrupt.mp4"), directory.filePath("03.mp4")));
    QVERIFY(QFile::copy(QStringLiteral(TEST_FIXTURE_DIR "/animated.gif"), directory.filePath("04.gif")));

    MainWindow window;
    window.setAttribute(Qt::WA_DeleteOnClose, false);
    window.show();
    window.openFile(directory.filePath("01.png"));
    QTRY_VERIFY(window.getCurrentFileDetails().isPixmapLoaded);
    QCOMPARE(window.getCurrentFileDetails().folderFileInfoList.size(), 4);
    QCOMPARE(window.getCurrentFileDetails().loadedIndexInFolder, 0);
    window.nextFile();
    QTRY_VERIFY_WITH_TIMEOUT(window.getCurrentFileDetails().isVideoLoaded, 10000);
    QCOMPARE(window.getCurrentFileDetails().fileInfo.fileName(), QStringLiteral("02.MP4"));
    QCOMPARE(window.getCurrentFileDetails().loadedIndexInFolder, 1);
    window.nextFile();
    QTRY_VERIFY_WITH_TIMEOUT(window.getCurrentFileDetails().errorData.hasError, 10000);
    QCOMPARE(window.getCurrentFileDetails().loadedIndexInFolder, 2);
    window.nextFile();
    QTRY_VERIFY(window.getCurrentFileDetails().isMovieLoaded);
    QCOMPARE(window.getCurrentFileDetails().fileInfo.fileName(), QStringLiteral("04.gif"));
    window.previousFile();
    QTRY_VERIFY_WITH_TIMEOUT(window.getCurrentFileDetails().errorData.hasError, 10000);
    window.previousFile();
    QTRY_VERIFY_WITH_TIMEOUT(window.getCurrentFileDetails().isVideoLoaded, 10000);
    window.close();
}

void MediaIntegrationTests::videoActionsDispatchAndReset()
{
    MainWindow window;
    window.setAttribute(Qt::WA_DeleteOnClose, false);
    window.show();
    auto *view = window.findChild<QVGraphicsView *>();
    QVERIFY(view);
    auto &actions = qvApp->getActionManager();
    const auto clones = [&](const QString &key) { return actions.getAllClonesOfAction(key, &window); };
    const auto dispatch = [&](const QString &key) {
        const auto instances = clones(key);
        QVERIFY(!instances.isEmpty());
        QVERIFY(instances.first()->isEnabled());
        ActionManager::actionTriggered(instances.first(), &window);
    };
    window.openFile(QStringLiteral(TEST_FIXTURE_DIR "/audio.mp4"));
    QTRY_VERIFY_WITH_TIMEOUT(window.getCurrentFileDetails().isVideoLoaded, 10000);
    QVERIFY(view->isMuted());
    QVERIFY(!view->isPaused());
    QVERIFY(!clones("saveframeas").first()->isEnabled());
    QVERIFY(!clones("nextframe").first()->isEnabled());
    QVERIFY(clones("mute").first()->shortcuts().contains(QKeySequence(Qt::Key_M)));
    dispatch("pause");
    QTRY_VERIFY(view->isPaused());
    QVERIFY(clones("pause").first()->text().remove('&').contains("Resume"));
    dispatch("increasespeed");
    QCOMPARE(view->getSpeed(), 125);
    dispatch("decreasespeed");
    QCOMPARE(view->getSpeed(), 100);
    dispatch("increasespeed");
    dispatch("resetspeed");
    QCOMPARE(view->getSpeed(), 100);
    dispatch("mute");
    QVERIFY(!view->isMuted());
    QVERIFY(clones("mute").first()->text().remove('&').contains("Mute"));
    QApplication::clipboard()->clear();
    window.copy();
    QVERIFY(QApplication::clipboard()->mimeData()->hasUrls());
    QVERIFY(!QApplication::clipboard()->mimeData()->hasImage());
    QCOMPARE(QApplication::clipboard()->mimeData()->urls().first().toLocalFile(),
             QStringLiteral(TEST_FIXTURE_DIR "/audio.mp4"));

    window.openFile(QStringLiteral(TEST_FIXTURE_DIR "/loop.mp4"));
    QTRY_VERIFY_WITH_TIMEOUT(window.getCurrentFileDetails().isVideoLoaded, 10000);
    QVERIFY(!view->isPaused());
    QVERIFY(view->isMuted());
    QCOMPARE(view->getSpeed(), 100);
    QVERIFY(clones("pause").first()->text().remove('&').contains("Pause"));
    QVERIFY(clones("mute").first()->text().remove('&').contains("Unmute"));
    window.close();
}

void MediaIntegrationTests::gifAndStillActionsRemainCorrect()
{
    MainWindow window;
    window.setAttribute(Qt::WA_DeleteOnClose, false);
    window.show();
    auto *view = window.findChild<QVGraphicsView *>();
    QVERIFY(view);
    const auto clones = [&](const QString &key) {
        return qvApp->getActionManager().getAllClonesOfAction(key, &window);
    };
    window.openFile(QStringLiteral(TEST_FIXTURE_DIR "/animated.gif"));
    QTRY_VERIFY(window.getCurrentFileDetails().isMovieLoaded);
    QVERIFY(clones("pause").first()->isEnabled());
    QVERIFY(clones("nextframe").first()->isEnabled());
    QVERIFY(clones("saveframeas").first()->isEnabled());
    QVERIFY(!clones("mute").first()->isEnabled());
    window.pause();
    QVERIFY(view->isPaused());
    window.increaseSpeed();
    QCOMPARE(view->getSpeed(), 125);
    const int frame = view->getLoadedMovie().currentFrameNumber();
    window.nextFrame();
    QVERIFY(view->getLoadedMovie().currentFrameNumber() != frame);
    window.saveFrameAs();
    auto *saveDialog = window.findChild<QFileDialog *>();
    QVERIFY(saveDialog);
    QVERIFY(!saveDialog->nameFilters().join(' ').contains("*.mp4"));
    QVERIFY(!saveDialog->nameFilters().join(' ').contains("(*)"));
    saveDialog->reject();

    window.openFile(QStringLiteral(TEST_FIXTURE_DIR "/still.png"));
    QTRY_VERIFY(window.getCurrentFileDetails().isPixmapLoaded);
    QVERIFY(!window.getCurrentFileDetails().isMovieLoaded);
    QVERIFY(!window.getCurrentFileDetails().isVideoLoaded);
    QCOMPARE(window.getCurrentFileDetails().baseImageSize, QSize(80, 60));
    for (const QString &key : {QStringLiteral("pause"), QStringLiteral("increasespeed"),
                               QStringLiteral("mute"), QStringLiteral("saveframeas"),
                               QStringLiteral("nextframe")})
        QVERIFY(!clones(key).first()->isEnabled());
    window.close();
}

void MediaIntegrationTests::failedVideoRenameResumesOriginal()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString original = directory.filePath("original.mp4");
    const QString existing = directory.filePath("existing.mp4");
    QVERIFY(QFile::copy(QStringLiteral(TEST_FIXTURE_DIR "/loop.mp4"), original));
    QVERIFY(QFile::copy(QStringLiteral(TEST_FIXTURE_DIR "/loop.mp4"), existing));
    MainWindow window;
    window.setAttribute(Qt::WA_DeleteOnClose, false);
    window.show();
    window.openFile(original);
    QTRY_VERIFY_WITH_TIMEOUT(window.getCurrentFileDetails().isVideoLoaded, 10000);
    window.rename();
    auto *dialog = window.findChild<QVRenameDialog *>();
    QVERIFY(dialog);
    dialog->setTextValue("existing.mp4");
    QTimer dismissError;
    connect(&dismissError, &QTimer::timeout, this, [] {
        if (auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
            message->accept();
    });
    dismissError.start(20);
    dialog->accept();
    dismissError.stop();
    QTRY_VERIFY_WITH_TIMEOUT(window.getCurrentFileDetails().isVideoLoaded, 10000);
    QCOMPARE(window.getCurrentFileDetails().fileInfo.absoluteFilePath(), original);
    QVERIFY(QFile::exists(original));
    QVERIFY(QFile::exists(existing));
    window.close();
}

int main(int argc, char *argv[])
{
    QTemporaryDir settingsDirectory;
    if (!settingsDirectory.isValid())
        return 1;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    QCoreApplication::setOrganizationName("qView Loop Tests");
    QCoreApplication::setApplicationName("MediaIntegration");
    QSettings().setValue("options/updatenotifications", false);
    QSettings().setValue("firstlaunch", true);
    QSettings().setValue("options/language", "en");
    QVApplication app(argc, argv);
    MediaIntegrationTests tests;
    const int result = QTest::qExec(&tests, argc, argv);
    QThreadPool::globalInstance()->waitForDone();
    return result;
}

#include "tst_mediaintegration.moc"
