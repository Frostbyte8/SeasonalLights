#include "main_window.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    MainWindow mainWindow;

    if(!mainWindow.initCOM()) {
    }
    
    if(!mainWindow.registerSelf(hInstance)) {
        return 0;
    }

    if(!mainWindow.createWindow(hInstance)) {
        return 0;
    }

    mainWindow.setInitalSize();

    if(!mainWindow.InitDirect2D()) {
        return 0;
    }

    // Load assets requires InitDirect2D, 
    mainWindow.loadAssets();

    return mainWindow.doLoop();
}