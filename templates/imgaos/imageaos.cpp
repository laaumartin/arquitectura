#include <vector>
#include <string>
#include<stdexcept>
#include <fstream>
#include <iostream>
#include<cmath>
#include <algorithm> 
using namespace std;

struct Pixel {
    unsigned char red;   // Use uint16_t to accommodate up to 65535 if needed. se puede cambiar por 'unsigned char'
    unsigned char green;
    unsigned char blue;
};

struct AOS {
    int width;
    int height;
    int maxval;
    vector<Pixel> pixels; // Vector que almacena cada píxel completo
};

//function 1
bool filePMM(const string &file , AOS &image) {
    ifstream ifs(file.c_str(), ios::binary);
    if (!ifs.is_open()) {
        cerr << "No es posible abrir el archivo " << file << endl;
        return false;
    }
    //check if the magic number identifying the file type is  P6
    string magicnb;
    ifs >> magicnb;
    if (magicnb.compare("P6") != 0) {
        cerr<<"The PMM format is not the correct one"<<endl;
        return false;
    }

    ifs >> image.width >> image.height >> image.maxval;
    if (image.maxval <= 0 || image.maxval >= 65536) {
        cerr<<"The maximum color value is not inside the range"<<endl;
        return false;
    }

    ifs.ignore(); // Ignore white spaces

    int pixelCount = image.width * image.height;
    image.pixels.resize(pixelCount); // Redimensionate the vector of pixels

    for (int i = 0; i < pixelCount; ++i) {
        ifs.read(reinterpret_cast<char*>(&image.pixels[i].red), 1);
        ifs.read(reinterpret_cast<char*>(&image.pixels[i].green), 1);
        ifs.read(reinterpret_cast<char*>(&image.pixels[i].blue), 1);
    }
    return true;
}

