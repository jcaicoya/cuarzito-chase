#include "SdlControllerBackend.h"

#ifdef Q_OS_WIN

#include <QByteArray>

#include <cstring>

SdlControllerBackend &SdlControllerBackend::instance()
{
    static SdlControllerBackend inst;
    return inst;
}

QSet<Action> SdlControllerBackend::poll()
{
    QSet<Action> actions;
    if (!m_available) return actions;

    ensureController();
    if (!m_controller) return actions;

    if (m_update) m_update();

    constexpr int    AX_LX = 0, AX_LY = 1, AX_LT = 4, AX_RT = 5;
    constexpr int    BTN_A = 0, BTN_B = 1, BTN_BACK = 4, BTN_START = 6;
    constexpr int    BTN_DU = 11, BTN_DD = 12, BTN_DL = 13, BTN_DR = 14;
    constexpr qint16 DEAD  = 9000;

    const qint16 lx = m_getAxis(m_controller, AX_LX);
    const qint16 ly = m_getAxis(m_controller, AX_LY);
    const qint16 lt = m_getAxis(m_controller, AX_LT);
    const qint16 rt = m_getAxis(m_controller, AX_RT);
    // SDL3 bool returns: mask low byte — upper bytes are not guaranteed zero.
    auto btn = [&](int b){ return (m_getButton(m_controller, b) & 0xFF) != 0; };

    if (btn(BTN_DL) || lx < -DEAD) actions.insert(Action::MoveLeft);
    if (btn(BTN_DR) || lx >  DEAD) actions.insert(Action::MoveRight);
    if (btn(BTN_DU) || ly < -DEAD) actions.insert(Action::MoveUp);
    if (btn(BTN_DD) || ly >  DEAD) actions.insert(Action::MoveDown);
    if (btn(BTN_A)  || btn(BTN_START)) actions.insert(Action::Confirm);
    if (btn(BTN_A)  || rt > DEAD)  actions.insert(Action::Accelerate);
    if (btn(BTN_B)  || btn(BTN_BACK)) actions.insert(Action::Cancel);
    if (btn(BTN_B)  || lt > DEAD)  actions.insert(Action::Brake);

    return actions;
}

// static
QString SdlControllerBackend::regString(HKEY root, const char *subKey, const char *value)
{
    HKEY hKey = nullptr;
    if (RegOpenKeyExA(root, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return {};
    char buf[MAX_PATH] = {};
    DWORD size = MAX_PATH, type = 0;
    const bool ok = RegQueryValueExA(hKey, value, nullptr, &type,
                                     reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return ok ? QString::fromLocal8Bit(buf) : QString{};
}

SdlControllerBackend::SdlControllerBackend()
{
    m_diag += "=== SDL3 Controller Diagnostics ===\n";

    // Find Steam's actual install path via registry
    const QString steam1 = regString(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\WOW6432Node\\Valve\\Steam", "InstallPath");
    const QString steam2 = regString(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Valve\\Steam", "InstallPath");
    const QString steam3 = regString(HKEY_CURRENT_USER,
        "SOFTWARE\\Valve\\Steam", "SteamPath");

    char exeDir[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exeDir, MAX_PATH);
    if (char *sl = strrchr(exeDir, '\\')) *(sl + 1) = '\0';

    QStringList candidates;
    auto addDir = [&](const QString &dir) {
        if (dir.isEmpty()) return;
        const QString d = dir.endsWith('\\') ? dir : dir + "\\";
        candidates << d + "SDL3.dll";
    };
    addDir(QString::fromLocal8Bit(exeDir));
    candidates << "SDL3.dll";   // PATH lookup
    addDir(steam1);
    addDir(steam2);
    addDir(steam3);
    addDir("C:\\Program Files (x86)\\Steam");
    addDir("C:\\Program Files\\Steam");

    static QByteArray foundBuf;
    for (const QString &p : candidates) {
        foundBuf = p.toLocal8Bit();
        m_module = LoadLibraryA(foundBuf.constData());
        if (m_module) break;
        m_diag += QString("  not found: %1\n").arg(p);
    }

    if (!m_module) {
        m_diag += "SDL3.dll: NOT FOUND\n";
        m_diag += "Fix: SDL3.dll is in libs/ and should be copied next to the exe by CMake.\n";
        return;
    }
    m_diag += QString("Loaded: %1\n").arg(QString::fromLocal8Bit(foundBuf));

    m_init         = load<InitFn>        ("SDL_InitSubSystem");
    m_update       = load<UpdateFn>      ("SDL_UpdateGamepads");
    m_close        = load<CloseFn>       ("SDL_CloseGamepad");
    m_getAxis      = load<GetAxisFn>     ("SDL_GetGamepadAxis");
    m_getButton    = load<GetButtonFn>   ("SDL_GetGamepadButton");
    m_getName      = load<GetNameFn>     ("SDL_GetGamepadName");
    m_open         = load<OpenFn>        ("SDL_OpenGamepad");
    m_getJoysticks = load<GetJoysticksFn>("SDL_GetJoysticks");
    m_isGamepad    = load<IsGamepadFn>   ("SDL_IsGamepad");
    m_joyNameByID  = load<JoyNameByIDFn> ("SDL_GetJoystickNameForID");
    m_sdlFree      = load<SdlFreeFn>     ("SDL_free");

    const bool ok = m_init && m_getAxis && m_getButton && m_open &&
                    m_close && m_getJoysticks && m_isGamepad;
    m_diag += QString("Functions: %1\n").arg(ok ? "OK" : "SOME MISSING");
    if (!ok) return;

    constexpr quint32 SDL_INIT_GAMEPAD = 0x00002000u;
    const int rc = m_init(SDL_INIT_GAMEPAD);
    // SDL3 returns bool (1 byte) — check low byte only.
    const bool initOk = (rc & 0xFF) != 0;
    m_diag += QString("SDL_InitSubSystem: %1\n").arg(initOk ? "OK" : "FAILED");
    m_available = initOk;
}

SdlControllerBackend::~SdlControllerBackend()
{
    if (m_controller && m_close) m_close(m_controller);
}

void SdlControllerBackend::ensureController()
{
    if (m_controller) return;

    int count = 0;
    quint32 *ids = m_getJoysticks ? m_getJoysticks(&count) : nullptr;
    m_diag += QString("Joysticks: %1\n").arg(count);

    for (int i = 0; i < count; ++i) {
        const char *name = m_joyNameByID ? m_joyNameByID(ids[i]) : nullptr;
        const bool  isGP = m_isGamepad && (m_isGamepad(ids[i]) & 0xFF) != 0;
        m_diag += QString("  [id=%1] \"%2\"  isGamepad=%3\n")
                      .arg(ids[i]).arg(name ? name : "?").arg(isGP ? "YES" : "NO");
        if (!isGP) continue;
        m_controller = m_open(ids[i]);
        if (m_controller) {
            m_diag += QString("  -> Opened: \"%1\"\n")
                          .arg(m_getName ? m_getName(m_controller) : "?");
            break;
        }
        m_diag += "  -> SDL_OpenGamepad FAILED\n";
    }

    if (ids && m_sdlFree) m_sdlFree(ids);
    if (!m_controller) m_diag += "  -> No usable gamepad found\n";
}

#endif // Q_OS_WIN
