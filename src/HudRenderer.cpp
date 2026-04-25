#include "HudRenderer.h"

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRadialGradient>
#include <QRectF>
#include <Qt>
#include <QtGlobal>

#include "GameConstants.h"

namespace HudRenderer {

void drawTopScores(QPainter *painter,
                   const QList<HighScoreManager::Entry> &entries,
                   float x,
                   float y,
                   int maxRows)
{
    painter->save();

    QFont titleFont("Courier New", 18, QFont::Bold);
    painter->setFont(titleFont);
    painter->setPen(QColor(110, 255, 210, 230));
    painter->drawText(QPointF(x, y), "TOP SCORES");

    QFont rowFont("Courier New", 15, QFont::Bold);
    painter->setFont(rowFont);
    painter->setPen(QColor(225, 245, 255, 215));

    const int rows = qMin(maxRows, entries.size());
    for (int i = 0; i < rows; ++i) {
        const auto &entry = entries[i];
        const QString row = QString("%1. %2  %3")
            .arg(i + 1, 2)
            .arg(entry.name.leftJustified(3, ' '))
            .arg(entry.score, 5);
        painter->drawText(QPointF(x, y + 32.f + i * 25.f), row);
    }

    painter->restore();
}

void drawHighScoreEntry(QPainter *painter,
                        const QString &initials,
                        int initialIndex)
{
    painter->save();

    const float centerX = kLogicalW * 0.5f;
    const float y = kLogicalH * 0.48f;
    const float spacing = 76.f;
    const float startX = centerX - spacing;

    QFont letterFont("Courier New", 46, QFont::Bold);
    painter->setFont(letterFont);
    painter->setPen(QColor(240, 255, 248));

    for (int i = 0; i < 3; ++i) {
        const float x = startX + spacing * i;
        QRectF letterRect(x - 30.f, y - 46.f, 60.f, 58.f);
        painter->drawText(letterRect, Qt::AlignCenter, QString(initials[i]));

        if (i == initialIndex) {
            painter->setPen(Qt::NoPen);
            QRadialGradient glow(x, y + 25.f, 32.f);
            glow.setColorAt(0.0, QColor(100, 255, 150, 115));
            glow.setColorAt(1.0, QColor(0, 0, 0, 0));
            painter->setBrush(glow);
            painter->drawEllipse(QPointF(x, y + 25.f), 34.f, 16.f);

            painter->setPen(QPen(QColor(100, 255, 150, 240), 4.f));
            painter->drawLine(QPointF(x - 22.f, y + 25.f), QPointF(x + 22.f, y + 25.f));
            painter->setPen(QColor(240, 255, 248));
        }
    }

    painter->restore();
}

} // namespace HudRenderer
