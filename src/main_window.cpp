#include "main_window.h"
#include "resources\resource.h"

#include <stdlib.h>
#include <time.h>

const std::wstring MainWindow::className = std::wstring(L"HolidayLightsMainWindow");
#define IS_OK(x) if(x != S_OK) { return false; }

//-----------------------------------------------------------------------------
// initCom - Initializes the COM interface, as well as a few COM related
// objects.
//-----------------------------------------------------------------------------

bool MainWindow::initCOM() {
    // Do some COM related stuff first

    if(CoInitializeEx(NULL, 0) != S_OK) {
        MessageBox(NULL, L"Error Initalizing COM.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory)) != S_OK) {
        MessageBox(NULL, L"Error Initalizing WIC Imaging Factory.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if(CoCreateInstance(CLSID_WICGifDecoder, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&gifDecoder)) != S_OK) {
        MessageBox(NULL, L"Error Initalizing GIF decoder.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
// registerSelf - Registers the Window class.
//-----------------------------------------------------------------------------

bool MainWindow::registerSelf(HINSTANCE hInstance) {
    
    WNDCLASSEX wc ={ 0 };

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MainWindow::WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = MainWindow::className.c_str();
    wc.hIconSm       = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON));

    if(!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Error registering window class.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
// createWindow - Creates the main Window.
//-----------------------------------------------------------------------------

bool MainWindow::createWindow(HINSTANCE hInstance) {

    bulbCollection.loadBulb("Bulb.bul");

    if(window) {
        return true; // Already created.
    }

    /*
    window = CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST,
        MainWindow::className.c_str(),
        L"",
        WS_VISIBLE | WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
        NULL, NULL, hInstance, this);
    */

    window = CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_LAYERED,
        MainWindow::className.c_str(),
        L"",
        WS_VISIBLE | WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
        NULL, NULL, hInstance, this);

    if(window == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    srand(time(NULL));

    return true;
}

//-----------------------------------------------------------------------------
// doLoop - Standard run of the mill message loop
//-----------------------------------------------------------------------------

WPARAM MainWindow::doLoop() {
    
    SetTimer(window, 0, waitTime, NULL);

    MSG msg;

    while(GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(window, 0);

    return msg.wParam;
}

//-----------------------------------------------------------------------------
// loadAssets - Load assets releated to the program
//-----------------------------------------------------------------------------

bool MainWindow::loadAssets() {
    // TODO: load Assets should just load the GIF data, and then after
    // D2D is ready to go, then create the Bitmaps.
    if(!loadGIF()) {
        return false;
    }
    initBulbs();
    return true;
}

//-----------------------------------------------------------------------------
// setInitalSize - resize window to fit monitor
//-----------------------------------------------------------------------------

void MainWindow::setInitalSize() {
    HMONITOR hMonitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX mi;
    mi.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &mi);

    MoveWindow(window, mi.rcMonitor.left, mi.rcMonitor.top, 
                       mi.rcMonitor.right - mi.rcMonitor.left, 
                       mi.rcMonitor.bottom - mi.rcMonitor.top, FALSE);

}

//-----------------------------------------------------------------------------
// InitDirect2D - Startup/Reinit Direct2D
//-----------------------------------------------------------------------------

bool MainWindow::InitDirect2D() {

    // TODO: Turns out, this doesn't actually release any resources. Go Figure.
    if(dxInfo) {
        delete dxInfo;
    }

    dxInfo = new DXInfo();

    HRESULT status;

    // Create our D3D11 Device.

    status = D3D11CreateDevice(NULL,
                               D3D_DRIVER_TYPE_HARDWARE,
                               NULL,
                               D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                               NULL,
                               0,
                               D3D11_SDK_VERSION,
                               &(dxInfo->D3DDevice),
                               NULL,
                               NULL);
    IS_OK(status);

    // And let us use it as a DXGIDevice Interface
    status = dxInfo->D3DDevice.As(&(dxInfo->DXGIDevice));
    IS_OK(status);

    // Next, create a DXGIFactory so we can use it to create a swap chain
    status = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, __uuidof(dxInfo->dxFactory), reinterpret_cast<void**>(dxInfo->dxFactory.GetAddressOf()));
    IS_OK(status);

    // Now we'll describe how we want our Swap Chain to behave
    DXGI_SWAP_CHAIN_DESC1 desc = { 0 };
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SampleDesc.Count = 1;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    RECT rect ={ 0 };
    GetClientRect(window, &rect);
    desc.Width = rect.right - rect.left;
    desc.Height = rect.bottom - rect.top;

    status = dxInfo->dxFactory->CreateSwapChainForComposition(dxInfo->DXGIDevice.Get(), &desc, nullptr, dxInfo->swapChain.GetAddressOf());
    IS_OK(status);

    // Next we need to setup Direct2D so we can get a Device Context to draw with,
    // This also links it back to our Direct3D 11 device

	D2D1_FACTORY_OPTIONS const options = { D2D1_DEBUG_LEVEL_INFORMATION };

    status = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, dxInfo->d2Factory.GetAddressOf());
    IS_OK(status);

    status = dxInfo->d2Factory->CreateDevice(dxInfo->DXGIDevice.Get(), dxInfo->d2Device.GetAddressOf());
    IS_OK(status);

    status = dxInfo->d2Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, dxInfo->dc.GetAddressOf());
    IS_OK(status);

 	// Retrieve the swap chain's back buffer
	status = dxInfo->swapChain->GetBuffer(0, __uuidof(dxInfo->surface), reinterpret_cast<void**>(dxInfo->surface.GetAddressOf()));
    IS_OK(status);

	// Create a Direct2D bitmap that points to the swap chain surface
	D2D1_BITMAP_PROPERTIES1 properties = {};
	properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	properties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

    status = dxInfo->dc->CreateBitmapFromDxgiSurface(dxInfo->surface.Get(), properties, dxInfo->bitmap.GetAddressOf());
    IS_OK(status);

    dxInfo->dc->SetTarget(dxInfo->bitmap.Get());

    // Finally, we just need to setup a Composition object and set it up.

    status = DCompositionCreateDevice(dxInfo->DXGIDevice.Get(), __uuidof(dxInfo->dcompDevice), reinterpret_cast<void**>(dxInfo->dcompDevice.GetAddressOf()));
    IS_OK(status);

    status = dxInfo->dcompDevice->CreateTargetForHwnd(window, true, dxInfo->target.GetAddressOf());
    IS_OK(status);

    status = dxInfo->dcompDevice->CreateVisual(dxInfo->visual.GetAddressOf());
    IS_OK(status);

    status = dxInfo->visual->SetContent(dxInfo->swapChain.Get());
    IS_OK(status);

    status = dxInfo->target->SetRoot(dxInfo->visual.Get());
    IS_OK(status);

    status = dxInfo->dcompDevice->Commit();
    IS_OK(status);
    
    return true;
}

//-----------------------------------------------------------------------------
// loadGIF - Convert's GIFs to Direct2D Bitmaps. Mostly just for debugging.
//-----------------------------------------------------------------------------

bool MainWindow::loadGIF() {

    const Bulb* bulb = bulbCollection.getBulbByID("Bulb.bul", wicFactory.Get(), gifDecoder.Get(), dxInfo->dc.Get());

    if(!bulb) {   
        return false;
    }

    //bulb.loadBulb("Lights\\RedBulb.bul");
    //bulb.initBitmaps(wicFactory.Get(), gifDecoder.Get(), dxInfo->dc.Get());
    
    // TODO: Move this to it's own function

    const std::vector<BulbInfo>& bulbInfoVec = bulb->getBulbInfoVec();
    const std::vector<unsigned __int32> cornerIDs = bulb->getCornerIDsVec();
    
    RECT rc;
    GetClientRect(window, &rc);

    // Set inital sizes, the deduct corner lengths
    maxSideLength[SideID::TOP]      = maxSideLength[SideID::TOP] = rc.right - rc.left;;
    maxSideLength[SideID::BOTTOM]   = maxSideLength[SideID::BOTTOM] = rc.right - rc.left;
    maxSideLength[SideID::LEFT]     = maxSideLength[SideID::LEFT] = rc.bottom - rc.top;
    maxSideLength[SideID::RIGHT]    = maxSideLength[SideID::RIGHT] = rc.bottom - rc.top;
    
    maxSideLength[SideID::TOP]      -= bulbInfoVec[cornerIDs[CornerID::TOP_LEFT]].width + bulbInfoVec[cornerIDs[CornerID::TOP_RIGHT]].width;
    maxSideLength[SideID::BOTTOM]   -= bulbInfoVec[cornerIDs[CornerID::BOTTOM_LEFT]].width + bulbInfoVec[cornerIDs[CornerID::BOTTOM_RIGHT]].width;

    maxSideLength[SideID::LEFT]     -= bulbInfoVec[cornerIDs[CornerID::TOP_LEFT]].height + bulbInfoVec[cornerIDs[CornerID::BOTTOM_LEFT]].height;
    maxSideLength[SideID::RIGHT]    -= bulbInfoVec[cornerIDs[CornerID::TOP_RIGHT]].height + bulbInfoVec[cornerIDs[CornerID::BOTTOM_RIGHT]].height;
    
    // Keep this loop unrolled, it's small enough.
    sideLength[0] = 0;
    sideLength[1] = 0;
    sideLength[2] = 0;
    sideLength[3] = 0;

    size_t curID = 0;

    for(int currentSide = 0; currentSide <= 3; ++currentSide) {

        const bool useWidth = (currentSide == SideID::TOP || currentSide == SideID::BOTTOM) ? true : false;
        const std::vector<unsigned __int32> sideIDs = bulb->getSideIDsVec(currentSide);
        int currentFrame = 0;
        do {

            BulbT bulbType;
            bulbType.bulbInfo = &bulbInfoVec[sideIDs[curID]];
            const unsigned __int16 bulbLength = useWidth ? bulbType.bulbInfo->width : bulbType.bulbInfo->height;

            // Will this bulb even fit?
            if(sideLength[currentSide] + bulbLength > maxSideLength[currentSide]) {
                break;
            }

            sideBulbs[currentSide].push_back(bulbType);

            sideLength[currentSide] += bulbLength;
            curID++;

            if(curID >= sideIDs.size() || sideIDs[curID] == 0xFFFFFFFF) {
                curID = 0;
            }

        } while (true);

    }

    if(blinkMode == BlinkMode::ALTERNATING) {
        for(int currentSide = 0; currentSide <= 3; ++currentSide) {

            if(sideBulbs[currentSide].empty()) {
                continue;
            }

            const size_t numBulbs = sideBulbs[currentSide].size();
            
            __int8 currentFrame = 0;

            for(size_t curBulb = 0; curBulb < numBulbs; ++curBulb) {

                sideBulbs[currentSide][curBulb].currentFrame = currentFrame;
                currentFrame++;

                // TODO: This only works if all the bulbs are the same, so
                // when it's time, figure that out too.
                if (sideBulbs[currentSide][curBulb].bulbInfo->frames.size() <= currentFrame) {
                    currentFrame = 0;
                }

            }
        }
    }

    BulbT bulbType;
    bulbType.bulbInfo = &bulbInfoVec[cornerIDs[CornerID::TOP_LEFT]];
    cornerBulbs.push_back(bulbType);
    bulbType.bulbInfo = &bulbInfoVec[cornerIDs[CornerID::TOP_RIGHT]];
    cornerBulbs.push_back(bulbType);

    bulbType.bulbInfo = &bulbInfoVec[cornerIDs[CornerID::BOTTOM_RIGHT]];
    cornerBulbs.push_back(bulbType);
    bulbType.bulbInfo = &bulbInfoVec[cornerIDs[CornerID::BOTTOM_LEFT]];
    cornerBulbs.push_back(bulbType);
    
    return true;
}

//-----------------------------------------------------------------------------
// OnPaint - Draw the bulbs to the monitor
//-----------------------------------------------------------------------------

bool MainWindow::OnPaint() {

    //if(sideBulbs[0][0].bulbInfo == NULL || sideBulbs[0][0].bulbInfo->frames.empty()) {
    //    return true;
    //}

    /*
    if(gifFrames.empty()) {
        return true;
    }
    */
  
    dxInfo->dc->BeginDraw();
    dxInfo->dc->Clear();

    RECT rc;
    GetClientRect(window, &rc);
    
    for(int currentSide = 0; currentSide <= 3; ++currentSide) {

        if(sideBulbs[currentSide].empty()) {
            continue;
        }

        FLOAT posOffset = (maxSideLength[currentSide] - sideLength[currentSide]) / 2;
        D2D1_RECT_F dest ={ 0, 0, 0, 0 };
        
        // Offset the X/Y position by the width/height of the corner bulb, if it exists.
 
        switch (currentSide) {

            case SideID::TOP:
                dest.top = 0;
                dest.left = posOffset;

                if(cornerBulbs[CornerID::TOP_LEFT].bulbInfo)  {
                    dest.left += cornerBulbs[CornerID::TOP_LEFT].bulbInfo->width;
                }

                break;

            case SideID::BOTTOM:
                dest.top = (rc.bottom - rc.top) - sideBulbs[currentSide][0].bulbInfo->height;
                dest.left = posOffset;

                if(cornerBulbs[CornerID::BOTTOM_LEFT].bulbInfo)  {
                    dest.left += cornerBulbs[CornerID::BOTTOM_LEFT].bulbInfo->width;
                }
                break;

            case SideID::LEFT:
                dest.top = posOffset;
                dest.left = 0;

                if(cornerBulbs[CornerID::TOP_LEFT].bulbInfo) {
                    dest.top += cornerBulbs[CornerID::TOP_LEFT].bulbInfo->height;
                }
                break;

            case SideID::RIGHT:
                dest.top = posOffset;
                dest.left = (rc.right - rc.left) - sideBulbs[currentSide][0].bulbInfo->width;
                if(cornerBulbs[CornerID::TOP_RIGHT].bulbInfo) {
                    dest.top += cornerBulbs[CornerID::TOP_RIGHT].bulbInfo->height;
                }
                break;

        }

        dest.right = dest.left + sideBulbs[currentSide][0].bulbInfo->width;
        dest.bottom = dest.top + sideBulbs[currentSide][0].bulbInfo->height;     
        
        const size_t numBulbs = sideBulbs[currentSide].size();

        for(int curBulb = 0; curBulb < numBulbs; ++curBulb) {

            const BulbInfo* bi = sideBulbs[currentSide][curBulb].bulbInfo;
            const BulbInfo* nextBulb = (curBulb + 1 < numBulbs) ? sideBulbs[currentSide][curBulb + 1].bulbInfo : NULL;
            const unsigned __int8 currentFrame = sideBulbs[currentSide][curBulb].currentFrame;
           
            dxInfo->dc->DrawBitmap(bi->frames[currentFrame], &dest);
            
            if(nextBulb) {
                if(currentSide == SideID::TOP || currentSide == SideID::BOTTOM) {
                    dest.left = dest.right;
                    dest.right += nextBulb->width;
                }
                else {
                    dest.top = dest.bottom;
                    dest.bottom += nextBulb->height;
                }
            }

        }

    }

    // TODO: Figure out which corner bulbs have bulbs.

    

    // Top Left

    const BulbInfo* bi = cornerBulbs[CornerID::TOP_LEFT].bulbInfo;
    unsigned __int8 currentFrame = cornerBulbs[CornerID::TOP_LEFT].currentFrame;

    D2D1_RECT_F dest = { 0, 0, bi->width, bi->height };
    dxInfo->dc->DrawBitmap(bi->frames[currentFrame], &dest);
    
    // Top Right

    bi = cornerBulbs[CornerID::TOP_RIGHT].bulbInfo;
    currentFrame = cornerBulbs[CornerID::TOP_RIGHT].currentFrame;
    dest.left = (rc.right - rc.left) - bi->width;
    dest.right = dest.left + bi->width;
    dest.top = 0;
    dest.bottom = bi->height;
   
    dxInfo->dc->DrawBitmap(bi->frames[currentFrame], &dest);

    // Bottom Left

    bi = cornerBulbs[CornerID::BOTTOM_LEFT].bulbInfo;
    currentFrame = cornerBulbs[CornerID::BOTTOM_LEFT].currentFrame;

    dest.left = 0;
    dest.right = bi->width;
    dest.top = (rc.bottom - rc.top) - bi->height;
    dest.bottom = dest.top + bi->height;

    dxInfo->dc->DrawBitmap(bi->frames[currentFrame], &dest);

    // Bottom Right

    bi = cornerBulbs[CornerID::BOTTOM_RIGHT].bulbInfo;
    currentFrame = cornerBulbs[CornerID::BOTTOM_RIGHT].currentFrame;

    dest.left = (rc.right - rc.left) - bi->width;
    dest.right = dest.left + bi->width;
    dest.top = (rc.bottom - rc.top) - bi->height;
    dest.bottom = dest.top + bi->height;

    dxInfo->dc->DrawBitmap(bi->frames[currentFrame], &dest);
    
        
    HRESULT status = dxInfo->dc->EndDraw();
    IS_OK(status);

    status = dxInfo->swapChain->Present(1, 0);
    IS_OK(status);

    return true;
}

//-----------------------------------------------------------------------------
// initBulbs - Setup the bulb vectors for the screen.
//-----------------------------------------------------------------------------

void MainWindow::initBulbs() {

}

//-----------------------------------------------------------------------------
// updateBulbs - Make the bulbs blink
//-----------------------------------------------------------------------------

void MainWindow::updateBulbs() {

        for(size_t currentSide = 0; currentSide <= 3; ++currentSide) {

        const size_t numBulbs = sideBulbs[currentSide].size();

        for(size_t currentBulb = 0; currentBulb < numBulbs; ++currentBulb) {
            if(sideBulbs[currentSide][currentBulb].bulbInfo != NULL) {

                const size_t numFrames = sideBulbs[currentSide][currentBulb].bulbInfo->frames.size();
            
                if(blinkMode == BlinkMode::RANDOM && numFrames != 1) {

                    int next = rand() % 100;

                    if(rand() % 100 > 50) {
                        sideBulbs[currentSide][currentBulb].currentFrame++;
                    }

                }
                else {
                    sideBulbs[currentSide][currentBulb].currentFrame++;
                }

                if(sideBulbs[currentSide][currentBulb].currentFrame >= numFrames) {
                    sideBulbs[currentSide][currentBulb].currentFrame = 0;
                }

            }
        }
    }

    // TODO

    if(cornerBulbs.empty()) {
        return;
    }

    for(size_t currentCorner = 0; currentCorner <= 3; ++ currentCorner) {
        if(cornerBulbs[currentCorner].bulbInfo != NULL) {
            
            cornerBulbs[currentCorner].currentFrame++;

            if(cornerBulbs[currentCorner].currentFrame >= cornerBulbs[currentCorner].bulbInfo->frames.size()) {
                cornerBulbs[currentCorner].currentFrame = 0;
            }

        }
    }

}

//-----------------------------------------------------------------------------
// windowProc - Standard window procedure for a window
//-----------------------------------------------------------------------------

LRESULT MainWindow::windowProc(const UINT& msg, const WPARAM wParam, const LPARAM lParam) {
    switch(msg)
    {
        case WM_PAINT:
            OnPaint();
            // Fall Thru. Paint needs DefWindowProc or it won't work
        default:
            return DefWindowProc(window, msg, wParam, lParam);

        case WM_TIMER:
            updateBulbs();
            InvalidateRect(window, NULL, TRUE);
            SetTimer(window, 0, waitTime, NULL);
            break;

        case WM_CLOSE:
            DestroyWindow(window);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

    }
    return 0;
}

//-----------------------------------------------------------------------------
// WndProc - A static function of a window proc that allows us to keep data
// for the window encapsulated.
//-----------------------------------------------------------------------------

LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* self;

    if(msg == WM_NCCREATE) {
        // Store a copy of an instance of this window in the USERDATA section
        self = static_cast<MainWindow*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);

        SetLastError(0);

        if (!SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self))) {
            const DWORD ERR = GetLastError();
            if(ERR != 0) {
                return FALSE;
            }
        }
    }
    else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if(self) {
            return self->windowProc(msg, wParam, lParam);
        }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);

}