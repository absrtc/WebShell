#include <windows.h>
#include <string>
#include <fstream>
#include <shlobj.h>
#include <wrl.h>
#include <wil/com.h>
#include <dwmapi.h>
#include <windowsx.h>
#include "WebView2.h"

#pragma comment(lib, "dwmapi.lib")

using namespace Microsoft::WRL;

wil::com_ptr<ICoreWebView2Controller> webviewController;
wil::com_ptr<ICoreWebView2> webviewWindow;

std::wstring g_Title = L"WebShell";
int g_Width = 1200;
int g_Height = 900;
bool g_Frame = true;
bool g_Resizable = true;
bool g_Maximizable = true;
bool g_DevTools = true;
bool g_Shadow = true;

std::wstring GetJsonStr(const std::string& c, std::string k) {
    size_t p = c.find("\"" + k + "\"");
    if (p == std::string::npos) return L"";
    size_t s = c.find("\"", c.find(":", p)) + 1;
    size_t e = c.find("\"", s);
    return std::wstring(c.begin() + s, c.begin() + e);
}

bool GetJsonBool(const std::string& c, std::string k) {
    size_t p = c.find("\"" + k + "\"");
    if (p == std::string::npos) return true;
    size_t vp = c.find(":", p);
    return c.find("true", vp) < c.find("false", vp);
}

int GetJsonInt(const std::string& c, std::string k, int d) {
    size_t p = c.find("\"" + k + "\"");
    if (p == std::string::npos) return d;
    size_t s = c.find(":", p) + 1;
    size_t e = c.find_first_of(",}\n", s);
    try { return std::stoi(c.substr(s, e - s)); }
    catch (...) { return d; }
}

void LoadConfig() {
    std::ifstream f("webshell_config.json");
    if (!f.is_open()) return;
    std::string c((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    g_Title = GetJsonStr(c, "title");
    g_Width = GetJsonInt(c, "width", 1200);
    g_Height = GetJsonInt(c, "height", 900);
    g_Frame = GetJsonBool(c, "frame");
    g_Resizable = GetJsonBool(c, "resizable");
    g_Maximizable = GetJsonBool(c, "maximizable");
    g_DevTools = GetJsonBool(c, "devtools");
    g_Shadow = GetJsonBool(c, "shadow");
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_SIZE && webviewController) {
        RECT b; GetClientRect(h, &b);
        webviewController->put_Bounds(b);
    }
    if (m == WM_DESTROY) PostQuitMessage(0);

    if (m == WM_NCCALCSIZE && !g_Frame && w == TRUE) {
        NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)l;

        if (IsMaximized(h)) {
            WINDOWINFO wi = { sizeof(wi) };
            GetWindowInfo(h, &wi);
            params->rgrc[0].top += wi.cyWindowBorders;
            params->rgrc[0].left += wi.cxWindowBorders;
            params->rgrc[0].right -= wi.cxWindowBorders;
            params->rgrc[0].bottom -= wi.cyWindowBorders;
        }

        return 0;
    }

    if (m == WM_NCHITTEST && !g_Frame) {
        LRESULT hit = DefWindowProc(h, m, w, l);
        if (hit == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
            ScreenToClient(h, &pt);
            RECT rc;
            GetClientRect(h, &rc);

            const int BORDER_SIZE = 5;

            bool left = pt.x < BORDER_SIZE;
            bool right = pt.x > rc.right - BORDER_SIZE;
            bool top = pt.y < BORDER_SIZE;
            bool bottom = pt.y > rc.bottom - BORDER_SIZE;

            if (g_Resizable) {
                if (top && left) return HTTOPLEFT;
                if (top && right) return HTTOPRIGHT;
                if (bottom && left) return HTBOTTOMLEFT;
                if (bottom && right) return HTBOTTOMRIGHT;
                if (left) return HTLEFT;
                if (right) return HTRIGHT;
                if (top) return HTTOP;
                if (bottom) return HTBOTTOM;
            }
        }
        return hit;
    }

    return DefWindowProc(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    int argc;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::wstring targetUrl = L"https://www.bing.com";
    bool useConfig = false;

    for (int i = 1; i < argc; i++) {
        std::wstring a = argv[i];
        if (a == L"-config=true") useConfig = true;
        if (a == L"-shadow=false") g_Shadow = false;
        if (a.find(L"-url=") == 0) {
            targetUrl = a.substr(5);
            if (!targetUrl.empty() && targetUrl.front() == L'\"') targetUrl = targetUrl.substr(1, targetUrl.length() - 2);
        }
    }

    if (useConfig) LoadConfig();

    LocalFree(argv);

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!g_Resizable) style &= ~WS_THICKFRAME;
    if (!g_Maximizable) style &= ~WS_MAXIMIZEBOX;

    WNDCLASSEXW wx = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hI, NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, L"WS_MAIN", NULL };
    RegisterClassExW(&wx);
    HWND hWnd = CreateWindowExW(0, L"WS_MAIN", g_Title.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT, g_Width, g_Height, NULL, NULL, hI, NULL);

    if (g_Shadow) {
        MARGINS margins = { 0, 0, 0, 1 };
        DwmExtendFrameIntoClientArea(hWnd, &margins);

        DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
        DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
    }

    if (!g_Frame) {
        SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }

    ShowWindow(hWnd, nS);
    UpdateWindow(hWnd);

    wchar_t appData[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appData);
    std::wstring dataPath = std::wstring(appData) + L"\\WebShellRuntimeData";
    CreateDirectoryW(dataPath.c_str(), NULL);

    CreateCoreWebView2EnvironmentWithOptions(nullptr, dataPath.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([hWnd, targetUrl](HRESULT r, ICoreWebView2Environment* env) -> HRESULT {
            env->CreateCoreWebView2Controller(hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([hWnd, targetUrl](HRESULT r, ICoreWebView2Controller* ctrl) -> HRESULT {
                webviewController = ctrl;
                webviewController->get_CoreWebView2(&webviewWindow);

                wil::com_ptr<ICoreWebView2Settings> set;
                webviewWindow->get_Settings(&set);
                set->put_IsWebMessageEnabled(TRUE);

                webviewWindow->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>([hWnd](ICoreWebView2* s, ICoreWebView2WebMessageReceivedEventArgs* a) -> HRESULT {
                    wil::unique_cotaskmem_string m;
                    if (SUCCEEDED(a->TryGetWebMessageAsString(&m))) {
                        std::wstring sm = m.get();

                        HWND mainWnd = GetAncestor(hWnd, GA_ROOT);
                        if (!mainWnd) mainWnd = hWnd;

                        if (sm == L"drag") {
                            ReleaseCapture();
                            SendMessage(mainWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
                        }
                        else if (sm == L"min") {
                            ShowWindow(mainWnd, SW_MINIMIZE);
                        }
                        else if (sm == L"max") {
                            WINDOWPLACEMENT wp = { sizeof(wp) };
                            GetWindowPlacement(mainWnd, &wp);
                            ShowWindow(mainWnd, (wp.showCmd == SW_MAXIMIZE) ? SW_RESTORE : SW_MAXIMIZE);
                        }
                        else if (sm == L"close") {
                            SendMessage(mainWnd, WM_CLOSE, 0, 0);
                        }
                    }
                    return S_OK;
                    }).Get(), nullptr);

                RECT b; GetClientRect(hWnd, &b);
                webviewController->put_Bounds(b);
                webviewWindow->Navigate(targetUrl.c_str());
                return S_OK;
                }).Get());
            return S_OK;
            }).Get());

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return (int)msg.wParam;
}