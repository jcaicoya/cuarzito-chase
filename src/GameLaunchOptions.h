#pragma once

#include <QString>

struct GameLaunchOptions {
    bool testMode = false;
    bool traceMode = false;
    QString testName;
    QString trackResource = QStringLiteral(":/tracks/demo_tunnel.json");
};
