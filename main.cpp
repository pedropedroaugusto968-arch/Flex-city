#define UNICODE
#include <windows.h>
#include <tlhelp32.h>
#include <gdiplus.h>
#include <vector>
#include <string>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;

// --- ESTRUTURA DE DADOS (SEU IMGUI + LÓGICA) ---
struct HackState {
    bool bBypass = false, bAimbot = false, bFly = false, bEsp = false;
    int  aimbotFov = 150;
    bool minimized = false;
    int  activeTab = 0;
    HANDLE hProcess = NULL;
    uintptr_t baseAddress = 0;
} S;

// --- SCANNER DE PATTERN (ESTILO SEARCH DO GAME GUARDIAN) ---
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
                        if (mask[j] == 'x' && buffer[i + j] != pattern[j]) {
                            found = false; break;
                        }
                    }
                    if (found) return (uintptr_t)mbi.BaseAddress + i;
                }
            }
        }
        start += mbi.RegionSize;
    }
    return 0;
}

// --- THREAD DE INJEÇÃO REAL-TIME ---
DWORD WINAPI InjectionLoop(LPVOID lpParam) {
    while (true) {
        if (S.bBypass && S.hProcess) {
            // LÓGICA DE FLY (Exemplo de Assinatura)
            if (S.bFly) {
                // Aqui você coloca os bytes que o GG acharia
                const BYTE flySig[] = { 0xDE, 0xAD, 0xBE, 0xEF }; // SUBSTITUIR PELOS BYTES REAIS
                uintptr_t addr = FindPattern(flySig, "xxxx");
                if (addr) {
                    float val = 500.0f;
                    WriteProcessMemory(S.hProcess, (LPVOID)addr, &val, sizeof(val), NULL);
                }
            }
            // Adicione aqui as outras funções (Aimbot, ESP, etc.)
        }
        Sleep(500); // Não sobrecarrega a CPU
    }
    return 0;
}

// --- FUNÇÃO DE ATTACH (SEU CÓDIGO) ---
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

// [INÍCIO DO DESIGN SIRIUS GDI+]
void DrawSwitch(Graphics& g, float x, float y, bool on, const wchar_t* label, RECT& hit) {
    SolidBrush track(on ? Color(255, 138, 43, 226) : Color(255, 60, 60, 60));
    g.FillRectangle(&track, x, y, 40, 20); // Simplificado para performance
    SolidBrush knob(Color(255, 255, 255, 255));
    g.FillRectangle(&knob, on ? x + 22 : x + 2, y + 2, 16, 16);
    Font f(L"Segoe UI", 10.0f); SolidBrush b(Color(255, 255, 255, 255));
    g.DrawString(label, -1, &f, PointF(x + 50, y), &b);
    hit = { (LONG)x, (LONG)y, (LONG)x + 150, (LONG)y + 20 };
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static RECT rcSw[4], rcTab[2], rcBall;
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            Graphics g(hdc);
            if (S.minimized) {
                SolidBrush p(Color(255, 138, 43, 226)); g.FillEllipse(&p, 0, 0, 50, 50);
                rcBall = {0,0,50,50};
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
            int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp); POINT pt = {mx, my};
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
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, L"SpaceXit", L"SpaceXit", WS_POPUP, 100, 100, 300, 400, NULL, NULL, hI, NULL);
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA); ShowWindow(hwnd, nS);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    GdiplusShutdown(gst); return 0;
}
