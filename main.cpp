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

// --- ESTRUTURAS ---
struct State {
    int catIdx = 0;
    bool bBypass = false;
    bool flyEnabled = false;
    HANDLE hProcess = NULL;
} S;

// --- SCANNER ---
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

// --- DESENHO CORRIGIDO (REAL) ---
void DrawSwitch(Graphics& g, float x, float y, const wchar_t* label, bool on) {
    // Usando REAL para evitar o erro C2666 que apareceu no seu print
    SolidBrush tBr(on ? Color(200, 130, 60, 220) : Color(255, 40, 40, 40));
    g.FillRectangle(&tBr, (REAL)x, (REAL)y, (REAL)34.0, (REAL)16.0);
    
    SolidBrush kBr(Color(255, 255, 255, 255));
    g.FillEllipse(&kBr, on ? (REAL)(x + 18.0) : (REAL)(x + 2.0), (REAL)(y + 2.0), (REAL)12.0, (REAL)12.0);
    
    Font f(L"Arial", 9.0f);
    SolidBrush b(Color(255, 255, 255, 255));
    g.DrawString(label, -1, &f, PointF(x + 40, y), &b);
}

// --- WNDPROC ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            Graphics g(hdc);
            SolidBrush bg(Color(255, 15, 15, 25));
            g.FillRectangle(&bg, (REAL)0, (REAL)0, (REAL)680.0, (REAL)420.0);
            DrawSwitch(g, 200, 100, L"Bypass Flex City", S.bBypass);
            EndPaint(hwnd, &ps); return 0;
        }
        case WM_LBUTTONDOWN: {
            // As macros agora funcionam com o windowsx.h no lugar certo
            int mx = GET_X_LPARAM(lp);
            int my = GET_Y_LPARAM(lp);
            if (mx > 200 && mx < 234 && my > 100 && my < 116) {
                S.bBypass = !S.bBypass;
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    GdiplusStartupInput gsi; ULONG_PTR gst; GdiplusStartup(&gst, &gsi, NULL);
    WNDCLASSW wc = {0}; wc.lpfnWndProc = WndProc; wc.hInstance = hI; wc.lpszClassName = L"SpaceXit";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, L"SpaceXit", L"Space Xit", WS_POPUP, 100, 100, 680, 420, NULL, NULL, hI, NULL);
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA); ShowWindow(hwnd, nS);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    GdiplusShutdown(gst); return 0;
}
