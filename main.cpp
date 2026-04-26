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
#include <string>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

using namespace Gdiplus;

// --- ESTADO COMPLETO ---
struct State {
    int catIdx = 0; // 0: Aim, 1: Visuals, 2: Misc
    // Funções
    bool aimEnabled = false, aimSolve = false;
    int aimFov = 150;
    bool espBox = false, espSkeleton = false, espLines = false;
    bool godMode = false, bBypass = false;
    // Sistema
    HANDLE hProcess = NULL;
    bool dragging = false;
    POINT dragOff = {};
} S;

// --- MOTOR DE BUSCA DE OFFSETS (AOB SCANNER) ---
uintptr_t FindPattern(HANDLE hProc, const BYTE* pattern, const char* mask) {
    SYSTEM_INFO si; GetSystemInfo(&si);
    uintptr_t start = (uintptr_t)si.lpMinimumApplicationAddress;
    uintptr_t end = (uintptr_t)si.lpMaximumApplicationAddress;
    size_t patternLen = strlen(mask);

    while (start < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQueryEx(hProc, (LPCVOID)start, &mbi, sizeof(mbi))) break;
        if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_READWRITE || mbi.Protect & PAGE_EXECUTE_READWRITE)) {
            std::vector<BYTE> buffer(mbi.RegionSize);
            if (ReadProcessMemory(hProc, mbi.BaseAddress, buffer.data(), mbi.RegionSize, NULL)) {
                for (size_t i = 0; i < mbi.RegionSize - patternLen; i++) {
                    bool found = true;
                    for (size_t j = 0; j < patternLen; j++) {
                        if (mask[j] == 'x' && buffer[i + j] != pattern[j]) { found = false; break; }
                    }
                    if (found) return (uintptr_t)mbi.BaseAddress + i;
                }
            }
        }
        start += mbi.RegionSize;
    }
    return 0;
}

// --- VISUAL SIRIUS COMPLETO ---
static const int W_WIN = 680, H_WIN = 420, W_LEFT = 170, H_TOPBAR = 36, BALL_D = 54;
static const Color C_BG(255, 13, 13, 20), C_LEFT(255, 10, 9, 16), C_PURPLE(255, 130, 60, 220), C_TEXT(255, 210, 205, 225);

void DrawSwitch(Graphics& g, float x, float y, const wchar_t* label, bool on) {
    SolidBrush swBg(on ? C_PURPLE : Color(255, 35, 35, 45));
    GraphicsPath p; 
    float r = 8;
    p.AddArc(x, y, r*2, r*2, 180, 90); p.AddArc(x+34-r*2, y, r*2, r*2, 270, 90);
    p.AddArc(x+34-r*2, y+16-r*2, r*2, r*2, 0, 90); p.AddArc(x, y+16-r*2, r*2, r*2, 90, 90);
    g.FillPath(&swBg, &p);
    SolidBrush k(Color(255, 255, 255, 255));
    g.FillEllipse(&k, on ? (REAL)(x + 18) : (REAL)(x + 2), (REAL)(y + 2), (REAL)12, (REAL)12);
    Font f(L"Segoe UI", 9.0f); SolidBrush b(C_TEXT);
    g.DrawString(label, -1, &f, PointF(x + 45, y - 2), &b);
}

// --- PAINT DO PAINEL ---
void PaintPanel(HWND hwnd) {
    PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
    Graphics g(hdc); g.SetSmoothingMode(SmoothingModeAntiAlias);

    SolidBrush bg(C_BG); g.FillRectangle(&bg, 0, 0, W_WIN, H_WIN);
    SolidBrush lbg(C_LEFT); g.FillRectangle(&lbg, 0, 0, W_LEFT, H_WIN);

    // Sidebar Tabs
    Font fL(L"Segoe UI", 11, FontStyleBold); SolidBrush brP(C_PURPLE);
    g.DrawString(L"Space Xit", -1, &fL, PointF(20, 20), &brP);

    Font fM(L"Segoe UI", 10); SolidBrush brT(C_TEXT);
    g.DrawString(L"Aimbot", -1, &fM, PointF(25, 80), S.catIdx == 0 ? &brP : &brT);
    g.DrawString(L"Visuals", -1, &fM, PointF(25, 110), S.catIdx == 1 ? &brP : &brT);
    g.DrawString(L"Misc", -1, &fM, PointF(25, 140), S.catIdx == 2 ? &brP : &brT);

    // Botões X e -
    SolidBrush bR(Color(255, 230, 70, 70));
    g.FillEllipse(&bR, (REAL)(W_WIN - 25), (REAL)10, (REAL)14, (REAL)14);
    g.FillEllipse(&brP, (REAL)(W_WIN - 50), (REAL)10, (REAL)14, (REAL)14);

    // Conteúdo dinâmico
    float sX = W_LEFT + 30;
    if (S.catIdx == 0) {
        DrawSwitch(g, sX, 80, L"Enable Aimbot", S.aimEnabled);
        DrawSwitch(g, sX, 110, L"Aim Solve (Prediction)", S.aimSolve);
    } else if (S.catIdx == 1) {
        DrawSwitch(g, sX, 80, L"ESP Box", S.espBox);
        DrawSwitch(g, sX, 110, L"ESP Skeleton", S.espSkeleton);
        DrawSwitch(g, sX, 140, L"ESP Lines", S.espLines);
    } else if (S.catIdx == 2) {
        DrawSwitch(g, sX, 80, L"God Mode", S.godMode);
        DrawSwitch(g, sX, 110, L"Bypass Flex City", S.bBypass);
    }

    EndPaint(hwnd, &ps);
}

// --- WNDPROCS ---
LRESULT CALLBACK PanelProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT: PaintPanel(hwnd); return 0;
        case WM_LBUTTONDOWN: {
            int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
            if (mx >= W_WIN - 25 && my <= 30) ExitProcess(0);
            if (mx >= W_WIN - 50 && mx <= W_WIN - 35 && my <= 30) { ShowWindow(hwnd, SW_HIDE); ShowWindow(FindWindow(L"SiriusBall", NULL), SW_SHOW); }
            
            if (mx < W_LEFT) {
                if (my >= 75 && my <= 95) S.catIdx = 0;
                else if (my >= 105 && my <= 125) S.catIdx = 1;
                else if (my >= 135 && my <= 155) S.catIdx = 2;
            } else {
                if (S.catIdx == 0 && my >= 80 && my <= 100) S.aimEnabled = !S.aimEnabled;
                if (S.catIdx == 1 && my >= 80 && my <= 100) S.espBox = !S.espBox;
                if (S.catIdx == 2 && my >= 110 && my <= 130) S.bBypass = !S.bBypass;
            }
            if (my < H_TOPBAR) { S.dragging = true; SetCapture(hwnd); GetCursorPos(&S.dragOff); RECT r; GetWindowRect(hwnd, &r); S.dragOff.x -= r.left; S.dragOff.y -= r.top; }
            InvalidateRect(hwnd, NULL, FALSE); return 0;
        }
        case WM_MOUSEMOVE: if (S.dragging) { POINT p; GetCursorPos(&p); SetWindowPos(hwnd, NULL, p.x - S.dragOff.x, p.y - S.dragOff.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER); } return 0;
        case WM_LBUTTONUP: S.dragging = false; ReleaseCapture(); return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// --- WINMAIN ---
int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    GdiplusStartupInput gsi; ULONG_PTR gst; GdiplusStartup(&gst, &gsi, NULL);
    WNDCLASSW wc = {0}; wc.lpfnWndProc = PanelProc; wc.hInstance = hI; wc.lpszClassName = L"SiriusPanel"; wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClassW(&wc);
    
    // Classe da Bolinha Flutuante (Arrastável)
    WNDCLASSW wb = {0}; wb.lpfnWndProc = DefWindowProc; wb.hInstance = hI; wb.lpszClassName = L"SiriusBall"; RegisterClassW(&wb);

    HWND hP = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, L"SiriusPanel", L"Space Xit", WS_POPUP, 100, 100, W_WIN, H_WIN, NULL, NULL, hI, NULL);
    SetLayeredWindowAttributes(hP, RGB(1, 1, 1), 252, LWA_ALPHA | LWA_COLORKEY);
    
    ShowWindow(hP, nS);
    MSG m; while (GetMessage(&m, NULL, 0, 0)) { TranslateMessage(&m); DispatchMessage(&m); }
    GdiplusShutdown(gst); return 0;
}
