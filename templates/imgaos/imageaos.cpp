#include <vector>
#include <string>
#include<stdexcept>
#include <fstream>
#include <iostream>
#include<cmath>
#include <algorithm> 
#include <tuple>
#include <unordered_map>
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

//function 3: Size Scaling with bilinear interpolation

void sizescaling(const int newHeight, const int newWidth, const AOS &image, AOS &newImage) {
    newImage.height = newHeight;
    newImage.width = newWidth;
    newImage.red.resize(newHeight * newWidth);
    newImage.green.resize(newHeight * newWidth);
    newImage.blue.resize(newHeight * newWidth);

    for (int ynew = 0; ynew < newHeight; ++ynew) {
        for (int xnew = 0; xnew < newWidth; ++xnew) {
            float x = xnew * (image.width - 1) / static_cast<float>(newWidth - 1);
            float y = ynew * (image.height - 1) / static_cast<float>(newHeight - 1);
            int xl = static_cast<int>(floor(x));
            int yl = static_cast<int>(floor(y));
            int xh = static_cast<int>(ceil(x));
            int yh = static_cast<int>(ceil(y));

            int newIndex = ynew * newWidth + xnew;
            newImage.pixels[newIndex].red = interpolatePixel(image, xl, yl, xh, yh, x, y, image.width);
            newImage.pixels[newIndex].green = interpolatePixel(image, xl, yl, xh, yh, x, y, image.width);
            newImage.pixels[newIndex].blue = interpolatePixel(image, xl, yl, xh, yh, x, y, image.width);
        }
    }
}

// Helper for color frequency calculation. ESTO LO HE COPIADO DE SAO PERO NO SE PARA QUE ES
vector<int> calculateColorFrequencies(const AOS &image, const int pixelCount) {
    vector<int> freq(pixelCount, 0);
    for (int i = 0; i < pixelCount; ++i) {
        for (int j = 0; j < pixelCount; ++j) {
            if (image.pixels[i].red == image.pixels[j].red && image.pixels[i].green == image.pixels[j].green && image.pixels[i].blue == image.pixels[j].blue) {
                freq[i]++;
            }
        }
    }
    return freq;
}

// Function to find closest color by distance
int findClosestColor(const AOS &image, int idx, const vector<int> &excludedIndices) {
    double minDist = numeric_limits<double>::max();
    int closestIdx = -1;
    for (int j = 0; j < image.width * image.height; ++j) {
        if (find(excludedIndices.begin(), excludedIndices.end(), j) != excludedIndices.end()) continue;
        double dist = sqrt(pow(image.pixels[idx].red - image.pixels[j].red, 2) + pow(image.pixels[idx].green - image.pixels[j].green, 2) + pow(image.pixels[idx].blue - image.pixels[j].blue, 2));
        if (dist < minDist) {
            minDist = dist;
            closestIdx = j;
        }
    }
    return closestIdx;
}

// Function 4: Remove Least Frequent Colors
void removeLeastFrequentColors(AOS &image, const int n) {
    const int pixelCount = image.width * image.height;
    vector<int> freq = calculateColorFrequencies(image, pixelCount);
    vector<int> indices(pixelCount);
    iota(indices.begin(), indices.end(), 0);

    sort(indices.begin(), indices.end(), [&](int a, int b) {
        return freq[a] < freq[b] || (freq[a] == freq[b] && tie(image.pixels[a].blue, image.pixels[a].green, image.pixels[a].red) > tie(image.pixels[b].blue, image.pixels[b].green, image.pixels[b].red));
    });

    vector<int> colorsToRemove(indices.begin(), indices.begin() + n);
    vector<int> replacements(pixelCount, -1);
    for (int idx : colorsToRemove) {
        replacements[idx] = findClosestColor(image, idx, colorsToRemove);
    }

    for (int i = 0; i < pixelCount; ++i) {
        if (replacements[i] != -1) {
            int replaceIdx = replacements[i];
            image.pixels[i].red = image.pixels[replaceIdx].red;
            image.pixels[i].green = image.pixels[replaceIdx].green;
            image.pixels[i].blue = image.pixels[replaceIdx].blue;
        }
    }
}

//Function 5

void compressionCPPM(const string &outputFile, AOS &image) {
    ofstream ofs(outputFile, ios::binary);
    if (!ofs || image.maxval <= 0 || image.maxval >= 65536) {
        cerr << "Error al abrir el archivo o valor máximo fuera de rango" << endl;
        return;
    }

    vector<Pixel> colorTable;
    vector<int> pixelIndices;
    for (const auto& pixel : image.pixels) {
        auto it = find(colorTable.begin(), colorTable.end(), pixel);
        if (it != colorTable.end()) {
            pixelIndices.push_back(distance(colorTable.begin(), it));
        } else {
            colorTable.push_back(pixel);
            pixelIndices.push_back(colorTable.size() - 1);
        }
    }
    ofs << "C6 " << image.width << " " << image.height << " " << image.maxval << " " << colorTable.size() << "\n";
    writeColorTable(ofs, colorTable, (image.maxval <= 255) ? 1 : 2);
    writePixelIndices(ofs, pixelIndices, colorTable.size());

    ofs.close();
}

// auxiliares para no pasarnos de 40 lineas por funcion 

void writeColorTable(ofstream &ofs, const vector<Pixel> &colorTable, int bytesPerColor) {
    for (const auto& color : colorTable) {
        if (bytesPerColor == 1) {
            ofs.write(reinterpret_cast<const char*>(&color.red), 1);
            ofs.write(reinterpret_cast<const char*>(&color.green), 1);
            ofs.write(reinterpret_cast<const char*>(&color.blue), 1);
        } else {
            writeLittleEndian(ofs, color.red, 2);
            writeLittleEndian(ofs, color.green, 2);
            writeLittleEndian(ofs, color.blue, 2);
        }
    }
}

void writePixelIndices(ofstream &ofs, const vector<int> &pixelIndices, int colorTableSize) {
    for (int colorIndex : pixelIndices) {
        if (colorTableSize <= 256) {
            unsigned char index = static_cast<unsigned char>(colorIndex);
            ofs.write(reinterpret_cast<const char*>(&index), 1);
        } else if (colorTableSize <= 65536) {
            writeLittleEndian(ofs, static_cast<unsigned short>(colorIndex), 2);
        } else if (colorTableSize <= 4294967296) {
            writeLittleEndian(ofs, static_cast<unsigned int>(colorIndex), 4);
        }
    }
}


void writeLittleEndian(ofstream &ofs, unsigned short value, int byteCount) {
    for (int i = 0; i < byteCount; ++i) {
        unsigned char byte = value & 0xFF;  // Extraer el byte menos significativo
        ofs.write(reinterpret_cast<const char*>(&byte), 1);
        value >>= 8;  // Desplazar a la derecha 8 bits para el siguiente byte
    }
}