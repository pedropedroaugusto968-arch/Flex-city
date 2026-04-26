#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h> // <--- ISSO RESOLVE O ERRO GET_X_LPARAM
#include <tlhelp32.h>
#include <gdiplus.h>
#include <vector>
#include <string>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;

struct HackState {
    bool bBypass = false, bAimbot = false, bFly = false, bEsp = false;
    int  aimbotFov = 150;
    bool minimized = false;
    int  activeTab = 0;
    HANDLE hProcess = NULL;
    uintptr_t baseAddress = 0;
} S;

// --- SCANNER DE PATTERN ---
uintptr_t FindPattern(const BYTE* pattern, const char* mask) {
    if (!S.hProcess) return 0;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uintptr_t start = (uintptr_t)si.lpMinimumApplicationAddress;
    uintptr_t end = (uintptr_t)si.lpMaximumApplicationAddress;
    size_t patternLen = strlen(mask);

    while (start < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQueryEx(S.hProcess, (LPCVOID)start, &mbi, sizeof(mbi))) break;
        if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_READWRITE || mbi.Protect & PAGE_EXECUTE_READWRITE)) {
            std::vector<BYTE> buffer(mbi.RegionSize);
            if (ReadProcessMemory(S.hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, NULL)) {
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

DWORD WINAPI InjectionLoop(LPVOID lpParam) {
    while (true) {
        if (S.bBypass && S.hProcess) {
            if (S.bFly) {
                const BYTE flySig[] = { 0xDE, 0xAD, 0xBE, 0xEF }; 
                uintptr_t addr = FindPattern(flySig, "xxxx");
                if (addr) {
                    float val = 500.0f;
                    WriteProcessMemory(S.hProcess, (LPVOID)addr, &val, sizeof(val), NULL);
                }
            }
        }
        Sleep(500);
    }
    return 0;
}

void ApplyBypass() {
    HWND hwnd = FindWindowA(NULL, "Flex City");
    if (hwnd) {
        DWORD pId;
        GetWindowThreadProcessId(hwnd, &pId);
        S.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pId);
        if (S.hProcess) {
            S.bBypass = true;
            CreateThread(NULL, 0, InjectionLoop, NULL, 0, NULL);
        }
    }
}

// --- AJUSTE NA FUNÇÃO DE DESENHO ---
void DrawSwitch(Graphics& g, float x, float y, bool on, const wchar_t* label, RECT& hit) {
    SolidBrush track(on ? Color(255, 138, 43, 226) : Color(255, 60, 60, 60));
    // Usando (REAL) para converter float para o tipo que o GDI+ aceita
    g.FillRectangle(&track, (REAL)x, (REAL)y, (REAL)40.0, (REAL)20.0); 
    
    SolidBrush knob(Color(255, 255, 255, 255));
    g.FillRectangle(&knob, on ? (REAL)(x + 22) : (REAL)(x + 2), (REAL)(y + 2), (REAL)16.0, (REAL)16.0);
    
    Font f(L"Segoe UI", 10.0f); 
    SolidBrush b(Color(255, 255, 255, 255));
    g.DrawString(label, -1, &f, PointF(x + 50, y), &b);
    hit.left = (LONG)x; hit.top = (LONG)y; hit.right = (LONG)(x + 150); hit.bottom = (LONG)(y + 20);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static RECT rcSw[4], rcBall;
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            Graphics g(hdc);
            if (S.minimized) {
                SolidBrush p(Color(255, 138, 43, 226)); g.FillEllipse(&p, 0, 0, 50, 50);
                rcBall.left = 0; rcBall.top = 0; rcBall.right = 50; rcBall.bottom = 50;
            } else {
                SolidBrush bg(Color(255, 16, 14, 28)); g.FillRectangle(&bg, 0, 0, 300, 400);
                DrawSwitch(g, 20, 60, S.bBypass, L"BYPASS", rcSw[0]);
                DrawSwitch(g, 20, 100, S.bAimbot, L"AIMBOT", rcSw[1]);
                DrawSwitch(g, 20, 140, S.bFly, L"FLY MODE", rcSw[2]);
                DrawSwitch(g, 20, 180, S.bEsp, L"ESP", rcSw[3]);
            }
            EndPaint(hwnd, &ps); return 0;
        }
        case WM_LBUTTONDOWN: {
            int mx = GET_X_LPARAM(lp); 
            int my = GET_Y_LPARAM(lp); 
            POINT pt = {mx, my};
            if (S.minimized) { S.minimized = false; SetWindowPos(hwnd, NULL, 0, 0, 300, 400, SWP_NOMOVE); }
            else {
                if (PtInRect(&rcSw[0], pt)) ApplyBypass();
                if (PtInRect(&rcSw[1], pt)) S.bAimbot = !S.bAimbot;
                if (PtInRect(&rcSw[2], pt)) S.bFly = !S.bFly;
                if (PtInRect(&rcSw[3], pt)) S.bEsp = !S.bEsp;
            }
            InvalidateRect(hwnd, NULL, FALSE); return 0;
        }
        case WM_NCHITTEST: return HTCAPTION;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    GdiplusStartupInput gsi; ULONG_PTR gst; GdiplusStartup(&gst, &gsi, NULL);
    WNDCLASSW wc = {0}; wc.lpfnWndProc = WndProc; wc.hInstance = hI; wc.lpszClassName = L"SpaceXit";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, L"SpaceXit", L"SpaceXit", WS_POPUP, 100, 100, 300, 400, NULL, NULL, hI, NULL);
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA); ShowWindow(hwnd, nS);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    GdiplusShutdown(gst); return 0;
}
