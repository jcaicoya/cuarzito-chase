#pragma once

// Shared project-wide constants. Include this wherever any of these values
// are needed — never hardcode 1280/720/400/etc. directly in source files.

// Logical (authored) resolution. All game geometry is designed for this frame.
// GameWidget letterboxes/pillarboxes to preserve it on any physical display.
constexpr float kLogicalW  = 1280.f;
constexpr float kLogicalH  = 720.f;
constexpr float kLogicalCX = kLogicalW / 2.f;  // 640
constexpr float kLogicalCY = kLogicalH / 2.f;  // 360

// Perspective focal length. Must be identical in GameScene and CaveRenderer.
constexpr float kFocal = 400.f;

// Game loop
constexpr int   kFrameIntervalMs = 16;      // ~62.5 fps
constexpr float kMaxDeltaTime    = 0.05f;   // clamp to avoid spiral-of-death

// Leaderboard
constexpr int kMaxScoreEntries = 10;
constexpr int kMaxNameLength   = 3;
