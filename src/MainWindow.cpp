#include "MainWindow.h"
#include "GameConstants.h"
#include "GameLaunchOptions.h"
#include "GameWidget.h"

MainWindow::MainWindow(const GameLaunchOptions &options, QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Cuarzito");
    resize(static_cast<int>(kLogicalW), static_cast<int>(kLogicalH));

    auto *game = new GameWidget(options, this);
    setCentralWidget(game);
    game->setFocus();
}
