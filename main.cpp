#include <windows.h>
#include <d3d9.h>
#include <iostream>

// Variáveis do Hack
bool bAimbot = false;
float aimbotFov = 150.0f;
bool bEspVida = false, bEspBox = false, bEspSkeleton = false;
bool bFly = false, bNoClip = false, bUnlockAll = false;
bool bMinimized = false;

// Configuração da "Bolinha"
float ballX = 50, ballY = 50;

void ProcessCheats() {
    if (bFly) { /* Lógica de escrita na memória aqui */ }
    if (bAimbot) { /* Lógica de trava de mira aqui */ }
}

// Simulação da Interface Visual
void RenderUI() {
    if (bMinimized) {
        // Renderiza apenas um círculo roxo pequeno (Bolinha)
        std::cout << "[ SPACE ]" << std::flush; 
    } else {
        // Renderiza o Painel Completo
        std::cout << "\n--- SPACE XIT PREMIUM ---" << std::endl;
        std::cout << "[1] AIMBOT FOV: " << aimbotFov << std::endl;
        std::cout << "[2] ESP BOX: " << (bEspBox ? "ON" : "OFF") << std::endl;
        std::cout << "[3] FLY: " << (bFly ? "ON" : "OFF") << std::endl;
        std::cout << "[F2] MINIMIZAR PARA BOLINHA" << std::endl;
    }
}

int main() {
    SetConsoleTitleA("SpaceXit PC - Flex City");
    
    while (true) {
        // Tecla para Minimizar (Igual você pediu)
        if (GetAsyncKeyState(VK_F2) & 1) {
            bMinimized = !bMinimized;
            system("cls");
        }

        if (!bMinimized) {
            // Controles de Aimbot (0 a 500)
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) if (aimbotFov < 500) aimbotFov += 1.0f;
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) if (aimbotFov > 0) aimbotFov -= 1.0f;
            
            if (GetAsyncKeyState('1') & 1) bAimbot = !bAimbot;
            if (GetAsyncKeyState('2') & 1) bFly = !bFly;
        }

        RenderUI();
        ProcessCheats();
        Sleep(10);
    }
    return 0;
}
