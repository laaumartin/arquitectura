#include <vector>
#include <string>
#include<stdexcept>
#include <fstream>
#include <iostream>
#include<cmath>
#include <algorithm> 
using namespace std;

struct Pixel {
    unsigned char red;  //puede que  este tipo de variable nos de fallos ya que solo soporta 1 byte de info. habria que preguntar
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

    int bytesPerColor= (image.maxval<=255) ? 1: 2; //assignates 1 if maxval<255 and 2 otherwise
    for (int i = 0; i < pixelCount; ++i) {
        ifs.read(reinterpret_cast<char*>(&image.pixels[i].red), bytesPerColor);
        ifs.read(reinterpret_cast<char*>(&image.pixels[i].green), bytesPerColor);
        ifs.read(reinterpret_cast<char*>(&image.pixels[i].blue), bytesPerColor);
    }
    return true;
}

//function 2
void scaleIntensityAOS(SOA& image, int oldMaxIntensity, int newMaxIntensity) {
    int pixelCount = image.pixels.size(); //para saber el numero de pixels

    for (int i = 0; i < pixelCount; ++i) {
        //using the formula from the statement
        image.red[i] = static_cast<unsigned char>(round(image.pixels[i].red * newMaxIntensity / oldMaxIntensity));
        image.green[i] = static_cast<unsigned char>(round(image.pixels[i].green * newMaxIntensity / oldMaxIntensity));
        image.blue[i] = static_cast<unsigned char>(round(image.pixels[i].blue* newMaxIntensity / oldMaxIntensity));

        // esto es para corroborar que esta dentro del rango nuevo lo ha añadido chatgpt pero yo lo quitaria
        image.pixels[i].red = clamp(image.pixels[i].red, 0, newMaxIntensity);
        image.pixels[i].green = clamp(image.pixels[i].green, 0, newMaxIntensity);
        image.pixels[i].blue = clamp(image.pixels[i].blue, 0, newMaxIntensity);
    }
}
