#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <tlhelp32.h> // Para o Scanner
#include <string>
#include <vector>
#include <cmath>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

using namespace Gdiplus;

// ── Dimensões do seu Visual ────────────────────────────────
static const int W_WIN       = 680;
static const int H_WIN       = 420;
static const int W_LEFT      = 170;
static const int H_TOPBAR    = 36;
static const int H_SUBTAB    = 34;
static const int BALL_D      = 54;

// ── Paleta Sirius ──────────────────────────────────────────
static const Color C_BG        = Color(255,  13,  13,  20);
static const Color C_LEFT      = Color(255,  10,   9,  16);
static const Color C_TOPBAR    = Color(255,  11,  10,  17);
static const Color C_CARD      = Color(255,  18,  16,  27);
static const Color C_CARD2     = Color(255,  22,  20,  33);
static const Color C_BORDER    = Color(255,  35,  28,  55);
static const Color C_BORDER2   = Color(255,  55,  35,  90);
static const Color C_PURPLE    = Color(255, 130,  60, 220);
static const Color C_PURPLE2   = Color(255, 160,  90, 255);
static const Color C_NEON      = Color(255, 190, 130, 255);
static const Color C_TEXT      = Color(255, 210, 205, 225);
static const Color C_TEXTDIM   = Color(255, 110, 100, 140);
static const Color C_TEXTFAINT = Color(255,  70,  65,  95);
static const Color C_GREEN     = Color(255,  70, 210, 130);
static const Color C_SEP       = Color(255,  30,  26,  45);

static ULONG_PTR g_gdipToken;

// ── ESTADO INTEGRADO (VISUAL + HACK) ───────────────────────
struct State {
    int  catIdx    = 0;
    int  subIdx    = 0;
    bool aimEnabled = false;
    bool aimLock    = false;
    bool triggerbot = false;
    int  fov        = 80;
    int  smooth     = 45;
    bool legitAim   = false;
    bool recoil     = false;
    int  legSmooth  = 60;
    bool esp        = false;
    bool skeleton   = false;
    bool snaplines  = false;
    bool radarHack  = false;
    bool keybind    = false;
    bool saveConfig = false;
    bool minimized  = false;
    bool dragging   = false;
    POINT dragOff   = {};
    
    // Dados do Processo
    HANDLE hProcess = NULL;
    DWORD procId    = 0;
    bool bBypass    = false;
    bool flyEnabled = false; // Adicionei para o Fly que você pediu
};
static State S;

// ── SISTEMA DE SCANNER (ESTILO GAME GUARDIAN) ──────────────
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

// ── THREAD DE INJEÇÃO (TRABALHA NO FUNDO) ──────────────────
DWORD WINAPI InjectionThread(LPVOID lpParam) {
    while (true) {
        if (S.bBypass && S.hProcess) {
            // Exemplo Fly: Procura os bytes e injeta
            if (S.flyEnabled) {
                const BYTE flySig[] = { 0xDE, 0xAD, 0xBE, 0xEF }; // TROCAR PELOS BYTES REAIS
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

// ── BYPASS / ATTACH ────────────────────────────────────────
void ApplyBypass() {
    HWND hwnd = FindWindowA(NULL, "Flex City");
    if (hwnd) {
        GetWindowThreadProcessId(hwnd, &S.procId);
        S.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, S.procId);
        if (S.hProcess) {
            S.bBypass = true;
            static bool threadCreated = false;
            if (!threadCreated) {
                CreateThread(NULL, 0, InjectionThread, NULL, 0, NULL);
                threadCreated = true;
            }
        }
    }
}

// [AQUI CONTINUA TODO O SEU CÓDIGO DE DESENHO GDI+ QUE VOCÊ MANDOU]
// (FillRR, DrawRR, TXT, DrawSwitch, DrawSlider, DrawCard, DrawContent...)
// Eu mantive as funções de desenho exatamente como você enviou.

// No DrawContent, adicionei a chamada do Bypass quando clicar no Switch:
static void DrawContent(Graphics& g, float cx, float cy, float cw2, float ch2) {
    // ... (restante do seu código)
    if (S.catIdx == 0 && S.subIdx == 0) {
        DrawCard(g, lx, ly, lw, 190, L"General");
        // Aqui conectamos o seu switch à função de Bypass e Fly
        DrawSwitch(g, lx+14, ly+30, L"Enable Bypass", S.bBypass, 99, &S.bBypass); 
        if (S.bBypass) ApplyBypass(); // Ativa o motor se ligado
        
        DrawSwitch(g, lx+14, ly+56, L"Enable Aimbot", S.aimEnabled, 1, &S.aimEnabled);
        DrawSwitch(g, lx+14, ly+82, L"Fly Mode", S.flyEnabled, 2, &S.flyEnabled);
    }
    // ... (restante do seu código de abas)
}

// [AQUI CONTINUA O RESTANTE DO SEU CÓDIGO: PaintPanel, PaintBall, WndProc, etc.]

// No WinMain, apenas certifique-se de iniciar o GDI+ e as janelas como você mandou.
