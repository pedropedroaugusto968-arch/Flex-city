#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>

// Configurações do Hack
bool bAimbot = false;
int aimbotFov = 150;
bool bEsp = false;
bool bFly = false;
bool bMinimized = false;

// Cores do Painel
#define COLOR_PURPLE RGB(128, 0, 128)
#define COLOR_BLACK  RGB(15, 15, 15)

// Função para achar o jogo (Bypass/Attach)
DWORD GetProcId(const char* procName) {
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);
        if (Process32First(hSnap, &procEntry)) {
            do {
                if (!_stricmp(procEntry.szExeFile, procName)) {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

// Lógica da Janela (Painel Flutuante)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);

            // Fundo do Painel
            HBRUSH hbg = CreateSolidBrush(COLOR_BLACK);
            FillRect(hdc, &rect, hbg);
            DeleteObject(hbg);

            // Borda Roxa
            HBRUSH hbr = CreateSolidBrush(COLOR_PURPLE);
            FrameRect(hdc, &rect, hbr);
            DeleteObject(hbr);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, COLOR_PURPLE);

            if (bMinimized) {
                // Desenha a "Bolinha"
                TextOutA(hdc, 10, 15, "[ SPACE ]", 9);
            } else {
                // Desenha o Menu
                TextOutA(hdc, 60, 10, "SPACE XIT PC - PREMIUM", 21);
                SetTextColor(hdc, RGB(255, 255, 255));
                TextOutA(hdc, 20, 50, bAimbot ? "[X] AIMBOT ON" : "[ ] AIMBOT OFF", 14);
                TextOutA(hdc, 20, 80, bFly ? "[X] FLY ON" : "[ ] FLY OFF", 11);
                TextOutA(hdc, 20, 110, bEsp ? "[X] ESP ON" : "[ ] ESP OFF", 11);
                
                char fovTxt[32];
                sprintf(fovTxt, "FOV: %d", aimbotFov);
                TextOutA(hdc, 20, 140, fovTxt, strlen(fovTxt));
                
                SetTextColor(hdc, COLOR_PURPLE);
                TextOutA(hdc, 20, 180, "F2 - MINIMIZAR / ABRIR", 22);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_NCHITTEST: return HTCAPTION; // Permite arrastar o painel
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    // Registrar Janela
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "SpaceXitMenu";
    RegisterClass(&wc);

    // Criar Janela (Painel Flutuante)
    HWND hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED, "SpaceXitMenu", "SpaceXit", WS_POPUP, 100, 100, 250, 220, NULL, NULL, hInst, NULL);
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    ShowWindow(hwnd, nShow);

    // Loop de Teclas e Funções
    MSG msg = { };
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Tecla F2 para Minimizar/Abrir
        if (GetAsyncKeyState(VK_F2) & 1) {
            bMinimized = !bMinimized;
            if (bMinimized) SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 80, 50, SWP_NOMOVE);
            else SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 250, 220, SWP_NOMOVE);
            InvalidateRect(hwnd, NULL, TRUE);
        }

        // Atalhos para as funções
        if (GetAsyncKeyState('1') & 1) { bAimbot = !bAimbot; InvalidateRect(hwnd, NULL, TRUE); }
        if (GetAsyncKeyState('2') & 1) { bFly = !bFly; InvalidateRect(hwnd, NULL, TRUE); }
        if (GetAsyncKeyState('3') & 1) { bEsp = !bEsp; InvalidateRect(hwnd, NULL, TRUE); }

        Sleep(10);
    }
    return 0;
}
