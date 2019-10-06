#include "__header__.h"
Actor::Actor(HWND hwnd)
{
    this->hwnd = hwnd;
}
void Actor::action(int mode, short time)
{
    this->mode = mode;
    this->time = time;
    handle = CreateThread(0, 0, s_run, this, 0, 0);
}
DWORD Actor::run() // when the action function is called, start a thread to execute run.
{
    switch (mode)
    {
    case MOVELEFT:
        move_left(time);
        break;
    case MOVERIGHT:
        move_right(time);
        break;
    case STOP:
        stop(time);
        break;
    case RESTART:
        restart();
        break;
    }

    return 0;
}
DWORD Actor::get_status()
{
    DWORD exitCode = 0;
    GetExitCodeThread(handle, &exitCode);
    return exitCode;
}
void Actor::move_left(short time)
{
    SendMessage(hwnd, WM_KEYDOWN, VK_LEFT, 0);
    Sleep(time);
    SendMessage(hwnd, WM_KEYUP, VK_LEFT, 0);
}
void Actor::move_right(short time)
{
    SendMessage(hwnd, WM_KEYDOWN, VK_RIGHT, 0);
    Sleep(time);
    SendMessage(hwnd, WM_KEYUP, VK_RIGHT, 0);
}
void Actor::restart()
{
    PostMessage(hwnd, WM_SYSKEYDOWN, VK_F10, 29);
    PostMessage(hwnd, WM_SYSKEYUP, VK_F10, 29);
    PostMessage(hwnd, WM_KEYDOWN, 0x46, 0);
    PostMessage(hwnd, WM_KEYUP, 0x46, 0);
    PostMessage(hwnd, WM_KEYDOWN, 0x4E, 0);
    PostMessage(hwnd, WM_KEYUP, 0x4E, 0);
}
void Actor::stop(short time)
{
    Sleep(time);
}