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
#include <string>
#include <vector>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

using namespace Gdiplus;

// --- Configurações Visuais Sirius ---
static const int W_WIN = 680; static const int H_WIN = 420;
static const int W_LEFT = 170; static const int H_TOPBAR = 36;
static const int BALL_D = 54;

static const Color C_BG = Color(255, 13, 13, 20);
static const Color C_LEFT = Color(255, 10, 9, 16);
static const Color C_PURPLE = Color(255, 130, 60, 220);
static const Color C_NEON = Color(255, 190, 130, 255);
static const Color C_TEXT = Color(255, 210, 205, 225);

struct State {
    int catIdx = 0;
    bool bBypass = false;
    bool flyEnabled = false;
    bool minimized = false;
    bool dragging = false;
    POINT dragOff = {};
    HANDLE hProcess = NULL;
} S;

// --- Funções de Desenho Auxiliares (Resolvendo Ambiguidade) ---
void FillRR(Graphics& g, Brush& br, float x, float y, float w, float h, float r) {
    GraphicsPath p;
    p.AddArc(x, y, r * 2, r * 2, 180, 90);
    p.AddArc(x + w - r * 2, y, r * 2, r * 2, 270, 90);
    p.AddArc(x + w - r * 2, y + h - r * 2, r * 2, r * 2, 0, 90);
    p.AddArc(x, y + h - r * 2, r * 2, r * 2, 90, 90);
    p.CloseFigure();
    g.FillPath(&br, &p);
}

void TXT(Graphics& g, const wchar_t* s, float sz, float x, float y, Color c, bool bold = false) {
    Font f(L"Segoe UI", sz, bold ? FontStyleBold : FontStyleRegular);
    SolidBrush br(c);
    g.DrawString(s, -1, &f, PointF(x, y), NULL, &br);
}

// --- Lógica do Scanner ---
void ApplyBypass() {
    HWND hwnd = FindWindowA(NULL, "Flex City");
    if (hwnd) {
        DWORD pid; GetWindowThreadProcessId(hwnd, &pid);
        S.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (S.hProcess) S.bBypass = true;
    }
}

// --- Paint do Painel Principal ---
void PaintPanel(HWND hwnd) {
    PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    // Fundo Principal
    SolidBrush bg(C_BG);
    g.FillRectangle(&bg, (REAL)0, (REAL)0, (REAL)W_WIN, (REAL)H_WIN);

    // Sidebar
    SolidBrush lbg(C_LEFT);
    g.FillRectangle(&lbg, (REAL)0, (REAL)0, (REAL)W_LEFT, (REAL)H_WIN);

    // Barra Superior
    SolidBrush tbg(Color(255, 11, 10, 17));
    g.FillRectangle(&tbg, (REAL)W_LEFT, (REAL)0, (REAL)(W_WIN - W_LEFT), (REAL)H_TOPBAR);

    // Botões de Controle (X e -)
    SolidBrush btnClose(Color(200, 200, 55, 70)); // Vermelho
    g.FillEllipse(&btnClose, (REAL)(W_WIN - 25), (REAL)11, (REAL)13, (REAL)13);

    SolidBrush btnMin(C_PURPLE); // Roxo
    g.FillEllipse(&btnMin, (REAL)(W_WIN - 45), (REAL)11, (REAL)13, (REAL)13);

    // Conteúdo Space Xit
    TXT(g, L"Space Xit", 10.5f, 14, 14, C_TEXT, true);
    
    // Switch de Exemplo
    SolidBrush swBg(S.bBypass ? C_PURPLE : Color(255, 30, 26, 46));
    FillRR(g, swBg, (REAL)(W_LEFT + 20), (REAL)80, (REAL)34, (REAL)16, (REAL)8);
    TXT(g, L"Bypass Flex City", 9.0f, (REAL)(W_LEFT + 65), (REAL)78, C_TEXT);

    EndPaint(hwnd, &ps);
}

// --- Paint da Bolinha (Ball) ---
void PaintBall(HWND hwnd) {
    PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    SolidBrush br(C_PURPLE);
    g.FillEllipse(&br, (REAL)4, (REAL)4, (REAL)(BALL_D - 8), (REAL)(BALL_D - 8));
    TXT(g, L"S", 12.0f, (REAL)20, (REAL)15, Color(255, 255, 255, 255), true);

    EndPaint(hwnd, &ps);
}

// --- WndProc Painel ---
LRESULT CALLBACK PanelProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT: PaintPanel(hwnd); return 0;
        case WM_LBUTTONDOWN: {
            int mx = GET_X_LPARAM(lp); int my = GET_Y_LPARAM(lp);
            // Botão Fechar
            if (mx >= W_WIN - 25 && my <= 25) PostQuitMessage(0);
            // Botão Minimizar
            if (mx >= W_WIN - 45 && mx <= W_WIN - 30 && my <= 25) {
                ShowWindow(hwnd, SW_HIDE);
                ShowWindow(FindWindow(L"SiriusBall", NULL), SW_SHOW);
            }
            // Clique no Switch
            if (mx >= W_LEFT + 20 && mx <= W_LEFT + 54 && my >= 80 && my <= 96) {
                ApplyBypass();
            }
            // Drag Barra Superior
            if (my <= H_TOPBAR) {
                S.dragging = true;
                POINT pt = {mx, my}; ClientToScreen(hwnd, &pt);
                RECT wr; GetWindowRect(hwnd, &wr);
                S.dragOff = {pt.x - wr.left, pt.y - wr.top};
                SetCapture(hwnd);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (S.dragging) {
                POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)}; ClientToScreen(hwnd, &pt);
                SetWindowPos(hwnd, NULL, pt.x - S.dragOff.x, pt.y - S.dragOff.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            return 0;
        }
        case WM_LBUTTONUP: S.dragging = false; ReleaseCapture(); return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// --- WndProc Bolinha ---
LRESULT CALLBACK BallProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_PAINT) { PaintBall(hwnd); return 0; }
    if (msg == WM_LBUTTONDBLCLK) {
        ShowWindow(hwnd, SW_HIDE);
        ShowWindow(FindWindow(L"SiriusPanel", NULL), SW_SHOW);
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// --- Entry Point ---
int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    GdiplusStartupInput gsi; ULONG_PTR gst; GdiplusStartup(&gst, &gsi, NULL);

    WNDCLASSW wc = {0}; wc.lpfnWndProc = PanelProc; wc.hInstance = hI; wc.lpszClassName = L"SiriusPanel";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClassW(&wc);

    WNDCLASSW wb = {0}; wb.lpfnWndProc = BallProc; wb.hInstance = hI; wb.lpszClassName = L"SiriusBall";
    wb.hCursor = LoadCursor(NULL, IDC_ARROW); wc.style = CS_DBLCLKS; RegisterClassW(&wb);

    HWND hPanel = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, L"SiriusPanel", L"Sirius", WS_POPUP, 100, 100, W_WIN, H_WIN, NULL, NULL, hI, NULL);
    SetLayeredWindowAttributes(hPanel, RGB(1, 1, 1), 252, LWA_ALPHA | LWA_COLORKEY);

    HWND hBall = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, L"SiriusBall", L"", WS_POPUP, 200, 200, BALL_D, BALL_D, NULL, NULL, hI, NULL);
    SetLayeredWindowAttributes(hBall, RGB(1, 1, 1), 255, LWA_COLORKEY);

    ShowWindow(hPanel, nS);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    GdiplusShutdown(gst); return 0;
}
