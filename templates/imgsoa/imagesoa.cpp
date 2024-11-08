#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>

using namespace std;

// Constants to replace magic numbers
const int MAX_COLOR_VALUE = 65536;
const int MAX_BYTE_VALUE = 255;
const int COLOR_COMPONENTS = 3;
const float EPSILON = 1e-5;

struct SOA {
    int width;
    int height;
    int maxval;
    vector<unsigned char> red;
    vector<unsigned char> green;
    vector<unsigned char> blue;
};

// Utility function to resize image color vectors
void resizeImageVectors(SOA &image, int pixelCount, int bytes) {
    image.red.resize(bytes * pixelCount);
    image.green.resize(bytes * pixelCount);
    image.blue.resize(bytes * pixelCount);
}

// Utility function to read pixels from file
void readImagePixels(ifstream &ifs, SOA &image, int pixelCount, int bytes) {
    for (int i = 0; i < pixelCount; i++) {
        // Added braces around each single-line if statement
        ifs.read(reinterpret_cast<char*>(&image.red[i * bytes]), bytes);
        ifs.read(reinterpret_cast<char*>(&image.green[i * bytes]), bytes);
        ifs.read(reinterpret_cast<char*>(&image.blue[i * bytes]), bytes);
    }
}

// Function 2.1: Load PMM File with exception handling
bool filePMM(const string &file, SOA &image) {
    ifstream ifs(file.c_str(), ios::binary);
    if (!ifs.is_open()) {
        throw runtime_error("Not possible to open the file " + file);
    }
    string magicnb;
    ifs >> magicnb;
    if (magicnb != "P6") {
        throw runtime_error("The PMM format is not the correct one");
    }

    ifs >> image.width >> image.height >> image.maxval;
    if (image.maxval <= 0 || image.maxval >= MAX_COLOR_VALUE) {
        throw runtime_error("The maximum color value is out of range");
    }
    ifs.ignore();

    int pixelCount = image.width * image.height;
    int bytes = (image.maxval > MAX_BYTE_VALUE) ? 2 : 1;
    resizeImageVectors(image, pixelCount, bytes);
    readImagePixels(ifs, image, pixelCount, bytes);

    return true;
}

// Function 2.2: Scale Intensity with clamping (avoiding narrowing conversions)
void scaleIntensitySOA(SOA &image, const int oldMaxIntensity, const int newMaxIntensity) {
    const int pixelCount = static_cast<int>(image.red.size());
    for (int i = 0; i < pixelCount; ++i) {
        auto scaleAndClamp = [&](unsigned char color) {
            float scaledValue = round(color * static_cast<float>(newMaxIntensity) / oldMaxIntensity);
            return static_cast<unsigned char>(min(max(0.0f, scaledValue), static_cast<float>(newMaxIntensity)));
        };
        image.red[i] = scaleAndClamp(image.red[i]);
        image.green[i] = scaleAndClamp(image.green[i]);
        image.blue[i] = scaleAndClamp(image.blue[i]);
    }
}

// Helper function for bilinear interpolation in size scaling
unsigned char interpolatePixel(const vector<unsigned char> &color, int xl, int yl, int xh, int yh, float x, float y, int width) {
    float c1 = color[yl * width + xl] * (xh - x) + color[yl * width + xh] * (x - xl);
    float c2 = color[yh * width + xl] * (xh - x) + color[yh * width + xh] * (x - xl);
    return static_cast<unsigned char>(c1 * (yh - y) + c2 * (y - yl));
}

// Function 2.3: Size Scaling with bilinear interpolation
void sizescaling(const int newHeight, const int newWidth, const SOA &image, SOA &newImage) {
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
            newImage.red[newIndex] = interpolatePixel(image.red, xl, yl, xh, yh, x, y, image.width);
            newImage.green[newIndex] = interpolatePixel(image.green, xl, yl, xh, yh, x, y, image.width);
            newImage.blue[newIndex] = interpolatePixel(image.blue, xl, yl, xh, yh, x, y, image.width);
        }
    }
}

// Helper for color frequency calculation
vector<int> calculateColorFrequencies(const SOA &image, const int pixelCount) {
    vector<int> freq(pixelCount, 0);
    for (int i = 0; i < pixelCount; ++i) {
        for (int j = 0; j < pixelCount; ++j) {
            if (image.red[i] == image.red[j] && image.green[i] == image.green[j] && image.blue[i] == image.blue[j]) {
                freq[i]++;
            }
        }
    }
    return freq;
}

// Function to find closest color by distance
int findClosestColor(const SOA &image, int idx, const vector<int> &excludedIndices) {
    double minDist = numeric_limits<double>::max();
    int closestIdx = -1;
    for (int j = 0; j < image.width * image.height; ++j) {
        if (find(excludedIndices.begin(), excludedIndices.end(), j) != excludedIndices.end()) continue;
        double dist = sqrt(pow(image.red[idx] - image.red[j], 2) + pow(image.green[idx] - image.green[j], 2) + pow(image.blue[idx] - image.blue[j], 2));
        if (dist < minDist) {
            minDist = dist;
            closestIdx = j;
        }
    }
    return closestIdx;
}

// Function 2.4: Remove Least Frequent Colors
void removeLeastFrequentColors(SOA &image, const int n) {
    const int pixelCount = image.width * image.height;
    vector<int> freq = calculateColorFrequencies(image, pixelCount);
    vector<int> indices(pixelCount);
    iota(indices.begin(), indices.end(), 0);

    sort(indices.begin(), indices.end(), [&](int a, int b) {
        return freq[a] < freq[b] || (freq[a] == freq[b] && tie(image.blue[a], image.green[a], image.red[a]) > tie(image.blue[b], image.green[b], image.red[b]));
    });

    vector<int> colorsToRemove(indices.begin(), indices.begin() + n);
    vector<int> replacements(pixelCount, -1);
    for (int idx : colorsToRemove) {
        replacements[idx] = findClosestColor(image, idx, colorsToRemove);
    }

    for (int i = 0; i < pixelCount; ++i) {
        if (replacements[i] != -1) {
            int replaceIdx = replacements[i];
            image.red[i] = image.red[replaceIdx];
            image.green[i] = image.green[replaceIdx];
            image.blue[i] = image.blue[replaceIdx];
        }
    }
}
void compressionCPPM(const string &outputFile, SOA &image) {
    ofstream ofs(outputFile, binary);
    if (!ofs || image.maxval <= 0 || image.maxval >= MAX_COLOR_VALUE) {
        cerr << "Error al abrir el archivo o valor máximo fuera de rango" << endl;
        return;
    }
    SOA colorTable;
    vector<int> pixelIndices;
    int totalPixels = image.width * image.height;
    for (int i=0; i < totalPixels; i++) {
        bool found = false;
        for (int j = 0; j < colorTable.red.size(); ++j) {
            if (image.red[i] == colorTable.red[j] && image.green[i] == colorTable.green[j] && image.blue[i] == colorTable.blue[j]) {
                pixelIndices.push_back(j);
                found = true;
                break;
            }
        }
        if (!found) {
            colorTable.red.push_back(image.red[i]);
            colorTable.green.push_back(image.green[i]);
            colorTable.blue.push_back(image.blue[i]);
            pixelIndices.push_back(colorTable.red.size() - 1);
        }
    }
    ofs << "C6 " << image.width << " " << image.height << " " << image.maxval << " " << colorTable.red.size() << "\n";
    writeColorTable(ofs, colorTable, (image.maxval <= MAX_BYTE_VALUE) ? 1 : 2);
    writePixelIndices(ofs, pixelIndices, colorTable.size());
    ofs.close();
}

// auxiliares para no pasarnos de 40 lineas por funcion 

void writeColorTable(ofstream &ofs, const vector<vector<unsigned char>> &colorTable, int bytesPerColor) {
    for (int i = 0; i < colorTable.red.size(); ++i) {
        if (bytesPerColor == 1) {
            ofs.write(reinterpret_cast<const char*>(&colorTable.red[i]), 1);
            ofs.write(reinterpret_cast<const char*>(&colorTable.green[i]), 1);
            ofs.write(reinterpret_cast<const char*>(&colorTable.blue[i]), 1);
        } else {
            writeLittleEndian(ofs, colorTable.red[i], 2);
            writeLittleEndian(ofs, colorTable.green[i], 2);
            writeLittleEndian(ofs, colorTable.blue[i], 2);
        }
    }
}

void writePixelIndices(ofstream &ofs, const vector<int> &pixelIndices, int colorTable.Size) {
    for (int colorIndex : pixelIndices) {
        if (colorTableSize <= MAX_BYTE_VALUE+1) {
            unsigned char index = static_cast<unsigned char>(colorIndex);
            ofs.write(reinterpret_cast<const char*>(&index), 1);
        } else if (colorTableSize <= MAX_COLOR_VALUE) {
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