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

enum class CornerID {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_RIGHT,
    BOTTOM_LEFT,
};

enum class SideID {
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
        
        unsigned __int32                            cornerIDs[4];
        std::vector<unsigned __int32>               sideIDs[4];
        std::vector<GIFData>                        imageData;
        std::vector<BulbInfo>                       d2dData;
        std::vector<unsigned __int32>               validIDs;
        // TODO: Create Bulbs based on some kind of reference count, if a specific gif isn't in use
        // we'll unload the D2D Bitmap for it.

        const BulbInfo* getSideInfo(const SideID& side, int index) {

            if(index >= sideIDs[static_cast<int>(side)].size()) {
                index = 0;
            }

            int ID = sideIDs[static_cast<int>(side)][index];

            if(ID == 0xFFFFFFFF) {
                ID = sideIDs[static_cast<int>(side)][0];
            }

            return &(d2dData[ID]);
        }

        const BulbInfo* getCornerInfo(const CornerID& corner) {
            int ID = cornerIDs[static_cast<int>(corner)];
            return &(d2dData[ID]);
        }

        const BulbInfo* getInfo(const int& ID) {
            return &(d2dData[ID]);
        }

    private:
        void destroyBitmaps();
        
};

// And this (When it's written) will be for a collection of bulbs.

#endif // __BULB_H__