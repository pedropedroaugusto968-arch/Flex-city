#include <windows.h>
#include <iostream>

// Configurações do Hack
bool bAimbot = false;
int aimbotFov = 150; // 0 a 500
bool bEspVida = false, bEspBox = false, bEspSkeleton = false;
bool bFly = false, bNoClip = false, bUnlockAll = false;
bool bBypass = true;

// Estado do Painel
bool bMinimized = false;

void RenderMenu() {
    system("cls");
    std::cout << "====================================" << std::endl;
    std::cout << "   SPACE XIT PC - @eoo_gomes3       " << std::endl;
    std::cout << "====================================" << std::endl;
    
    if (bMinimized) {
        std::cout << "\n    [ SPACE ] <-- Bolinha Ativa" << std::endl;
        std::cout << "\n (Pressione F2 para abrir o Painel)" << std::endl;
    } else {
        std::cout << " [F2] MINIMIZAR PARA BOLINHA" << std::endl;
        std::cout << "------------------------------------" << std::endl;
        std::cout << " [1] BYPASS: " << (bBypass ? "ON" : "OFF") << std::endl;
        std::cout << " [2] AIMBOT FOV [" << aimbotFov << "]: " << (bAimbot ? "ON" : "OFF") << std::endl;
        std::cout << "     (Use SETAS Esq/Dir para ajustar)" << std::endl;
        std::cout << " [3] ESP (BOX/VIDA/SKELETON): " << (bEspBox ? "ON" : "OFF") << std::endl;
        std::cout << " [4] FLY / NOCLIP: " << (bFly ? "ON" : "OFF") << std::endl;
        std::cout << " [5] UNLOCK ALL: " << (bUnlockAll ? "ON" : "OFF") << std::endl;
        std::cout << "------------------------------------" << std::endl;
        std::cout << " [END] FECHAR HACK" << std::endl;
    }
}

int main() {
    SetConsoleTitleA("SpaceXit - Flex City Edition");
    RenderMenu();

    while (true) {
        // Alternar entre Painel e Bolinha
        if (GetAsyncKeyState(VK_F2) & 1) {
            bMinimized = !bMinimized;
            RenderMenu();
        }

        if (!bMinimized) {
            // Ativação das funções
            if (GetAsyncKeyState('1') & 1) { bBypass = !bBypass; RenderMenu(); }
            if (GetAsyncKeyState('2') & 1) { bAimbot = !bAimbot; RenderMenu(); }
            if (GetAsyncKeyState('3') & 1) { bEspBox = !bEspBox; bEspVida = !bEspVida; RenderMenu(); }
            if (GetAsyncKeyState('4') & 1) { bFly = !bFly; RenderMenu(); }

            // Ajuste do FOV do Aimbot (0 a 500)
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
                if (aimbotFov < 500) { aimbotFov += 5; RenderMenu(); }
            }
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
                if (aimbotFov > 0) { aimbotFov -= 5; RenderMenu(); }
            }
        }

        if (GetAsyncKeyState(VK_END) & 1) break;
        Sleep(100);
    }
    return 0;
}
