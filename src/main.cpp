#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QScreen>
#include <QSurfaceFormat>
#include <QWindow>
#include <csignal>
#include "GameLaunchOptions.h"
#include "MainWindow.h"

static void handleSignal(int) { QApplication::quit(); }

int main(int argc, char *argv[])
{
    std::signal(SIGINT,  handleSignal);
    std::signal(SIGTERM, handleSignal);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Cuarzito pre-show game");
    parser.addHelpOption();

    QCommandLineOption testOption(QStringList() << "test",
                                  "Run a named automated test track.",
                                  "name");
    parser.addOption(testOption);
    QCommandLineOption demoOption(QStringList() << "demo",
                                  "Launch in demo mode.");
    parser.addOption(demoOption);
    QCommandLineOption liveOption(QStringList() << "live",
                                  "Launch in live mode.");
    parser.addOption(liveOption);
    QCommandLineOption fullscreenOption(QStringList() << "fullscreen",
                                        "Launch in fullscreen mode.");
    parser.addOption(fullscreenOption);
    QCommandLineOption screenOption(QStringList() << "screen",
                                    "Target screen index for fullscreen mode.",
                                    "index");
    parser.addOption(screenOption);
    parser.process(a);

    GameLaunchOptions options;
    if (parser.isSet(testOption)) {
        const QString testName = parser.value(testOption).trimmed().toLower();
        if (testName == "downhill")
            options.trackResource = QStringLiteral(":/tracks/tests/downhill.json");
        else if (testName == "uphill")
            options.trackResource = QStringLiteral(":/tracks/tests/uphill.json");
        else if (testName == "vertical")
            options.trackResource = QStringLiteral(":/tracks/tests/vertical.json");

        if (!testName.isEmpty()) {
            options.testMode = true;
            options.traceMode = true;
            options.testName = testName;
        }
    }

    MainWindow w(options);
    bool wantFullscreen = parser.isSet(fullscreenOption) || !parser.isSet(screenOption);
    if (parser.isSet(screenOption)) {
        bool ok = false;
        int screenIndex = parser.value(screenOption).toInt(&ok);
        if (ok) {
            const auto screens = QGuiApplication::screens();
            if (screenIndex >= 0 && screenIndex < screens.size()) {
                if (QWindow* handle = w.windowHandle())
                    handle->setScreen(screens[screenIndex]);
                else {
                    w.winId();
                    if (QWindow* createdHandle = w.windowHandle())
                        createdHandle->setScreen(screens[screenIndex]);
                }
                w.setGeometry(screens[screenIndex]->geometry());
            }
        }
    }

    if (wantFullscreen)
        w.showFullScreen();
    else
        w.show();
    return a.exec();
}
