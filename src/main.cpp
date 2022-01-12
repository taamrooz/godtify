#pragma comment(                                                                                   \
    linker,                                                                                        \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <Windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <Objbase.h>
#include "resource.h"
#include <stdio.h>
#include <string.h>


typedef HWND hSpotify;

//
// Global variables/constants
//
constexpr auto SPOTIFY_MUTE                   = 524288;
constexpr auto SPOTIFY_VOLUMEDOWN             = 589824;
constexpr auto SPOTIFY_VOLUMEUP               = 655360;
constexpr auto SPOTIFY_NEXT                   = 720896;
constexpr auto SPOTIFY_PREV                   = 786432;
constexpr auto SPOTIFY_STOP                   = 851968;
constexpr auto SPOTIFY_PLAYPAUSE              = 917504;
constexpr auto ID_TRAY_EXIT_CONTEXT_MENU_ITEM = 1337;
constexpr auto ID_TRAY_ICON                   = 1338;
constexpr auto WMAPP_NOTIFYCALLBACK           = WM_APP + ID_TRAY_ICON;
constexpr auto FILE_PATH                      = "godtify.ini";
constexpr auto MAX_SPOTIFY_STR                = 20;
constexpr auto MAX_KEYBIND_STR                = 12;
constexpr auto DEFAULT_BINDS                  = "SPOTIFY_MUTE=\r\n"
                                                "SPOTIFY_VOLUMEDOWN=\r\n"
                                                "SPOTIFY_VOLUMEUP=\r\n"
                                                "SPOTIFY_NEXT=C-right\r\n"
                                                "SPOTIFY_PREV=C-left\r\n"
                                                "SPOTIFY_STOP=\r\n"
                                                "SPOTIFY_PLAYPAUSE=C-up\r\n";
hSpotify       g_spotify;
DWORD          g_spotifyID = 0;
HWND           g_hwnd;
HMENU          g_menu;
HINSTANCE      g_hinst;

//
// Forward decls
//
LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
BOOL             add_notification_icon(HWND);
BOOL             delete_notification_icon(HWND);
void             show_context_menu(HWND);
inline void      spotify_command(hSpotify spotify, unsigned int command);
void             read_hotkeys();
inline void      register_hotkeys(char* data);
inline void      resolve_keybind(char* bind, unsigned int* key, unsigned int* modifier);
bool             get_filename(hSpotify spotify, char* file_name, unsigned int name_length);
BOOL CALLBACK    enum_proc(HWND hwnd, LPARAM lparam);
bool             get_spotify();
DWORD WINAPI     manage_spotify(void*);
void             win_event_proc(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD);
unsigned short   char_to_vk(char c);
unsigned char    other_to_vk(const char* other);
static char*     strsep(char** str_p, const char* delim);
bool             char_to_modifier(char c, unsigned int& modifier);

BOOL CALLBACK enum_proc(HWND hwnd, LPARAM lparam)
{
    char file_name[MAX_PATH];
    char class_name[100];
    get_filename(hwnd, file_name, MAX_PATH);
    GetClassNameA(hwnd, class_name, 100);
    if (strcmp(file_name, "Spotify.exe") == 0 && strcmp(class_name, "Chrome_WidgetWin_0") == 0)
    {
#ifdef _DEBUG
        printf("Found instance\n");
#endif
        g_spotify = hwnd;
        return FALSE;
    }
    Sleep(1);
    return TRUE;
}

bool get_spotify() { return EnumWindows((WNDENUMPROC) enum_proc, 0) == 0; }

void win_event_proc(HWINEVENTHOOK hook,
                    DWORD         event,
                    HWND          hwnd,
                    LONG          id_obj,
                    LONG          id_child,
                    DWORD         thread_id,
                    DWORD         event_ms)
{
    if (event == EVENT_OBJECT_DESTROY)
    {
        g_spotifyID = 0;
        PostQuitMessage(0);
    }
}

DWORD WINAPI manage_spotify(void*)
{
    while (g_spotifyID == 0)
    {
        while (!get_spotify())
        {
            Sleep(50);
        }
        GetWindowThreadProcessId(g_spotify, &g_spotifyID);
        HWINEVENTHOOK hook = SetWinEventHook(EVENT_OBJECT_DESTROY,
                                             EVENT_OBJECT_DESTROY,
                                             nullptr,
                                             (WINEVENTPROC) win_event_proc,
                                             g_spotifyID,
                                             0,
                                             WINEVENT_OUTOFCONTEXT);
        MSG           msg;
        while (GetMessage(&msg, nullptr, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        UnhookWinEvent(hook);
    }
    return 0;
}

void read_hotkeys()
{
    char curr_path[MAX_PATH];
    GetModuleFileNameA(nullptr, curr_path, MAX_PATH);
    char* nameptr                      = strrchr(curr_path, '\\');
    curr_path[nameptr - curr_path + 1] = '\0';
    strcat_s(curr_path, MAX_PATH, FILE_PATH);
    HANDLE file = CreateFileA(curr_path,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ,
                              nullptr,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        char buffer[400];
        if (!ReadFile(file, buffer, 400, nullptr, nullptr))
        {
#ifdef _DEBUG
            printf("ERROR reading file: %lu\n", GetLastError());
#endif
            CloseHandle(file);
            return;
        }
        register_hotkeys(buffer);
    }
    else
    {
        if (!WriteFile(file, DEFAULT_BINDS, strlen(DEFAULT_BINDS), nullptr, nullptr))
        {
#ifdef _DEBUG
            printf("ERROR writing file: %lu\n", GetLastError());
#endif
        }
        register_hotkeys(const_cast<char*>(DEFAULT_BINDS));
    }
    CloseHandle(file);
}

inline void register_hotkeys(char* data)
{
    char *hotkeys, *str, *tofree;
    tofree = str = _strdup(data);
    while ((hotkeys = strsep(&str, "\r\n")))
    {
        unsigned int length     = strlen(hotkeys);
        char*        equals_pos = strchr(hotkeys, '=');
        if (equals_pos == nullptr)
        {
            continue;
        }
        char spotify_str[MAX_SPOTIFY_STR];
        char keybind[MAX_KEYBIND_STR];
        strncpy_s(spotify_str, MAX_SPOTIFY_STR, hotkeys, equals_pos - hotkeys);
        if (equals_pos + 1 - hotkeys == length)
        {
            continue;
        }
        strcpy_s(keybind, MAX_KEYBIND_STR, equals_pos + 1);
        unsigned int key         = 0;
        unsigned int modifier    = 0;
        unsigned int spotify_cmd = 0;
        resolve_keybind(keybind, &key, &modifier);
        if (strcmp(spotify_str, "SPOTIFY_MUTE") == 0)
        {
            spotify_cmd = SPOTIFY_MUTE;
        }
        else if (strcmp(spotify_str, "SPOTIFY_VOLUMEDOWN") == 0)
        {
            spotify_cmd = SPOTIFY_VOLUMEDOWN;
        }
        else if (strcmp(spotify_str, "SPOTIFY_VOLUMEUP") == 0)
        {
            spotify_cmd = SPOTIFY_VOLUMEUP;
        }
        else if (strcmp(spotify_str, "SPOTIFY_NEXT") == 0)
        {
            spotify_cmd = SPOTIFY_NEXT;
        }
        else if (strcmp(spotify_str, "SPOTIFY_PREV") == 0)
        {
            spotify_cmd = SPOTIFY_PREV;
        }
        else if (strcmp(spotify_str, "SPOTIFY_STOP") == 0)
        {
            spotify_cmd = SPOTIFY_STOP;
        }
        else if (strcmp(spotify_str, "SPOTIFY_PLAYPAUSE") == 0)
        {
            spotify_cmd = SPOTIFY_PLAYPAUSE;
        }

        if (spotify_cmd == 0)
        {
            return;
        }
        if (!RegisterHotKey(g_hwnd, spotify_cmd, modifier | MOD_NOREPEAT, key))
        {
#ifdef _DEBUG
            printf("Failed to register hotkey %lu\n", GetLastError());
#endif
        }
    }
    free(tofree);
}

inline void resolve_keybind(char* bind, unsigned int* key, unsigned int* modifier)
{
    char *dash_token, *dash_str, *dash_tofree;
    dash_tofree = dash_str = _strdup(bind);
    while ((dash_token = strsep(&dash_str, "-")))
    {
        if (strlen(dash_token) > 1)
        {
            *key = other_to_vk(dash_token);
        }
        else
        {
            if (!char_to_modifier(dash_token[0], *modifier))
            {
                unsigned int vk = char_to_vk(dash_token[0]);
                if (vk > 0x0F)
                {
                    *key = vk;
                }
            }
        }
    }

    free(dash_tofree);
}

bool get_filename(hSpotify spotify, char* name, unsigned int name_length)
{
    char  file_path[MAX_PATH];
    char* fp = file_path;
    char* curr_c;
    DWORD dwSize = MAX_PATH;
    DWORD dwPid;
    GetWindowThreadProcessId(spotify, &dwPid);
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, dwPid);
    if (hProc == nullptr)
    {
        return false;
    }
    QueryFullProcessImageNameA(hProc, 0, file_path, &dwSize);
    CloseHandle(hProc);
    curr_c = strchr(fp, '\\');
    while (curr_c)
    {
        fp     = ++curr_c;
        curr_c = strchr(fp, '\\');
    }
    strcpy_s(name, name_length, fp);
#ifdef _DEBUG
    printf("%s\n", fp);
#endif
    return true;
}

inline void spotify_command(hSpotify spotify, unsigned int command)
{
    SendMessage(spotify, WM_APPCOMMAND, 0, command);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int)
{
    const wchar_t* window_name = L"Godtify";
    WNDCLASSEX     wcex        = { sizeof(WNDCLASSEX) };
    g_hinst                    = hInst;
    wcex.style                 = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc           = wnd_proc;
    wcex.cbClsExtra            = 0;
    wcex.cbWndExtra            = 0;
    wcex.hInstance             = hInst;
    wcex.hbrBackground         = (HBRUSH) (COLOR_WINDOW + 1);
    wcex.lpszMenuName          = window_name;
    wcex.hIcon                 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hIconSm               = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor               = LoadCursor(nullptr, IDI_APPLICATION);
    wcex.lpszClassName         = window_name;
    CoInitialize(nullptr);
    RegisterClassEx(&wcex);
    g_hwnd = CreateWindow(window_name,
                          window_name,
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          nullptr,
                          nullptr,
                          hInst,
                          nullptr);

    if (g_hwnd == nullptr)
    {
        return 0;
    }

    g_menu = CreatePopupMenu();
    AppendMenu(g_menu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM, L"Exit");
#ifdef _DEBUG
    AllocConsole();
    freopen_s((FILE**) stdout, "CONOUT$", "w", stdout);
#endif
    ShowWindow(g_hwnd, 0);
    UpdateWindow(g_hwnd);
    CreateThread(nullptr, 0, &manage_spotify, nullptr, 0, nullptr);
    read_hotkeys();
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    CoUninitialize();
    delete_notification_icon(g_hwnd);
    return 0;
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static UINT taskbar_restart;
    switch (msg)
    {
        case WM_CREATE:
        {
            add_notification_icon(hwnd);
            taskbar_restart = RegisterWindowMessage(L"TaskbarCreated");
        }
        break;
        case WM_HOTKEY:
        {
            spotify_command(g_spotify, wparam);
        }
        break;
        case WM_COMMAND:
        {
            int wmId = LOWORD(wparam);
            switch (wmId)
            {
                case ID_TRAY_EXIT_CONTEXT_MENU_ITEM:
                {
                    SendMessage(g_hwnd, WM_DESTROY, wparam, lparam);
                }
                break;
            }
        }
        break;
        case WMAPP_NOTIFYCALLBACK:
        {
            switch (lparam)
            {
                case WM_RBUTTONUP:
                {
                    show_context_menu(hwnd);
                }
                break;
                // Yeah?????????
                case 87687684:
                {
                    show_context_menu(hwnd);
                }
                break;
            }
        }
        break;
        case WM_DESTROY:
        {
            delete_notification_icon(hwnd);
            PostQuitMessage(0);
        }
        break;
        default:
        {
            if (msg == taskbar_restart)
            {
                add_notification_icon(hwnd);
            }
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
    }
    return 0;
}

BOOL add_notification_icon(HWND hwnd)
{
    NOTIFYICONDATA nid   = { sizeof(nid) };
    nid.hWnd             = hwnd;
    nid.uFlags           = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
    nid.uID              = ID_TRAY_ICON;
    nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
    wcscpy_s(nid.szTip, L"Godtify");
    LoadIconMetric(g_hinst, MAKEINTRESOURCE(IDI_ICON1), LIM_SMALL, &nid.hIcon);
    Shell_NotifyIcon(NIM_ADD, &nid);
    nid.uVersion = NOTIFYICON_VERSION_4;
    return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL delete_notification_icon(HWND hwnd)
{
    NOTIFYICONDATA nid = { sizeof(nid) };
    nid.uID            = ID_TRAY_ICON;
    nid.cbSize         = sizeof(NOTIFYICONDATA);
    nid.hWnd           = hwnd;
    return Shell_NotifyIcon(NIM_DELETE, &nid);
}

void show_context_menu(HWND hwnd)
{
    POINT cur_point;
    GetCursorPos(&cur_point);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(g_menu, TPM_RIGHTBUTTON, cur_point.x, cur_point.y, 0, hwnd, nullptr);
}

unsigned short char_to_vk(char c) { return VkKeyScanA(c); }
unsigned char  other_to_vk(const char* other)
{
    if (strcmp(other, "bck") == 0)
        return VK_BACK;
    if (strcmp(other, "spc") == 0)
        return VK_SPACE;
    if (strcmp(other, "tab") == 0)
        return VK_TAB;
    if (strcmp(other, "ret") == 0)
        return VK_RETURN;
    if (strcmp(other, "esc") == 0)
        return VK_ESCAPE;
    if (strcmp(other, "del") == 0)
        return VK_DELETE;
    if (strcmp(other, "left") == 0)
        return VK_LEFT;
    if (strcmp(other, "right") == 0)
        return VK_RIGHT;
    if (strcmp(other, "up") == 0)
        return VK_UP;
    if (strcmp(other, "down") == 0)
        return VK_DOWN;
    if (strcmp(other, "f1") == 0)
        return VK_F1;
    if (strcmp(other, "f2") == 0)
        return VK_F2;
    if (strcmp(other, "f3") == 0)
        return VK_F3;
    if (strcmp(other, "f4") == 0)
        return VK_F4;
    if (strcmp(other, "f5") == 0)
        return VK_F5;
    if (strcmp(other, "f6") == 0)
        return VK_F6;
    if (strcmp(other, "f7") == 0)
        return VK_F7;
    if (strcmp(other, "f8") == 0)
        return VK_F8;
    if (strcmp(other, "f9") == 0)
        return VK_F9;
    if (strcmp(other, "f10") == 0)
        return VK_F10;
    if (strcmp(other, "f11") == 0)
        return VK_F11;
    if (strcmp(other, "f12") == 0)
        return VK_F12;
    if (strcmp(other, "f13") == 0)
        return VK_F13;
    if (strcmp(other, "f14") == 0)
        return VK_F14;
    if (strcmp(other, "f15") == 0)
        return VK_F15;
    if (strcmp(other, "f16") == 0)
        return VK_F16;
    if (strcmp(other, "f17") == 0)
        return VK_F17;
    if (strcmp(other, "f18") == 0)
        return VK_F18;
    if (strcmp(other, "f19") == 0)
        return VK_F19;
    if (strcmp(other, "f20") == 0)
        return VK_F20;
    if (strcmp(other, "f21") == 0)
        return VK_F21;
    if (strcmp(other, "f22") == 0)
        return VK_F22;
    if (strcmp(other, "f23") == 0)
        return VK_F23;
    if (strcmp(other, "f24") == 0)
        return VK_F24;
    return 0;
}

static char* strsep(char** str_p, const char* delim)
{
    if (*str_p == nullptr)
    {
        return nullptr;
    }
    char* token_start = *str_p;
    *str_p            = strpbrk(token_start, delim);
    if (*str_p)
    {
        **str_p = '\0';
        (*str_p)++;
    }
    return token_start;
}

bool char_to_modifier(char c, unsigned int& modifier)
{
    if (c == 'C')
    {
        modifier = MOD_CONTROL | modifier;
        return true;
    }
    if (c == 'S')
    {
        modifier = MOD_SHIFT | modifier;
        return true;
    }
    if (c == 'M')
    {
        modifier = MOD_ALT | modifier;
        return true;
    }
    return false;
}
