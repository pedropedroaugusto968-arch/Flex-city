#include <windows.h>
#include <iostream>
#include <vector>

// Variáveis de Configuração do Hack
bool bAimbot = false;
int aimbotFov = 150;
bool bEspVida = false;
bool bEspBox = false;
bool bEspSkeleton = false;
bool bBypass = true;
bool bUnlockAll = false;
bool bNoClip = false;
bool bFly = false;

// Lógica do Painel
bool bMinimized = false;
bool bShowMenu = true;

void RenderMenu() {
    system("cls");
    std::cout << "--- SPACE XIT PC | @eoo_gomes3 ---" << std::endl;
    if (bMinimized) {
        std::cout << "[ O ] BOLINHA FLUTUANTE (Pressione F2 para Expandir)" << std::endl;
    } else {
        std::cout << "Pressione F2 para Minimizar" << std::endl;
        std::cout << "---------------------------------" << std::endl;
        std::cout << "[1] BYPASS: " << (bBypass ? "ON" : "OFF") << std::endl;
        std::cout << "[2] AIMBOT (FOV: " << aimbotFov << "): " << (bAimbot ? "ON" : "OFF") << std::endl;
        std::cout << "[3] ESP BOX: " << (bEspBox ? "ON" : "OFF") << std::endl;
        std::cout << "[4] ESP VIDA: " << (bEspVida ? "ON" : "OFF") << std::endl;
        std::cout << "[5] FLY: " << (bFly ? "ON" : "OFF") << std::endl;
        std::cout << "[6] UNLOCK ALL: " << (bUnlockAll ? "ON" : "OFF") << std::endl;
        std::cout << "---------------------------------" << std::endl;
        std::cout << "Setas Esq/Dir para ajustar FOV do Aimbot" << std::endl;
    }
}

int main() {
    SetConsoleTitleA("SpaceXit - Flex City");
    
    // Loop Principal
    while (true) {
        // Tecla para Minimizar/Expandir (Simulando a Bolinha)
        if (GetAsyncKeyState(VK_F2) & 1) {
            bMinimized = !bMinimized;
            RenderMenu();
        }

        if (!bMinimized) {
            // Controles do Menu
            if (GetAsyncKeyState('1') & 1) { bBypass = !bBypass; RenderMenu(); }
            if (GetAsyncKeyState('2') & 1) { bAimbot = !bAimbot; RenderMenu(); }
            
            // Ajuste do FOV Aimbot (0 a 500)
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
                if (aimbotFov < 500) { aimbotFov += 5; RenderMenu(); }
            }
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
                if (aimbotFov > 0) { aimbotFov -= 5; RenderMenu(); }
            }

            if (GetAsyncKeyState('5') & 1) { bFly = !bFly; RenderMenu(); }
        }

        // Simulação da lógica de Fly/NoClip
        if (bFly) {
            // Aqui entraria a escrita de memória: WriteProcessMemory(...)
        }

        Sleep(100);
    }
    return 0;
}
