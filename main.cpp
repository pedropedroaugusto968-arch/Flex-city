#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h> 
#include <objidl.h> 
#include <algorithm>
namespace Gdiplus { using std::min; using std::max; }
#include <gdiplus.h>
#include <tlhelp32.h>
#include <vector>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

using namespace Gdiplus;

// --- ESTADO DO HACK ---
struct State {
    int catIdx = 0;
    bool bBypass = false, aimEnabled = false, aimSolve = false, godMode = false;
    bool minimized = false, dragging = false;
    POINT dragOff = {};
    HANDLE hProcess = NULL;
} S;

// --- CONFIGS VISUAIS ---
static const int W_WIN = 680, H_WIN = 420, W_LEFT = 170, H_TOPBAR = 36, BALL_D = 54;
static const Color C_BG(255, 13, 13, 20), C_LEFT(255, 10, 9, 16), C_PURPLE(255, 130, 60, 220), C_TEXT(255, 210, 205, 225);

// --- AUXILIARES DE DESENHO ---
void FillRR(Graphics& g, Brush& br, float x, float y, float w, float h, float r) {
    GraphicsPath p;
    p.AddArc(x, y, r * 2, r * 2, 180, 90);
    p.AddArc(x + w - r * 2, y, r * 2, r * 2, 270, 90);
    p.AddArc(x + w - r * 2, y + h - r * 2, r * 2, r * 2, 0, 90);
    p.AddArc(x, y + h - r * 2, r * 2, r * 2, 90, 90);
    p.CloseFigure(); g.FillPath(&br, &p);
}

void TXT(Graphics& g, const wchar_t* s, float sz, float x, float y, Color c, bool b = false) {
    Font f(L"Segoe UI", sz, b ? FontStyleBold : FontStyleRegular);
    SolidBrush br(c); g.DrawString(s, -1, &f, PointF(x, y), NULL, &br);
}

void DrawSwitch(Graphics& g, float x, float y, const wchar_t* label, bool on) {
    SolidBrush swBg(on ? C_PURPLE : Color(255, 35, 35, 45));
    FillRR(g, swBg, (REAL)x, (REAL)y, (REAL)34, (REAL)16, (REAL)8);
    SolidBrush kBr(Color(255, 255, 255, 255));
    g.FillEllipse(&kBr, on ? (REAL)(x + 18) : (REAL)(x + 2), (REAL)(y + 2), (REAL)12, (REAL)12);
    TXT(g, label, 9.0f, (REAL)(x + 45), (REAL)(y - 2), C_TEXT);
}

// --- PAINT DO PAINEL ---
void PaintPanel(HWND hwnd) {
    PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
    Graphics g(hdc); g.SetSmoothingMode(SmoothingModeAntiAlias);

    SolidBrush bg(C_BG); g.FillRectangle(&bg, (REAL)0, (REAL)0, (REAL)W_WIN, (REAL)H_WIN);
    SolidBrush lbg(C_LEFT); g.FillRectangle(&lbg, (REAL)0, (REAL)0, (REAL)W_LEFT, (REAL)H_WIN);

    // Sidebar e Logo
    TXT(g, L"Space Xit", 11, 15, 15, C_PURPLE, true);
    TXT(g, L"Aimbot", 10, 25, 80, S.catIdx == 0 ? C_PURPLE : C_TEXT, S.catIdx == 0);
    TXT(g, L"Visuals", 10, 25, 110, S.catIdx == 1 ? C_PURPLE : C_TEXT, S.catIdx == 1);
    TXT(g, L"Misc", 10, 25, 140, S.catIdx == 2 ? C_PURPLE : C_TEXT, S.catIdx == 2);

    // BOTÕES LATERAIS (X e -)
    SolidBrush bRed(Color(255, 220, 60, 60)), bPurp(C_PURPLE);
    g.FillEllipse(&bRed, (REAL)(W_WIN - 25), (REAL)10, (REAL)14, (REAL)14); // X para Fechar
    g.FillEllipse(&bPurp, (REAL)(W_WIN - 50), (REAL)10, (REAL)14, (REAL)14); // - para Minimizar

    // Conteúdo
    float sX = W_LEFT + 30;
    if (S.catIdx == 0) {
        DrawSwitch(g, sX, 80, L"Enable Aimbot", S.aimEnabled);
        DrawSwitch(g, sX, 110, L"Aim Solve", S.aimSolve);
    } else if (S.catIdx == 2) {
        DrawSwitch(g, sX, 80, L"God Mode", S.godMode);
        DrawSwitch(g, sX, 110, L"Bypass Flex City", S.bBypass);
    }

    EndPaint(hwnd, &ps);
}

void PaintBall(HWND hwnd) {
    PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
    Graphics g(hdc); g.SetSmoothingMode(SmoothingModeAntiAlias);
    SolidBrush br(C_PURPLE); g.FillEllipse(&br, (REAL)2, (REAL)2, (REAL)(BALL_D-4), (REAL)(BALL_D-4));
    TXT(g, L"S", 14, 20, 15, Color(255,255,255,255), true);
    EndPaint(hwnd, &ps);
}

// --- WNDPROCS (Lógica de Movimento e Cliques) ---
LRESULT CALLBACK PanelProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT: PaintPanel(hwnd); return 0;
        case WM_LBUTTONDOWN: {
            int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
            // Fechar (X) - Sai do Processo
            if (mx >= W_WIN - 25 && my <= 30) ExitProcess(0);
            // Minimizar (-) - Esconde e mostra a bola
            if (mx >= W_WIN - 50 && mx <= W_WIN - 35 && my <= 30) {
                ShowWindow(hwnd, SW_HIDE);
                ShowWindow(FindWindow(L"SiriusBall", NULL), SW_SHOW);
                return 0;
            }
            // Tabs e Toggles
            if (mx < W_LEFT) {
                if (my >= 75 && my <= 95) S.catIdx = 0;
                else if (my >= 105 && my <= 125) S.catIdx = 1;
                else if (my >= 135 && my <= 155) S.catIdx = 2;
            }
            if (mx > W_LEFT + 20 && mx < W_LEFT + 60) {
                if (S.catIdx == 0 && my >= 80 && my <= 100) S.aimEnabled = !S.aimEnabled;
                if (S.catIdx == 2 && my >= 110 && my <= 130) S.bBypass = !S.bBypass;
            }
            // Drag
            if (my < H_TOPBAR) { S.dragging = true; SetCapture(hwnd); GetCursorPos(&S.dragOff); RECT r; GetWindowRect(hwnd, &r); S.dragOff.x -= r.left; S.dragOff.y -= r.top; }
            InvalidateRect(hwnd, NULL, FALSE); return 0;
        }
        case WM_MOUSEMOVE: if (S.dragging) { POINT p; GetCursorPos(&p); SetWindowPos(hwnd, NULL, p.x - S.dragOff.x, p.y - S.dragOff.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER); } return 0;
        case WM_LBUTTONUP: S.dragging = false; ReleaseCapture(); return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

LRESULT CALLBACK BallProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static bool bDrag = false; static POINT bOff;
    switch (msg) {
        case WM_PAINT: PaintBall(hwnd); return 0;
        case WM_LBUTTONDBLCLK: ShowWindow(hwnd, SW_HIDE); ShowWindow(FindWindow(L"SiriusPanel", NULL), SW_SHOW); return 0;
        case WM_LBUTTONDOWN: bDrag = true; SetCapture(hwnd); GetCursorPos(&bOff); RECT r; GetWindowRect(hwnd, &r); bOff.x -= r.left; bOff.y -= r.top; return 0;
        case WM_MOUSEMOVE: if (bDrag) { POINT p; GetCursorPos(&p); SetWindowPos(hwnd, NULL, p.x - bOff.x, p.y - bOff.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER); } return 0;
        case WM_LBUTTONUP: bDrag = false; ReleaseCapture(); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// --- WINMAIN ---
int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    GdiplusStartupInput gsi; ULONG_PTR gst; GdiplusStartup(&gst, &gsi, NULL);
    WNDCLASSW wc = {0}; wc.lpfnWndProc = PanelProc; wc.hInstance = hI; wc.lpszClassName = L"SiriusPanel"; wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClassW(&wc);
    WNDCLASSW wb = {0}; wb.lpfnWndProc = BallProc; wb.hInstance = hI; wb.lpszClassName = L"SiriusBall"; wb.hCursor = LoadCursor(NULL, IDC_ARROW); wb.style = CS_DBLCLKS; RegisterClassW(&wb);

    HWND hP = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, L"SiriusPanel", L"Space Xit", WS_POPUP, 100, 100, W_WIN, H_WIN, NULL, NULL, hI, NULL);
    SetLayeredWindowAttributes(hP, RGB(1, 1, 1), 252, LWA_ALPHA | LWA_COLORKEY);
    HWND hB = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, L"SiriusBall", L"", WS_POPUP, 100, 100, BALL_D, BALL_D, NULL, NULL, hI, NULL);
    SetLayeredWindowAttributes(hB, RGB(1, 1, 1), 255, LWA_COLORKEY);

    ShowWindow(hP, nS);
    MSG m; while (GetMessage(&m, NULL, 0, 0)) { TranslateMessage(&m); DispatchMessage(&m); }
    GdiplusShutdown(gst); return 0;
}
