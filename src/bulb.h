#ifndef __BULB_H__
#define __BULB_H__

#include <vector>
#include <string>

#include <d2d1_2.h>
#include <wincodec.h>

struct GIFData {
    unsigned __int32 size;
    unsigned __int16 width;
    unsigned __int16 height;
    unsigned __int8* data;
    //D2D image;
    GIFData() : data(NULL), size(0), height(0), width(0) {}
};

// Decoded IDs and their individual gif frames

struct BulbInfo {
    unsigned __int16                width;
    unsigned __int16                height;
    std::vector<ID2D1Bitmap*>       frames;
    BulbInfo() : width(0), height(0) {
    }
};

enum CornerID {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_RIGHT,
    BOTTOM_LEFT,
};

enum SideID {
    TOP,
    RIGHT,
    LEFT,
    BOTTOM,
};

// This is the Bulb Data

class Bulb {

    public:
        ~Bulb();
        
        int loadBulb(const std::string& filePath);
        void initBitmaps(IWICImagingFactory* factory, IWICBitmapDecoder* decoder, ID2D1DeviceContext* dc);
        
        std::vector<unsigned __int32>               cornerIDVec;
        std::vector<unsigned __int32>               sideIDs[4];
        std::vector<GIFData>                        imageData;
        std::vector<BulbInfo>                       d2dData;
        std::vector<unsigned __int32>               validIDs;

        // TODO: Create Bulbs based on some kind of reference count, if a specific gif isn't in use
        // we'll unload the D2D Bitmap for it.
       
        const std::vector<BulbInfo>& getBulbInfoVec() const;
        const std::vector<unsigned __int32>& getCornerIDsVec() const;
        const std::vector<unsigned __int32>& getSideIDsVec(const SideID& side) const;

    private:
        void destroyBitmaps();
        
};

// And this (When it's written) will be for a collection of bulbs.

#endif // __BULB_H__