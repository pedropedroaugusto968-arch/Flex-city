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

// --- ESTRUTURA DE DADOS E MEMÓRIA ---
struct State {
    int catIdx = 0;
    bool aimEnabled = false, aimSolve = false, espBox = false, espSkeleton = false;
    bool godMode = false, bBypass = false;
    int aimFov = 150;
    
    // Controle de Processo
    DWORD processId = 0;
    HANDLE hProcess = NULL;
    bool dragging = false;
    POINT dragOff = {};
} S;

// --- MOTOR DE SCANNER (Busca de Offsets no Jogo) ---
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

// --- THREAD DE EXECUÇÃO DO HACK ---
DWORD WINAPI HackThread(LPVOID lpParam) {
    while (true) {
        if (S.bBypass && S.hProcess) {
            // Exemplo: Scanner de GodMode (Assinatura Genérica)
            if (S.godMode) {
                const BYTE godPattern[] = { 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00 }; 
                uintptr_t addr = FindPattern(S.hProcess, godPattern, "xx?????");
                if (addr) {
                    BYTE val = 1;
                    WriteProcessMemory(S.hProcess, (LPVOID)addr, &val, sizeof(val), NULL);
                }
            }
        }
        Sleep(500); // Evita sobrecarga de CPU
    }
    return 0;
}

void AttachToGame() {
    HWND hwnd = FindWindowA(NULL, "Flex City"); // Nome do processo do jogo
    if (hwnd) {
        GetWindowThreadProcessId(hwnd, &S.processId);
        S.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, S.processId);
        if (S.hProcess) {
            S.bBypass = true;
            CreateThread(NULL, 0, HackThread, NULL, 0, NULL);
        }
    }
}

// --- VISUAL E INTERFACE (SIRIUS STYLE) ---
static const int W_WIN = 680, H_WIN = 420, W_LEFT = 170, BALL_D = 54;
static const Color C_BG(255, 13, 13, 20), C_PURPLE(255, 130, 60, 220), C_TEXT(255, 210, 205, 225);

void DrawSwitch(Graphics& g, float x, float y, const wchar_t* label, bool on) {
    SolidBrush swBg(on ? C_PURPLE : Color(255, 35, 35, 45));
    g.FillRectangle(&swBg, (REAL)x, (REAL)y, (REAL)34, (REAL)16);
    SolidBrush kBr(Color(255, 255, 255, 255));
    g.FillEllipse(&kBr, on ? (REAL)(x + 18) : (REAL)(x + 2), (REAL)(y + 2), (REAL)12, (REAL)12);
    Font f(L"Segoe UI", 9.0f); SolidBrush b(C_TEXT);
    g.DrawString(label, -1, &f, PointF(x + 45, y - 2), &b);
}

// --- WNDPROCS ---
LRESULT CALLBACK PanelProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            Graphics g(hdc); g.SetSmoothingMode(SmoothingModeAntiAlias);
            SolidBrush bg(C_BG); g.FillRectangle(&bg, 0, 0, W_WIN, H_WIN);
            
            float sX = W_LEFT + 30;
            if (S.catIdx == 0) DrawSwitch(g, sX, 80, L"Enable Aimbot", S.aimEnabled);
            if (S.catIdx == 2) DrawSwitch(g, sX, 80, L"Bypass & Connect", S.bBypass);
            
            EndPaint(hwnd, &ps); return 0;
        }
        case WM_LBUTTONDOWN: {
            int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
            if (mx >= W_WIN - 25 && my <= 30) ExitProcess(0);
            if (mx >= W_WIN - 50 && mx <= W_WIN - 35 && my <= 30) {
                ShowWindow(hwnd, SW_HIDE);
                ShowWindow(FindWindow(L"SiriusBall", NULL), SW_SHOW);
            }
            if (S.catIdx == 2 && mx > W_LEFT + 20 && my >= 80 && my <= 100) AttachToGame();
            if (S.catIdx == 0 && mx > W_LEFT + 20 && my >= 80 && my <= 100) S.aimEnabled = !S.aimEnabled;
            
            InvalidateRect(hwnd, NULL, FALSE); return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// --- WINMAIN ---
int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    GdiplusStartupInput gsi; ULONG_PTR gst; GdiplusStartup(&gst, &gsi, NULL);
    
    WNDCLASSW wc = {0}; wc.lpfnWndProc = PanelProc; wc.hInstance = hI; wc.lpszClassName = L"SiriusPanel";
    RegisterClassW(&wc);
    
    HWND hP = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, L"SiriusPanel", L"Space Xit", WS_POPUP, 100, 100, W_WIN, H_WIN, NULL, NULL, hI, NULL);
    SetLayeredWindowAttributes(hP, RGB(1, 1, 1), 252, LWA_ALPHA | LWA_COLORKEY);
    
    ShowWindow(hP, nS);
    MSG m; while (GetMessage(&m, NULL, 0, 0)) { TranslateMessage(&m); DispatchMessage(&m); }
    GdiplusShutdown(gst); return 0;
}
