#include <windows.h>
#include <iostream>

// Variaveis do Hack
bool bAimbot = false;
int aimbotFov = 150;
bool bEsp = false;
bool bFly = false;
bool bMinimized = false;

void RenderMenu() {
    system("cls");
    std::cout << "====================================" << std::endl;
    std::cout << "   SPACE XIT PC - @eoo_gomes3       " << std::endl;
    std::cout << "====================================" << std::endl;
    
    if (bMinimized) {
        std::cout << "\n    [ SPACE ] (F2 para Abrir)" << std::endl;
    } else {
        std::cout << " [F2] MINIMIZAR" << std::endl;
        std::cout << "------------------------------------" << std::endl;
        std::cout << " [1] AIMBOT FOV [" << aimbotFov << "]: " << (bAimbot ? "ON" : "OFF") << std::endl;
        std::cout << "     (Setas Esq/Dir ajustam)" << std::endl;
        std::cout << " [2] ESP: " << (bEsp ? "ON" : "OFF") << std::endl;
        std::cout << " [3] FLY: " << (bFly ? "ON" : "OFF") << std::endl;
        std::cout << "------------------------------------" << std::endl;
    }
}

int main() {
    SetConsoleTitleA("SpaceXit - Flex City");
    RenderMenu();

    while (true) {
        if (GetAsyncKeyState(VK_F2) & 1) {
            bMinimized = !bMinimized;
            RenderMenu();
        }

        if (!bMinimized) {
            if (GetAsyncKeyState('1') & 1) { bAimbot = !bAimbot; RenderMenu(); }
            if (GetAsyncKeyState('2') & 1) { bEsp = !bEsp; RenderMenu(); }
            if (GetAsyncKeyState('3') & 1) { bFly = !bFly; RenderMenu(); }

            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
                if (aimbotFov < 500) { aimbotFov += 5; RenderMenu(); }
            }
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
                if (aimbotFov > 0) { aimbotFov -= 5; RenderMenu(); }
            }
        }
        Sleep(100);
    }
    return 0;
}
