#ifndef __BULB_H__
#define __BULB_H__

#include <vector>
#include <unordered_map>
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
    BOTTOM,
    LEFT,
};

// This is the Bulb Data

class Bulb {

    public:
        ~Bulb();
        
        int loadBulb(const std::string& filePath);
        void initBitmaps(IWICImagingFactory* factory, IWICBitmapDecoder* decoder, ID2D1DeviceContext* dc);

        bool isInit() {
            return !d2dData.empty();
        }
        
        std::vector<unsigned __int32>               cornerIDVec;
        std::vector<unsigned __int32>               sideIDs[4];
        std::vector<GIFData>                        imageData;
        std::vector<BulbInfo>                       d2dData;
        std::vector<unsigned __int32>               validIDs;
      
        const std::vector<BulbInfo>& getBulbInfoVec() const;
        const std::vector<unsigned __int32>& getCornerIDsVec() const;
        const std::vector<unsigned __int32>& getSideIDsVec(const int& side) const;

    private:
        void destroyBitmaps();
        
};

class BulbCollection {
    
    public:
        BulbCollection() {}
        ~BulbCollection() {

            for(auto it = loadedBulbs.begin(); it != loadedBulbs.end(); ++it) {
                if(it->second != NULL) {
                    delete (it->second);
                    it->second = NULL;
                }
            }

        }
        std::unordered_map<std::string, Bulb*> loadedBulbs;

        int loadBulb(const std::string& fileName) {

            const auto it = loadedBulbs.find(fileName);

            if(it != loadedBulbs.end()) {
                return 0; // File already loaded
            }

            // TODO: make sure fileName has a .bul extension.

            Bulb* newBulb = new Bulb();
            std::string filePath = "lights/" + fileName;
            const int retVal = newBulb->loadBulb(filePath);

            if(retVal == 0) {
                loadedBulbs[fileName] = newBulb;
            }
            else {
                delete newBulb;
                newBulb = NULL;
            }

            return 0;

        }

        const Bulb* getBulbByID(const std::string& ID, IWICImagingFactory* factory, IWICBitmapDecoder* decoder, ID2D1DeviceContext* dc) {
           
            auto it = loadedBulbs.find(ID);

            if(it == loadedBulbs.end()) {
                return NULL;
            }

            if(!it->second->isInit()) {
                it->second->initBitmaps(factory, decoder, dc);
            }

            return it->second;
        }

};

// And this (When it's written) will be for a collection of bulbs.

#endif // __BULB_H__