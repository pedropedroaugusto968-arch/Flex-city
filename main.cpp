#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

// --- ORDEM CRÍTICA PARA NÃO DAR ERRO NO BUILD ---
#include <windows.h>
#include <windowsx.h>
#include <objidl.h> 
#include <algorithm>
namespace Gdiplus { using std::min; using std::max; }
#include <gdiplus.h>

#include <tlhelp32.h>
#include <string>
#include <vector>
#include <cmath>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

using namespace Gdiplus;

// ── CONFIGURAÇÕES VISUAIS (SIRIUS) ──────────────────────────
static const int W_WIN = 680, H_WIN = 420, W_LEFT = 170;
static const int H_TOPBAR = 36, H_SUBTAB = 34, BALL_D = 54;

static const Color C_BG = Color(255, 13, 13, 20), C_LEFT = Color(255, 10, 9, 16);
static const Color C_PURPLE = Color(255, 130, 60, 220), C_NEON = Color(255, 190, 130, 255);
static const Color C_TEXT = Color(255, 210, 205, 225), C_TEXTDIM = Color(255, 110, 100, 140);
static const Color C_BORDER = Color(255, 35, 28, 55), C_CARD = Color(255, 18, 16, 27);

static ULONG_PTR g_gdipToken;

// ── ESTADO DO SOFTWARE ─────────────────────────────────────
struct State {
    int catIdx = 0, subIdx = 0;
    bool aimEnabled = false, flyEnabled = false, bBypass = false;
    bool minimized = false, dragging = false;
    POINT dragOff = {};
    HANDLE hProcess = NULL;
} S;

// ── MOTOR DE BUSCA (AOB SCANNER) ───────────────────────────
uintptr_t FindPattern(const BYTE* pattern, const char* mask) {
    if (!S.hProcess) return 0;
    SYSTEM_INFO si; GetSystemInfo(&si);
    uintptr_t start = (uintptr_t)si.lpMinimumApplicationAddress;
    uintptr_t end = (uintptr_t)si.lpMaximumApplicationAddress;
    size_t pLen = strlen(mask);
    while (start < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQueryEx(S.hProcess, (LPCVOID)start, &mbi, sizeof(mbi))) break;
        if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_READWRITE || mbi.Protect & PAGE_EXECUTE_READWRITE)) {
            std::vector<BYTE> buf(mbi.RegionSize);
            if (ReadProcessMemory(S.hProcess, mbi.BaseAddress, buf.data(), mbi.RegionSize, NULL)) {
                for (size_t i = 0; i < mbi.RegionSize - pLen; i++) {
                    bool f = true;
                    for (size_t j = 0; j < pLen; j++) {
                        if (mask[j] == 'x' && buf[i + j] != pattern[j]) { f = false; break; }
                    }
                    if (f) return (uintptr_t)mbi.BaseAddress + i;
                }
            }
        }
        start += mbi.RegionSize;
    }
    return 0;
}

// ── THREAD DE INJEÇÃO ──────────────────────────────────────
DWORD WINAPI InjectionThread(LPVOID) {
    while (true) {
        if (S.bBypass && S.hProcess) {
            if (S.flyEnabled) {
                const BYTE flySig[] = { 0xDE, 0xAD, 0xBE, 0xEF }; // Troque pelos bytes reais do Flex City
                uintptr_t addr = FindPattern(flySig, "xxxx");
                if (addr) {
                    float val = 1000.0f;
                    WriteProcessMemory(S.hProcess, (LPVOID)addr, &val, sizeof(val), NULL);
                }
            }
        }
        Sleep(1000);
    }
    return 0;
}

void ApplyBypass() {
    HWND hwnd = FindWindowA(NULL, "Flex City");
    if (hwnd) {
        DWORD pid; GetWindowThreadProcessId(hwnd, &pid);
        S.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (S.hProcess) {
            S.bBypass = true;
            static bool once = false;
            if (!once) { CreateThread(NULL, 0, InjectionThread, NULL, 0, NULL); once = true; }
        }
    }
}

// ── WIDGETS DE DESENHO (SIRIUS) ────────────────────────────
void TXT(Graphics& g, const wchar_t* s, float sz, float x, float y, Color c, bool b = false) {
    Font font(L"Segoe UI", sz, b ? FontStyleBold : FontStyleRegular);
    SolidBrush br(c); g.DrawString(s, -1, &font, PointF(x, y), &br);
}

void DrawSwitch(Graphics& g, float x, float y, const wchar_t* label, bool on, bool* val) {
    SolidBrush tBr(on ? Color(200, 110, 50, 200) : Color(255, 30, 26, 46));
    g.FillRectangle(&tBr, x, y, 34, 16);
    SolidBrush kBr(on ? C_NEON : C_TEXTDIM);
    g.FillEllipse(&kBr, on ? x + 18 : x + 2, y + 2, 12, 12);
    TXT(g, label, 9.0f, x + 45, y - 2, C_TEXT);
}

// ── PAINT PRINCIPAL ────────────────────────────────────────
void PaintPanel(HWND hwnd) {
    PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
    Graphics g(hdc); g.SetSmoothingMode(SmoothingModeAntiAlias);
    SolidBrush bg(C_BG); g.FillRectangle(&bg, 0, 0, W_WIN, H_WIN);
    
    // Sidebar
    SolidBrush lbg(C_LEFT); g.FillRectangle(&lbg, 0, 0, W_LEFT, H_WIN);
    TXT(g, L"Space Xit", 12, 15, 15, C_NEON, true);
    
    // Conteúdo (Aba Aim)
    if (S.catIdx == 0) {
        DrawSwitch(g, W_LEFT + 20, 80, L"Enable Bypass", S.bBypass, &S.bBypass);
        DrawSwitch(g, W_LEFT + 20, 110, L"Fly Mode", S.flyEnabled, &S.flyEnabled);
    }
    EndPaint(hwnd, &ps);
}

// ── WNDPROC ────────────────────────────────────────────────
LRESULT CALLBACK PanelProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT: PaintPanel(hwnd); return 0;
        case WM_LBUTTONDOWN: {
            int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
            if (mx > W_LEFT + 20 && mx < W_LEFT + 150) {
                if (my > 80 && my < 100) ApplyBypass();
                if (my > 110 && my < 130) S.flyEnabled = !S.flyEnabled;
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    GdiplusStartupInput gdi; GdiplusStartup(&g_gdipToken, &gdi, NULL);
    WNDCLASSW wc = {0}; wc.lpfnWndProc = PanelProc; wc.hInstance = hI; wc.lpszClassName = L"SiriusPanel";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST, L"SiriusPanel", L"Sirius", WS_POPUP, 100, 100, W_WIN, H_WIN, NULL, NULL, hI, NULL);
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA); ShowWindow(hwnd, nS);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    GdiplusShutdown(g_gdipToken); return 0;
}
