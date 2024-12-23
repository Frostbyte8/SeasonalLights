#include "bulb.h"
#include <cassert>
static const char FILE_ID[] = "bluBrgiT\x04";
static const unsigned int ID_OFFSET = 360;
static const unsigned int LOOKUP_OFFSET = 508;

//=============================================================================
// Destructor
//=============================================================================

Bulb::~Bulb() {

    destroyBitmaps();

    for(size_t i = 0; i < imageData.size(); ++i) {
        if(imageData[i].data != NULL) {
            free(imageData[i].data);
            imageData[i].data = NULL;
        }
    }
}

//=============================================================================
// Accessors
//=============================================================================

///----------------------------------------------------------------------------
/// getBulbInfoVec - Get a constant reference to the vector that contains the
/// information about each of the sub bulbs within the Bulb file
///----------------------------------------------------------------------------

const std::vector<BulbInfo>& Bulb::getBulbInfoVec() const {
    return d2dData;
}

///----------------------------------------------------------------------------
/// getCornerIDsVec - Get a constant reference to the vector that contains the
/// Bulb IDs for each of the 4 corners.
///----------------------------------------------------------------------------

const std::vector<unsigned __int32>& Bulb::getCornerIDsVec() const {
    return cornerIDVec;
}

///----------------------------------------------------------------------------
/// getSideIDsVec - Get a constant reference to a vector that contains the
/// Bulb IDs for requested side
///----------------------------------------------------------------------------

const std::vector<unsigned __int32>& Bulb::getSideIDsVec(const int& side) const {
    assert(side >= SideID::TOP && side <= SideID::LEFT);
    return sideIDs[side];
}

//=============================================================================
// Functions
//=============================================================================

///----------------------------------------------------------------------------
/// initBitmaps - Takes the loaded gif Data, and converts them into D2DBitmaps.
/// If it is recalled, it destroys the bitmaps and recreates them. This is
/// useful if Direct2D needs to reinitalize itself.
///----------------------------------------------------------------------------

void Bulb::initBitmaps(IWICImagingFactory* factory, IWICBitmapDecoder* decoder, ID2D1DeviceContext* dc) {

    destroyBitmaps();

    IWICStream*             imageStream     = NULL;
    IWICFormatConverter*    formatConverter = NULL;
    ID2D1Bitmap*            d2dBMP          = NULL;

    // TOOD: Error Checking
    const size_t numGifs = imageData.size();

    for(size_t curGif = 0; curGif < numGifs; ++curGif) {

        HRESULT hr = factory->CreateStream(&imageStream);   
        BulbInfo bi;

        if(imageData[curGif].data != NULL) {

            // Copy the iamge data into a IWIC stream that we will then use to convert this into
            // a series of D2D Bitmaps
            hr = imageStream->InitializeFromMemory((BYTE *)imageData[curGif].data, imageData[curGif].size);
            hr = factory->CreateDecoderFromStream(imageStream, NULL, WICDecodeMetadataCacheOnDemand, &decoder);

            UINT numFrames;
            decoder->GetFrameCount(&numFrames);

            bi.width  = imageData[curGif].width;
            bi.height = imageData[curGif].height;

            // Extract individual frames
            // TODO: Sprite Sheets instead of using a bitmap for every gif frame

            for (size_t curFrame = 0; curFrame < numFrames; ++curFrame) {

                IWICBitmapFrameDecode*      decodedFrame = NULL;
                IWICMetadataQueryReader*    decodedMeta = NULL;
            
                decoder->GetFrame(static_cast<UINT>(curFrame), &decodedFrame);
                factory->CreateFormatConverter(&formatConverter);
                formatConverter->Initialize(decodedFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);

                ID2D1Bitmap* bitmap = NULL;

                UINT frameWidth;
                UINT frameHeight;

                decodedFrame->GetSize(&frameWidth, &frameHeight);

                // Check to see if this frame is smaller than the gif image itself, it may be "replace" style gif.
                if(curFrame != 0 && (frameWidth != bi.width || frameHeight != bi.height) ) {
                   
                    // Get the Left and Top positions of the frame, and then
                    // use the previous frame as a base for this frame, and replace
                    // the changed pixels.

                    decodedFrame->GetMetadataQueryReader(&decodedMeta);

                    PROPVARIANT frameX;
                    PROPVARIANT frameY;
                    PropVariantInit(&frameX);
                    PropVariantInit(&frameY);
                    
                    decodedMeta->GetMetadataByName(L"/imgdesc/Left", &frameX);
                    decodedMeta->GetMetadataByName(L"/imgdesc/Top", &frameY);

                    ID2D1Bitmap* prevFrame = bi.frames[curFrame-1];
                    D2D1_SIZE_U prevSize = { bi.width, bi.height };
                    FLOAT dpiX;
                    FLOAT dpiY;
                    prevFrame->GetDpi(&dpiX, &dpiY);
                    D2D1_BITMAP_PROPERTIES props = { prevFrame->GetPixelFormat(), dpiX, dpiY };
                    
                    dc->CreateBitmap(prevFrame->GetPixelSize(), props, &bitmap);

                    bitmap->CopyFromBitmap(NULL, prevFrame, NULL);

                    ID2D1Bitmap* tempBMP;
                    dc->CreateBitmapFromWicBitmap(*(&formatConverter), NULL, &tempBMP);

                    D2D1_POINT_2U dest = { frameX.uintVal , frameY.uintVal };
                    
                    bitmap->CopyFromBitmap(&dest, tempBMP, NULL);
                    tempBMP->Release();
                    tempBMP = NULL;

                    decodedMeta->Release();
                    decodedMeta = NULL;

                }
                else {
                    dc->CreateBitmapFromWicBitmap(*(&formatConverter), NULL, &bitmap);
                }
       
                bi.frames.push_back(bitmap);

                decodedFrame->Release();  
                decodedFrame = NULL;

                formatConverter->Release();
                formatConverter = NULL;

            }

            imageStream->Release();
            imageStream = NULL;

        }

        d2dData.push_back(bi);
    }

}

///----------------------------------------------------------------------------
/// destroyBitmaps - Release resources used by any bitmaps that were created
///----------------------------------------------------------------------------

void Bulb::destroyBitmaps() {

    for(size_t i = 0; i < d2dData.size(); ++i) {
        
        for(size_t k = 0; k < d2dData[i].frames.size(); ++k) {
            if(d2dData[i].frames[k] != NULL) {
                d2dData[i].frames[k]->Release();
                d2dData[i].frames[k] = NULL;
            }
        }

        d2dData[i].frames.clear();
    }

    d2dData.clear();
    
}

///----------------------------------------------------------------------------
/// loadBulb - Read a .bul file into memory.
///----------------------------------------------------------------------------

int Bulb::loadBulb(const std::string& filePath) {

    FILE* fp = NULL;
    fopen_s(&fp, &filePath[0], "rb");

    if(fp == NULL) {
        return -1;
    }

    char fileID[sizeof(FILE_ID)];
    fread(fileID, sizeof(char), sizeof(FILE_ID), fp);

    if(strcmp(fileID, FILE_ID)) {
        fclose(fp);
        return -1;
    }

    // TODO: Read all the attribution information for the GUI.

    fseek(fp, ID_OFFSET, SEEK_SET);

    // Collect IDs

    unsigned __int32 cornerIDs[4];

    fread(cornerIDs, sizeof(unsigned __int32), 4, fp);

    for(int i = 0; i < 4; i++) {
        cornerIDVec.push_back(cornerIDs[i]);
    }

    for(int i = 0; i < 4; ++i) {

        unsigned __int32 buf[8];
        fread(buf, sizeof(unsigned __int32), 8, fp);
        
        for(int k = 0; k < 8; ++k) {
            sideIDs[i].push_back(buf[k]);
        }

    }

    // Read Jump Table

    struct {
        unsigned __int32    size;
        unsigned __int32    offset;
        unsigned __int32    hash;
        unsigned __int32    numFrames;
        unsigned __int32    unknown;
    } lookupTable[37];
    
    fread(lookupTable, sizeof(lookupTable), 1, fp);

    // Read and Cache GIF Data

    for(int curFrames = 0; curFrames < 37; ++curFrames) {

        GIFData gifData;

        if(lookupTable[curFrames].offset != 0) {
            

            fseek(fp, lookupTable[curFrames].offset, SEEK_SET);
            
            gifData.size    = lookupTable[curFrames].size;
            gifData.data    = (unsigned __int8*)malloc(gifData.size);
            fread(&gifData.data[0], sizeof(unsigned __int8), gifData.size, fp);
            gifData.width   = *reinterpret_cast<unsigned __int16*>(&gifData.data[6]);
            gifData.height   = *reinterpret_cast<unsigned __int16*>(&gifData.data[8]);

            validIDs.push_back(curFrames);
            
        }

        imageData.push_back(gifData);
    }

    fclose(fp);

    if(validIDs.empty()) {
        // File is not a bulb or is corrupt.
        return -1;
    }

    return 0;
}