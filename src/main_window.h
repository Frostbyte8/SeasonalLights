#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <windows.h>
#include <wrl.h>
#include <dxgi1_3.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <d2d1_2helper.h>
#include <dcomp.h>
#include <wincodec.h>
#include <wrl.h>
#include <string>
#include <vector>

// TODO: Put these in the proj
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dcomp")

#include "bulb.h"

using namespace Microsoft::WRL;

struct DXInfo {
    ComPtr<ID3D11Device>        D3DDevice;
    ComPtr<IDXGIDevice>         DXGIDevice;
    ComPtr<IDXGIFactory2>       dxFactory;
    ComPtr<IDXGISwapChain1>     swapChain;

    ComPtr<ID2D1Factory2>       d2Factory;
    ComPtr<ID2D1Device1>        d2Device;
    ComPtr<ID2D1DeviceContext>  dc;
    ComPtr<IDXGISurface2>       surface;

    ComPtr<ID2D1Bitmap1>        bitmap;
    ComPtr<IDCompositionDevice> dcompDevice;
    ComPtr<IDCompositionTarget> target;
    ComPtr<IDCompositionVisual> visual;

};

struct BulbT {
    const BulbInfo*             bulbInfo;
    unsigned __int8             currentFrame;
    BulbT() : bulbInfo(NULL), currentFrame(0) {}
};

namespace BlinkMode {
    enum blinkMode {
        DONT,
        TOGETHER,
        ALTERNATING,
        CHASE,
        RANDOM,
    };
}

class MainWindow {

    public:
        MainWindow() : dxInfo(NULL), wicFactory(NULL), gifDecoder(NULL), window(NULL), blinkMode(BlinkMode::RANDOM), waitTime(250) {}
        
        ~MainWindow() {
            delete dxInfo;
            dxInfo = NULL;
        }

        bool initCOM();
        bool registerSelf(HINSTANCE hInstance);
        bool createWindow(HINSTANCE hInstance);
        UINT doLoop();
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
        void setInitalSize();
        bool InitDirect2D();
        bool loadAssets();
        static const std::wstring className;
        bool OnPaint();
        void initBulbs();
        void updateBulbs();

    private:
        void loadGIF();
        LRESULT windowProc(const UINT& msg, const WPARAM wParam, const LPARAM lParam);
        HWND    window;
        DXInfo* dxInfo;
        ComPtr<IWICImagingFactory>      wicFactory;
        ComPtr<IWICBitmapDecoder>       gifDecoder;

        Bulb                            bulb;

        std::vector<BulbT>              cornerBulbs;
        std::vector<BulbT>              sideBulbs[4];

        LONG                            maxSideLength[4];
        LONG                            sideLength[4];
        unsigned __int32                blinkMode;
        LONG                            waitTime;

        /*
        std::vector<BulbType>           topBulbs;
        std::vector<BulbType>           leftBulbs;
        std::vector<BulbType>           bottomBulbs;
        std::vector<BulbType>           rightBulbs;
        std::vector<BulbType>           cornerBulbs;
        */

};




#endif // __MAIN_WINDOW_H__