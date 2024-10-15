#include "main_window.h"

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
    
    WNDCLASSEX wc;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MainWindow::WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = MainWindow::className.c_str();
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

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

    if(window) {
        return true; // Already created.
    }

    window = CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST,
        MainWindow::className.c_str(),
        L"",
        WS_VISIBLE | WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
        NULL, NULL, hInstance, this);

    if(window == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
// doLoop - Standard run of the mill message loop
//-----------------------------------------------------------------------------

UINT MainWindow::doLoop() {
    
    SetTimer(window, 0, 250, NULL);

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
    loadGIF();
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

    // Just for testing
    RECT rc;
    GetClientRect(window, &rc);
    length[0] = mi.rcMonitor.right - mi.rcMonitor.left;
    length[1] = mi.rcMonitor.bottom - mi.rcMonitor.top;

    length[2] = length[0];
    length[3] = length[1];

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

void MainWindow::loadGIF() {

    bulb.loadBulb("D:\\dump\\12.bul");
    bulb.initBitmaps(wicFactory.Get(), gifDecoder.Get(), dxInfo->dc.Get());

    // TODO: Need a way to get the width/height, and the number of Sub-Bulbs each bulb has

    /*
    const BulbInfo* bInfo = bulb.getSideInfo(SideID::TOP, 0);
    
    size_t numBulbs = length[0] / bInfo->width;

    for(size_t i = 0; i < numBulbs - 8; ++i) {
        BulbT bulbType;
        const BulbInfo* info = bulb.getSideInfo(SideID::TOP, i);
        bulbType.bulbInfo = info;
        sideBulbs[0].push_back(bulbType);
    }
    */

    const std::vector<BulbInfo>* bulbInfoVec = bulb.getBulbInfoVec();
    const std::vector<unsigned __int32> sideIDs = bulb.getSideIDsVec(SideID::TOP);

    unsigned int curLen = length[0];
    int curID = 0;
    do {
        BulbT bulbType;
       
        bulbType.bulbInfo = &(*bulbInfoVec)[sideIDs[curID]];

        // Will this bulb even fit?
        if(curLen < bulbType.bulbInfo->width) {
            break;
        }

        sideBulbs[0].push_back(bulbType);

        curLen -= bulbType.bulbInfo->width;
        curID++;

        if(curID >= sideIDs.size() || sideIDs[curID] == 0xFFFFFFFF) {
            curID = 0;
        }

    } while (true);

    
    //bulbSideTest[0].bulbInfo = bulb.getSideInfo(SideID::TOP, 0);
    /*
    bulbSideTest[1].bulbInfo = bulb.getSideInfo(SideID::LEFT, 0);
    bulbSideTest[3].bulbInfo = bulb.getSideInfo(SideID::BOTTOM, 0);
    bulbSideTest[4].bulbInfo = bulb.getSideInfo(SideID::RIGHT, 0);

    bulbCornerTest[0].bulbInfo = bulb.getCornerInfo(CornerID::TOP_LEFT);
    bulbCornerTest[1].bulbInfo = bulb.getCornerInfo(CornerID::TOP_RIGHT);
    bulbCornerTest[2].bulbInfo = bulb.getCornerInfo(CornerID::BOTTOM_LEFT);
    bulbCornerTest[3].bulbInfo = bulb.getCornerInfo(CornerID::BOTTOM_RIGHT);

    length[0] = length[0] - (bulbCornerTest[0].bulbInfo->width + bulbCornerTest[1].bulbInfo->width);
    length[1] = length[1] - (bulbCornerTest[0].bulbInfo->height + bulbCornerTest[2].bulbInfo->height);
    length[2] = length[2] - (bulbCornerTest[2].bulbInfo->width + bulbCornerTest[3].bulbInfo->width);
    length[3] = length[3] - (bulbCornerTest[1].bulbInfo->height + bulbCornerTest[3].bulbInfo->height);
    */

    //bulbTest.bulbInfo = bulb.getSideInfo(SideID::TOP, 0);

    /*
    if(!dxInfo) {
        return; // Can't do anything if D2D hasn't been initalized
    }
    
    // TODO: Figureout why we can't use ComPtr with wicFrame without an error.
    IWICBitmapFrameDecode*          wicFrame = NULL;
    ComPtr<IWICFormatConverter>     wicConverter;

    // TODO: From Stream
    wicFactory->CreateDecoderFromFilename(L"D://Dump//yellow.gif", NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &gifDecoder);

    UINT numFrames;
    gifDecoder->GetFrameCount(&numFrames);
    gifFrames.reserve(numFrames);

    for(size_t i = 0; i < numFrames; ++i) {

        // Decode Frame, and convert it to a Direct2D Bitmap we can use to draw with.
        
        gifDecoder->GetFrame(i, &wicFrame);
        wicFactory->CreateFormatConverter(&wicConverter);
        wicConverter->Initialize(wicFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);

        // TODO: Support for GIFs that use the compose mode?
        ID2D1Bitmap* bitmap = NULL;

        dxInfo->dc->CreateBitmapFromWicBitmap(*(&wicConverter), NULL, &bitmap);
        gifFrames.push_back(bitmap);

        wicFrame->Release();
        wicFrame = NULL;
        wicConverter = nullptr;
    }
    */
    
}

//-----------------------------------------------------------------------------
// OnPaint - Draw the bulbs to the monitor
//-----------------------------------------------------------------------------

bool MainWindow::OnPaint() {

    if(sideBulbs[0][0].bulbInfo == NULL || sideBulbs[0][0].bulbInfo->frames.empty()) {
        return true;
    }

    /*
    if(gifFrames.empty()) {
        return true;
    }
    */
  
    dxInfo->dc->BeginDraw();
    dxInfo->dc->Clear();

    /*
    D2D1_RECT_F rc;
    D2D1_RECT_F rc2;
    rc.top = 0;
    rc.left = 0;
    rc.right = 32;
    rc.bottom = 32;

    rc2.top = 900 - 32;
    rc2.left = 0;
    rc2.right = 32;
    rc2.bottom = 900;

    const int numBulbs = 1600 / 32;

    
    D2D1_MATRIX_4X4_F vFlip {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 32.0f, 0.0f, 1.0f,
    };

    D2D1_MATRIX_4X4_F ROT90{
        0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        32.0f, 0.0f, 0.0f, 1.0f,
    };

    D2D1_MATRIX_4X4_F ROT270{
        0.0f, -1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 32.0f, 0.0f, 1.0f,
    };
    
    for(int i = 0; i < numBulbs; ++i) {
        dxInfo->dc->DrawBitmap(gifFrames[frameIndex], &rc, 1.0F, D2D1_INTERPOLATION_MODE_LINEAR, NULL, &vFlip);
        dxInfo->dc->DrawBitmap(gifFrames[frameIndex], &rc2);
        rc.left += 32;
        rc.right += 32;
        rc2.left += 32;
        rc2.right += 32;
    }
    
    const int numB2 = 900 / 32;

    rc.top = 0;
    rc.left = 0;
    rc.right = 32;
    rc.bottom = 32;

    rc2.top = 1600;
    rc2.left = 0;
    rc2.right = 32;
    rc2.bottom = 1600 - 32;

    for(int i = 0; i < numB2 - 1; ++i) {
        if(i != 0) {
            dxInfo->dc->DrawBitmap(gifFrames[frameIndex], &rc, 1.0F, D2D1_INTERPOLATION_MODE_LINEAR, NULL, &ROT90);
            dxInfo->dc->DrawBitmap(gifFrames[frameIndex], &rc2, 1.0F, D2D1_INTERPOLATION_MODE_LINEAR, NULL, &ROT270);
        }
        rc.left += 32;
        rc.right += 32;
        rc2.left -= 32;
        rc2.right -= 32;
    }
    */

    /*
    D2D1_RECT_F dest1 = { 32, 0, 64, 32 };
    D2D1_RECT_F dest2 = { 32, 868, 64, 900 };
    const size_t tbSize = topBulbs.size();

    for(size_t i = 0; i < tbSize; ++i) {
        dxInfo->dc->DrawBitmap(gifFrames[topBulbs[i].currentFrame], &dest1); // , 1.0F, D2D1_INTERPOLATION_MODE_LINEAR, NULL, &vFlip);
        dxInfo->dc->DrawBitmap(gifFrames[bottomBulbs[i].currentFrame], &dest2);
        dest1.left += 32;
        dest1.right += 32;
        dest2.left += 32;
        dest2.right += 32;
    }

    dest1 = { 0, 32, 32, 64 };
    dest2 = { 1568, 32, 1600, 64 };

    const size_t lrSize = leftBulbs.size();

    for(size_t i = 0; i < lrSize; ++i) {
        dxInfo->dc->DrawBitmap(gifFrames[leftBulbs[i].currentFrame], &dest1); // , 1.0F, D2D1_INTERPOLATION_MODE_LINEAR, NULL, &vFlip);
        dxInfo->dc->DrawBitmap(gifFrames[rightBulbs[i].currentFrame], &dest2);
        dest1.top += 32;
        dest1.bottom += 32;
        dest2.top += 32;
        dest2.bottom += 32;
    }
    */

    D2D1_RECT_F dest1 = { 0, 0, 0, sideBulbs[0][0].bulbInfo->height };
    dest1.right += dest1.left;
    const int numBulbs = sideBulbs[0].size();
    
    int k = 0;

    for(int i = 0; i < numBulbs; ++i) {

        dest1.right += sideBulbs[0][k].bulbInfo->width;
        dxInfo->dc->DrawBitmap(sideBulbs[0][k].bulbInfo->frames[sideBulbs[0][k].currentFrame], &dest1);
        dest1.left += sideBulbs[0][k].bulbInfo->width;
        

        k++;
        if (k >= sideBulbs[0].size()) {
            k = 0;
        }
    }
    
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
    /*
    const size_t numTBBulbs = (1600 - 64) / 32;
    const size_t numLRBulbs = (900 - 64) / 32;
    
    topBulbs.reserve(numTBBulbs);

    BulbType bt = { 0 };
    bt.frameCount = gifFrames.size();

    for(size_t i = 0; i < numTBBulbs + 0; ++i) {
        bt.currentFrame = (i % bt.frameCount);
        topBulbs.push_back(bt);
        bottomBulbs.push_back(bt);
    }

     for(size_t i = 0; i < numLRBulbs + 0; ++i) {
        bt.currentFrame = (i % bt.frameCount);
        leftBulbs.push_back(bt);
        rightBulbs.push_back(bt);
    }   
    */
}

//-----------------------------------------------------------------------------
// updateBulbs - Make the bulbs blink
//-----------------------------------------------------------------------------

void MainWindow::updateBulbs() {


    const size_t numBulbs = sideBulbs[0].size();

    for(int i = 0; i < numBulbs; ++i) {
        if(sideBulbs[0][i].bulbInfo != NULL) {
            if(sideBulbs[0][i].currentFrame == sideBulbs[0][i].bulbInfo->frames.size() - 1) {
                sideBulbs[0][i].currentFrame = 0;
            }
            else {
                sideBulbs[0][i].currentFrame++;
            }
        }
    }

    /*
    const size_t tbBulbs = topBulbs.size();
    const size_t lrBulbs = leftBulbs.size();

    for(size_t i = 0; i < tbBulbs; ++i) {
        topBulbs[i].currentFrame = ((topBulbs[i].currentFrame + 1) % topBulbs[i].frameCount);
        bottomBulbs[i].currentFrame = ((bottomBulbs[i].currentFrame + 1) % bottomBulbs[i].frameCount);
    }

    for (size_t i = 0; i < lrBulbs; ++i) {
        leftBulbs[i].currentFrame = ((leftBulbs[i].currentFrame + 1) % leftBulbs[i].frameCount);
        rightBulbs[i].currentFrame = ((rightBulbs[i].currentFrame + 1) % rightBulbs[i].frameCount);
    }
    */
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
            SetTimer(window, 0, 100, NULL);
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