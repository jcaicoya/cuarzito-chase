#include "InputManager.h"

#include <QDebug>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <Xinput.h>
#include "SdlControllerBackend.h"
#endif

InputManager::InputManager(QObject *parent) : QObject(parent) {}

void InputManager::keyPressed(Qt::Key key)
{
    m_held.insert(key);
    if (hasActionForKey(key)) {
        const QSet<Action> actions = actionsForKey(key);
        for (Action action : actions) {
            m_heldActions.insert(action);
            m_pressedActions.insert(action);
        }
    }
}

void InputManager::keyReleased(Qt::Key key)
{
    m_held.remove(key);
    rebuildHeldActions();
}

bool InputManager::isHeld(Action action) const
{
    return m_heldActions.contains(action) ||
           m_gamepadHeldActions.contains(action) ||
           m_sdlHeldActions.contains(action);
}

bool InputManager::isJustPressed(Action action) const
{
    return m_pressedActions.contains(action);
}

bool InputManager::isMovingLeft() const
{
    return isHeld(Action::MoveLeft);
}

bool InputManager::isMovingRight() const
{
    return isHeld(Action::MoveRight);
}

bool InputManager::isMovingUp() const
{
    return isHeld(Action::MoveUp);
}

bool InputManager::isMovingDown() const
{
    return isHeld(Action::MoveDown);
}

bool InputManager::isAccelerating() const
{
    return isHeld(Action::Accelerate);
}

bool InputManager::isBraking() const
{
    return isHeld(Action::Brake);
}

bool InputManager::isConfirmJustPressed() const
{
    return isJustPressed(Action::Confirm);
}

bool InputManager::isLeftJustPressed() const
{
    return isJustPressed(Action::MoveLeft);
}

bool InputManager::isRightJustPressed() const
{
    return isJustPressed(Action::MoveRight);
}

bool InputManager::isUpJustPressed() const
{
    return isJustPressed(Action::MoveUp);
}

bool InputManager::isDownJustPressed() const
{
    return isJustPressed(Action::MoveDown);
}

void InputManager::endFrame()
{
    m_pressedActions.clear();
}

void InputManager::rebuildHeldActions()
{
    m_heldActions.clear();
    for (Qt::Key key : m_held) {
        if (!hasActionForKey(key))
            continue;

        const QSet<Action> actions = actionsForKey(key);
        for (Action action : actions)
            m_heldActions.insert(action);
    }
}

void InputManager::updateGamepad()
{
    QSet<Action> newSdlHeld;
#ifdef Q_OS_WIN
    static bool s_logged = false;
    auto &sdl2 = SdlControllerBackend::instance();
    newSdlHeld = sdl2.poll();
#endif

    for (Action action : newSdlHeld) {
        if (!m_sdlHeldActions.contains(action))
            m_pressedActions.insert(action);
    }
    m_sdlHeldActions = newSdlHeld;

#ifdef Q_OS_WIN
    using XInputGetStateFn = DWORD (WINAPI *)(DWORD, XINPUT_STATE *);

    static HMODULE module = []() -> HMODULE {
        const char *dlls[] = {"xinput1_4.dll", "xinput9_1_0.dll", "xinput1_3.dll"};
        for (const char *dll : dlls) {
            if (HMODULE handle = LoadLibraryA(dll))
                return handle;
        }
        return nullptr;
    }();

    static XInputGetStateFn getState = module
        ? reinterpret_cast<XInputGetStateFn>(GetProcAddress(module, "XInputGetState"))
        : nullptr;

    QSet<Action> newHeld;
    if (getState) {
        XINPUT_STATE state = {};
        for (DWORD index = 0; index < XUSER_MAX_COUNT; ++index) {
            if (getState(index, &state) != ERROR_SUCCESS)
                continue;

            const WORD buttons = state.Gamepad.wButtons;
            const SHORT lx = state.Gamepad.sThumbLX;
            const SHORT ly = state.Gamepad.sThumbLY;
            constexpr SHORT deadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

            if ((buttons & XINPUT_GAMEPAD_DPAD_LEFT) || lx < -deadZone)
                newHeld.insert(Action::MoveLeft);
            if ((buttons & XINPUT_GAMEPAD_DPAD_RIGHT) || lx > deadZone)
                newHeld.insert(Action::MoveRight);
            if ((buttons & XINPUT_GAMEPAD_DPAD_UP) || ly > deadZone)
                newHeld.insert(Action::MoveUp);
            if ((buttons & XINPUT_GAMEPAD_DPAD_DOWN) || ly < -deadZone)
                newHeld.insert(Action::MoveDown);
            if ((buttons & XINPUT_GAMEPAD_A) || (buttons & XINPUT_GAMEPAD_START))
                newHeld.insert(Action::Confirm);
            if ((buttons & XINPUT_GAMEPAD_A) || state.Gamepad.bRightTrigger > 30)
                newHeld.insert(Action::Accelerate);
            if ((buttons & XINPUT_GAMEPAD_B) || (buttons & XINPUT_GAMEPAD_BACK))
                newHeld.insert(Action::Cancel);
            if ((buttons & XINPUT_GAMEPAD_B) || state.Gamepad.bLeftTrigger > 30)
                newHeld.insert(Action::Brake);

            break;
        }
    }

    for (Action action : newHeld) {
        if (!m_gamepadHeldActions.contains(action))
            m_pressedActions.insert(action);
    }
    m_gamepadHeldActions = newHeld;

    if (!s_logged) {
        s_logged = true;
        qDebug().noquote() << gamepadDiagnostics();
    }
#endif
}

QString InputManager::gamepadDiagnostics() const
{
    QString report;
#ifdef Q_OS_WIN
    // SDL3 section
    report += SdlControllerBackend::instance().diagnostics();

    // XInput section
    using XInputGetStateFn = DWORD (WINAPI *)(DWORD, XINPUT_STATE *);
    static HMODULE xiModule = []() -> HMODULE {
        const char *dlls[] = {"xinput1_4.dll", "xinput9_1_0.dll", "xinput1_3.dll"};
        for (const char *dll : dlls)
            if (HMODULE h = LoadLibraryA(dll)) return h;
        return nullptr;
    }();
    static XInputGetStateFn xiGetState = xiModule
        ? reinterpret_cast<XInputGetStateFn>(GetProcAddress(xiModule, "XInputGetState"))
        : nullptr;

    report += "\n=== XInput ===\n";
    if (!xiModule) {
        report += "XInput DLL: NOT FOUND\n";
    } else {
        int connected = 0;
        if (xiGetState) {
            for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
                XINPUT_STATE state{};
                if (xiGetState(i, &state) == ERROR_SUCCESS)
                    ++connected;
            }
        }
        report += QString("XInput controllers connected: %1\n").arg(connected);
        report += "(DualSense is not XInput — 0 is expected for PS5 controllers)\n";
    }
#else
    report = "Gamepad diagnostics: Windows only\n";
#endif
    return report;
}

bool InputManager::hasActionForKey(Qt::Key key)
{
    switch (key) {
    case Qt::Key_Left:
    case Qt::Key_A:
    case Qt::Key_Right:
    case Qt::Key_D:
    case Qt::Key_Up:
    case Qt::Key_W:
    case Qt::Key_Down:
    case Qt::Key_S:
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Escape:
    case Qt::Key_R:
    case Qt::Key_F11:
        return true;
    default:
        return false;
    }
}

QSet<Action> InputManager::actionsForKey(Qt::Key key)
{
    switch (key) {
    case Qt::Key_Left:
    case Qt::Key_A:
        return {Action::MoveLeft};
    case Qt::Key_Right:
    case Qt::Key_D:
        return {Action::MoveRight};
    case Qt::Key_Up:
    case Qt::Key_W:
        return {Action::MoveUp};
    case Qt::Key_Down:
    case Qt::Key_S:
        return {Action::MoveDown};
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return {Action::Confirm, Action::Accelerate};
    case Qt::Key_Shift:
    case Qt::Key_Control:
        return {Action::Brake};
    case Qt::Key_Escape:
        return {Action::Cancel};
    case Qt::Key_R:
        return {Action::Restart};
    case Qt::Key_F11:
        return {Action::Fullscreen};
    default:
        return {};
    }
}
