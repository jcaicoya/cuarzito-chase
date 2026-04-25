#pragma once

#include <QList>
#include <QString>

#include "HighScoreManager.h"

class QPainter;

namespace HudRenderer {

void drawTopScores(QPainter *painter,
                   const QList<HighScoreManager::Entry> &entries,
                   float x,
                   float y,
                   int maxRows);

void drawHighScoreEntry(QPainter *painter,
                        const QString &initials,
                        int initialIndex);

} // namespace HudRenderer
