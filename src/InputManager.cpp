#include "InputManager.h"

InputManager::InputManager(QObject *parent) : QObject(parent) {}

void InputManager::keyPressed(Qt::Key key)
{
    m_held.insert(key);
    if (key == Qt::Key_Space || key == Qt::Key_Return || key == Qt::Key_Enter)
        m_confirmPressed = true;
    if (key == Qt::Key_Left || key == Qt::Key_A)
        m_leftPressed = true;
    if (key == Qt::Key_Right || key == Qt::Key_D)
        m_rightPressed = true;
    if (key == Qt::Key_Up || key == Qt::Key_W)
        m_upPressed = true;
    if (key == Qt::Key_Down || key == Qt::Key_S)
        m_downPressed = true;
}

void InputManager::keyReleased(Qt::Key key)
{
    m_held.remove(key);
}

bool InputManager::isMovingLeft() const
{
    return m_held.contains(Qt::Key_Left) || m_held.contains(Qt::Key_A);
}

bool InputManager::isMovingRight() const
{
    return m_held.contains(Qt::Key_Right) || m_held.contains(Qt::Key_D);
}

bool InputManager::isMovingUp() const
{
    return m_held.contains(Qt::Key_Up) || m_held.contains(Qt::Key_W);
}

bool InputManager::isMovingDown() const
{
    return m_held.contains(Qt::Key_Down) || m_held.contains(Qt::Key_S);
}

bool InputManager::isConfirmJustPressed() const
{
    return m_confirmPressed;
}

bool InputManager::isLeftJustPressed() const
{
    return m_leftPressed;
}

bool InputManager::isRightJustPressed() const
{
    return m_rightPressed;
}

bool InputManager::isUpJustPressed() const
{
    return m_upPressed;
}

bool InputManager::isDownJustPressed() const
{
    return m_downPressed;
}

void InputManager::endFrame()
{
    m_confirmPressed = false;
    m_leftPressed = false;
    m_rightPressed = false;
    m_upPressed = false;
    m_downPressed = false;
}
