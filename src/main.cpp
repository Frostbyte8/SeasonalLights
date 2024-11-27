#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
#include "main_window.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpFlag);

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
    if(!mainWindow.loadAssets()) {
        MessageBox(NULL, L"Unable to load Bulb.bul. The file is missing or corrupt.", L"Bulb.bul not found", MB_OK | MB_ICONERROR);
        return 0;
    }

    return mainWindow.doLoop();
}