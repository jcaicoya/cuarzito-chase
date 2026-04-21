#pragma once
#include <QGraphicsScene>
#include <QElapsedTimer>
#include <QList>
#include <QTimer>
#include "InputManager.h"

class QGraphicsTextItem;

enum class GameState { Attract, Playing, GameOver };

class GameScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit GameScene(QObject *parent = nullptr);
    InputManager *inputManager() { return &m_input; }

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private slots:
    void tick();

private:
    // ---------------------------------------------------------------
    // Scene constants
    // ---------------------------------------------------------------
    static constexpr float SCENE_W = 1280.f;
    static constexpr float SCENE_H = 720.f;
    static constexpr float CX      = SCENE_W / 2.f;   // screen center X
    static constexpr float CY      = SCENE_H / 2.f;   // screen center Y
    static constexpr float FOCAL   = 400.f;
    static constexpr float SPAWN_Z = 900.f;
    static constexpr float REMOVE_Z  = 25.f;
    static constexpr float COLLIDE_Z = 460.f;

    // Player movement speed and tunnel bounds (screen-space px relative to VP)
    static constexpr float PLAYER_SPEED  = 320.f;
    static constexpr float TUNNEL_HALF_W = 220.f;   // gameplay wall boundary
    static constexpr float TUNNEL_HALF_H = 152.f;

    // Visual tunnel opening at the near plane (slightly larger than gameplay bounds)
    static constexpr float WALL_NEAR_HW = 238.f;
    static constexpr float WALL_NEAR_HH = 165.f;

    // Near-plane Z used to derive world-space tunnel dimensions
    static constexpr float NEAR_Z = 60.f;

    // ---------------------------------------------------------------
    // Entity structs
    // ---------------------------------------------------------------
    struct Player {
        float offX = 0.f;   // screen-space offset from VP (tunnel center)
        float offY = 0.f;
    };

    struct Obstacle {
        float wx, wy, wz;
        float wHalfW, wHalfH;
    };

    struct Collectible {
        float wx, wy, wz;
        float wRadius;
        int   value;
        bool  special;
    };

    struct ScorePopup {
        float sx, sy;
        int   value;
        float life;
    };

    struct Spark {
        float wx, wy, wz;
        float speed;
    };

    struct Star {
        float x, y, r, bright;
    };

    // ---------------------------------------------------------------
    // Projection helpers — non-static, use moving vanishing point
    // ---------------------------------------------------------------
    float projX(float wx, float wz) const { return m_vpX + wx * FOCAL / wz; }
    float projY(float wy, float wz) const { return m_vpY + wy * FOCAL / wz; }
    static float projScale(float wz)      { return FOCAL / wz; }

    // Player absolute screen position (derived from VP + offset)
    float playerSX() const { return m_vpX + m_player.offX; }
    float playerSY() const { return m_vpY + m_player.offY; }

    // Tunnel projected half-widths at a given world Z
    static float tunnelProjHW(float z) { return WALL_NEAR_HW * NEAR_Z / z; }
    static float tunnelProjHH(float z) { return WALL_NEAR_HH * NEAR_Z / z; }

    // ---------------------------------------------------------------
    // Game flow
    // ---------------------------------------------------------------
    void startGame();
    void endGame();
    void spawnObstacle();
    void spawnCollectible();
    void initSparks();
    void initStars();
    void advanceSparks(float dt, float speedMult = 1.f);
    void updateVP(float dt);

    void updateAttract(float dt);
    void updatePlaying(float dt);
    void updateGameOver(float dt);
    void updateHUD();
    void setOverlay(const QString &text);

    // ---------------------------------------------------------------
    // Rendering passes
    // ---------------------------------------------------------------
    void drawEnvironment(QPainter *p) const;
    void drawSparks(QPainter *p) const;
    void drawCollectibles(QPainter *p) const;
    void drawObstacles(QPainter *p) const;
    void drawPlayer(QPainter *p) const;
    void drawPopups(QPainter *p) const;

    // ---------------------------------------------------------------
    // State
    // ---------------------------------------------------------------
    GameState    m_state = GameState::Attract;
    InputManager m_input;
    Player       m_player;

    QList<Obstacle>    m_obstacles;
    QList<Collectible> m_collectibles;
    QList<ScorePopup>  m_popups;
    QList<Spark>       m_sparks;
    QList<Star>        m_stars;

    QTimer        m_timer;
    QElapsedTimer m_clock;
    qint64        m_lastMs = 0;

    // Vanishing point (moves as the tunnel curves)
    float m_vpX  = CX;
    float m_vpY  = CY;
    float m_time = 0.f;

    float m_worldSpeed      = 220.f;
    float m_spawnTimer      = 1.5f;
    float m_spawnInterval   = 2.0f;
    float m_collectTimer    = 1.0f;
    float m_collectInterval = 1.0f;
    float m_survivalTime    = 0.f;
    float m_score           = 0.f;
    float m_gameOverTimer   = 0.f;

    QGraphicsTextItem *m_hudText     = nullptr;
    QGraphicsTextItem *m_overlayText = nullptr;
};
