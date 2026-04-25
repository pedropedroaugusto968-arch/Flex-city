#include <windows.h>
#include <iostream>
#include <tlhelp32.h>

// --- VARIAVEIS DE FUNÇÃO (LIGAR/DESLIGAR) ---
bool bBypass = true;
bool bAimbot = false;
int aimbotFov = 150; // Slider 0-500
bool bEspBox = false;
bool bEspVida = false;
bool bEspSkeleton = false;
bool bNoclip = false;
bool bFly = false;
bool bUnlockAll = false;

// --- ESTRUTURA PARA BOLINHA FLUTUANTE ---
bool bMinimized = false;
POINT ballPos = {50, 50}; // Posição inicial da bolinha
bool isDraggingBall = false;

// --- CONFIGURAÇÃO VISUAL (PALETA ROXA) ---
#define COLOR_PURPLE RGB(128, 0, 128)
#define COLOR_BLACK  RGB(10, 10, 10)
#define COLOR_WHITE  RGB(255, 255, 255)
#define COLOR_SWITCH_ON RGB(147, 112, 219)

// --- LÓGICA DE MEMÓRIA (OFFSETS EXEMPO) ---
// ADVISO: Voce PRECISA encontrar os OFFSETS REAIS usando Cheat Engine.
HANDLE hProcess = NULL;
uintptr_t baseAddress = 0; // Exemplo: 0x...
uintptr_t offset_player_z = 0x...; // Offset para a altura (FLY)

// Função para abrir o processo do jogo
bool AttachProcess(const char* procName) {
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
    if (procId) hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
    return hProcess != NULL;
}

// --- DESENHAR INTERFACE (ESTILO IMGUI) ---

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;

    switch (uMsg) {
        case WM_PAINT: {
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);

            // Fundo Preto/Quase Preto
            HBRUSH hBackground = CreateSolidBrush(COLOR_BLACK);
            FillRect(hdc, &rect, hBackground);
            DeleteObject(hBackground);

            // Borda Roxa
            HBRUSH hBorder = CreateSolidBrush(COLOR_PURPLE);
            FrameRect(hdc, &rect, hBorder);
            DeleteObject(hBorder);

            SetTextColor(hdc, COLOR_PURPLE);
            SetBkMode(hdc, TRANSPARENT);
            
            if (bMinimized) {
                // Desenhar a Bolinha Flutuante (X com círculo roxo)
                HFONT hFontBall = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
                SelectObject(hdc, hFontBall);
                
                // Círculo
                HBRUSH hCircle = CreateSolidBrush(COLOR_PURPLE);
                SelectObject(hdc, hCircle);
                Ellipse(hdc, 5, 5, 55, 55);
                DeleteObject(hCircle);

                SetTextColor(hdc, COLOR_WHITE);
                TextOutA(hdc, 15, 15, "SPACE", 5);
                DeleteObject(hFontBall);
            } else {
                // Desenhar o Painel Completo
                HFONT hFontTitle = CreateFontA(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
                SelectObject(hdc, hFontTitle);
                TextOutA(hdc, 50, 10, "SPACE XIT PC - PREMIUM", 21);
                DeleteObject(hFontTitle);

                HFONT hFontItem = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
                SelectObject(hdc, hFontItem);
                
                SetTextColor(hdc, COLOR_WHITE);
                TextOutA(hdc, 20, 40, "[F2] Minimizar", 14);
                
                // Simular Botões de Liga/Desliga (Switch)
                TextOutA(hdc, 20, 70, "BYPASS ANTICHEAT", 16);
                TextOutA(hdc, 220, 70, (bBypass ? "● ON" : "○ OFF"), 6);
                
                TextOutA(hdc, 20, 100, "AIMBOT (Use Setas Esq/Dir)", 25);
                char aimbotText[32]; sprintf(aimbotText, "○ %d FOV", aimbotFov);
                TextOutA(hdc, 220, 100, aimbotText, strlen(aimbotText));
                
                TextOutA(hdc, 20, 130, "ESP (Box, Skeleton, Vida)", 24);
                
                TextOutA(hdc, 20, 160, "GOD MODE / NOCLIP", 17);
                TextOutA(hdc, 220, 160, (bNoclip ? "● ON" : "○ OFF"), 6);
                
                TextOutA(hdc, 20, 190, "FLY (W/S para Altura)", 19);
                TextOutA(hdc, 220, 190, (bFly ? "● ON" : "○ OFF"), 6);

                DeleteObject(hFontItem);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            if (bMinimized) {
                isDraggingBall = true;
                SetCapture(hwnd);
                GetCursorPos(&ballPos);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (isDraggingBall) {
                POINT curPos;
                GetCursorPos(&curPos);
                int dx = curPos.x - ballPos.x;
                int dy = curPos.y - ballPos.y;
                RECT rect;
                GetWindowRect(hwnd, &rect);
                SetWindowPos(hwnd, NULL, rect.left + dx, rect.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                ballPos = curPos;
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            isDraggingBall = false;
            ReleaseCapture();
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// --- LOOP PRINCIPAL DO HACK (LOGICA DE FUNÇÕES) ---

DWORD WINAPI CheatLoop(LPVOID lpParam) {
    while (true) {
        if (!hProcess) AttachProcess("FlexCity.exe"); // Tenta conectar ao jogo

        if (hProcess && bFly) {
            // LÓGICA DE FLY: Ler altura, adicionar valor, escrever altura
            float currentZ;
            // Exemplo de leitura/escrita de memoria
            ReadProcessMemory(hProcess, (LPVOID)(offset_player_z), &currentZ, sizeof(currentZ), NULL);
            if (GetAsyncKeyState('W') & 0x8000) currentZ += 0.5f;
            if (GetAsyncKeyState('S') & 0x8000) currentZ -= 0.5f;
            WriteProcessMemory(hProcess, (LPVOID)(offset_player_z), &currentZ, sizeof(currentZ), NULL);
        }

        // --- CONTROLES DE MENU NO TECLADO ---
        if (GetAsyncKeyState(VK_F2) & 1) { // Minimizar para bolinha
            bMinimized = !bMinimized;
            HWND hwnd = (HWND)lpParam;
            if (bMinimized) {
                SetWindowPos(hwnd, HWND_TOPMOST, 50, 50, 60, 60, SWP_SHOWWINDOW);
            } else {
                SetWindowPos(hwnd, HWND_TOPMOST, 100, 100, 320, 320, SWP_SHOWWINDOW);
            }
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        }

        if (!bMinimized) {
            // Ativação por teclas (Substituindo botões switch por enquanto)
            if (GetAsyncKeyState('N') & 1) { bNoclip = !bNoclip; RedrawWindow((HWND)lpParam, NULL, NULL, RDW_INVALIDATE); }
            if (GetAsyncKeyState('M') & 1) { bFly = !bFly; RedrawWindow((HWND)lpParam, NULL, NULL, RDW_INVALIDATE); }
            
            // Slider FOV Aimbot (Setas)
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { if (aimbotFov < 500) aimbotFov += 5; RedrawWindow((HWND)lpParam, NULL, NULL, RDW_INVALIDATE); }
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) { if (aimbotFov > 0) aimbotFov -= 5; RedrawWindow((HWND)lpParam, NULL, NULL, RDW_INVALIDATE); }
        }

        Sleep(10);
    }
    return 0;
}

// --- ENTRADA DO PROGRAMA ---

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Esconder console
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    // Conectar ao jogo
    if (!AttachProcess("FlexCity.exe")) {
        MessageBoxA(NULL, "[-] Abra o Flex City primeiro!", "SPACE XIT", MB_ICONERROR);
        // return 0; // Descomente para fechar se o jogo nao estiver aberto
    }

    // Criar a janela do Hack
    const char CLASS_NAME[] = "SpaceXitMenuPC";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED, CLASS_NAME, "SpaceXit - Flex City", WS_POPUP, 100, 100, 320, 320, NULL, NULL, hInstance, NULL);
    
    // Tornar fundo transparente (estilo overlay)
    SetLayeredWindowAttributes(hwnd, COLOR_BLACK, 0, LWA_COLORKEY);
    
    ShowWindow(hwnd, nCmdShow);

    // Iniciar o loop das funções em segundo plano
    CreateThread(NULL, 0, CheatLoop, hwnd, 0, NULL);

    // Loop de mensagens do Windows (Janela)
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hProcess) CloseHandle(hProcess);
    return 0;
}
