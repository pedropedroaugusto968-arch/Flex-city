#include <windows.h>
#include <tlhelp32.h>
#include <iostream>

// --- ESTADOS DAS FUNÇÕES ---
bool bBypass = false;
bool bAimbot = false;
bool bFly = false;
bool bEsp = false;
bool bMinimized = false;
int aimbotFov = 150;

// --- CORES ---
#define COLOR_PURPLE RGB(160, 32, 240)
#define COLOR_GRAY   RGB(60, 60, 60)
#define COLOR_BLACK  RGB(15, 15, 15)
#define COLOR_WHITE  RGB(255, 255, 255)

// --- FUNÇÃO BYPASS (ATTACH) ---
void ApplyBypass() {
    HWND hwnd = FindWindowA(NULL, "Flex City"); // Nome da janela do jogo
    if (hwnd) {
        DWORD procId;
        GetWindowThreadProcessId(hwnd, &procId);
        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
        if (hProc) {
            bBypass = true;
            CloseHandle(hProc);
        }
    }
}

// --- DESENHO DA INTERFACE ---
void DrawSwitch(HDC hdc, int x, int y, bool state) {
    HBRUSH hBrush = CreateSolidBrush(state ? COLOR_PURPLE : COLOR_GRAY);
    RECT r = { x, y, x + 40, y + 20 };
    FillRect(hdc, &r, hBrush);
    DeleteObject(hBrush);
    
    // Pequeno indicador dentro do switch
    HBRUSH hKnob = CreateSolidBrush(COLOR_WHITE);
    RECT k = { state ? x + 22 : x + 2, y + 2, state ? x + 38 : x + 18, y + 18 };
    FillRect(hdc, &k, hKnob);
    DeleteObject(hKnob);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect; GetClientRect(hwnd, &rect);
            HBRUSH hbg = CreateSolidBrush(COLOR_BLACK);
            FillRect(hdc, &rect, hbg); DeleteObject(hbg);
            HBRUSH hbr = CreateSolidBrush(COLOR_PURPLE);
            FrameRect(hdc, &rect, hbr); DeleteObject(hbr);

            SetBkMode(hdc, TRANSPARENT);
            if (bMinimized) {
                SetTextColor(hdc, COLOR_PURPLE);
                TextOutA(hdc, 10, 15, "[ SPACE ]", 9);
            } else {
                SetTextColor(hdc, COLOR_PURPLE);
                TextOutA(hdc, 45, 10, "SPACE XIT - PREMIUM", 19);
                
                SetTextColor(hdc, COLOR_WHITE);
                TextOutA(hdc, 20, 50, "BYPASS GAME", 11); DrawSwitch(hdc, 180, 50, bBypass);
                TextOutA(hdc, 20, 85, "AIMBOT", 6);       DrawSwitch(hdc, 180, 85, bAimbot);
                TextOutA(hdc, 20, 120, "FLY MODE", 8);    DrawSwitch(hdc, 180, 120, bFly);
                TextOutA(hdc, 20, 155, "ESP VISUAL", 10);  DrawSwitch(hdc, 180, 155, bEsp);
                
                char fov[20]; sprintf(fov, "FOV: %d", aimbotFov);
                TextOutA(hdc, 20, 190, fov, strlen(fov));
            }
            EndPaint(hwnd, &ps); return 0;
        }
        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam); int y = HIWORD(lParam);
            if (!bMinimized) {
                if (y > 45 && y < 75) ApplyBypass();
                if (y > 80 && y < 110) bAimbot = !bAimbot;
                if (y > 115 && y < 145) bFly = !bFly;
                if (y > 150 && y < 180) bEsp = !bEsp;
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
        }
        case WM_NCHITTEST: return HTCAPTION;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE hP, LPSTR lp, int nS) {
    WNDCLASS wc = { }; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = "SpaceXit";
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED, "SpaceXit", "SpaceXit", WS_POPUP, 100, 100, 250, 230, NULL, NULL, hI, NULL);
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    ShowWindow(hwnd, nS);

    MSG msg = { };
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg); DispatchMessage(&msg);
        }
        if (GetAsyncKeyState(VK_F2) & 1) {
            bMinimized = !bMinimized;
            SetWindowPos(hwnd, NULL, 0, 0, bMinimized ? 80 : 250, bMinimized ? 50 : 230, SWP_NOMOVE | SWP_NOZORDER);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        if (!bMinimized) {
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { if (aimbotFov < 500) aimbotFov += 2; InvalidateRect(hwnd, NULL, TRUE); }
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) { if (aimbotFov > 0) aimbotFov -= 2; InvalidateRect(hwnd, NULL, TRUE); }
        }
        Sleep(10);
    }
    return 0;
}
