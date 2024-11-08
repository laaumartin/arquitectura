#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <map>
#include <tuple>
#include <unordered_map>
#include <cstdint>
#include <functional>

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
    vector<unsigned short> red;
    vector<unsigned short> green;
    vector<unsigned short> blue;
};
namespace std {
    template <>
    struct hash<std::tuple<unsigned short, unsigned short, unsigned short>> {
        std::size_t operator()(const std::tuple<unsigned short, unsigned short, unsigned short>& t) const {
            return std::hash<unsigned short>{}(std::get<0>(t)) ^
                   (std::hash<unsigned short>{}(std::get<1>(t)) << 1) ^
                   (std::hash<unsigned short>{}(std::get<2>(t)) << 2);
        }
    };
}
// Implementación de readLittleEndian para leer en little-endian
uint32_t readLittleEndian(ifstream &ifs, int byteCount) {
    uint32_t result = 0;
    for (int i = 0; i < byteCount; ++i) {
        uint8_t byte;
        ifs.read(reinterpret_cast<char*>(&byte), 1);
        result |= (byte << (8 * i));  // Coloca cada byte en su posición en formato little-endian
    }
    return result;
}


// Función para leer un archivo PPM
bool filePPM(const string &file , SOA &image) {
    ifstream ifs(file.c_str(), ios::binary);
    if (!ifs.is_open()) {
        cerr << "No es posible abrir el archivo " << file << '\n';
        return false;
    }

    string magicnb;
    ifs >> magicnb;
    if (magicnb != "P6") {
        cerr << "El formato del archivo no es correcto (debería ser P6)" << '\n';
        return false;
    }
    ifs >> image.width >> image.height >> image.maxval;
    if (image.maxval <= 0 || image.maxval > 65535) {
        cerr << "El valor máximo de color no está en el rango (1-65535)" << '\n';
        return false;
    }

    ifs.ignore();  // Ignorar el salto de línea después del encabezado

    int pixelCount = image.width * image.height;
    image.red.resize(pixelCount);
    image.green.resize(pixelCount);
    image.blue.resize(pixelCount);
    if (image.maxval <= 255) {
        // Leer 1 byte por componente de color
        for (int i = 0; i < pixelCount; ++i) {
            unsigned char r, g, b;
            ifs.read(reinterpret_cast<char*>(&r), 1);
            ifs.read(reinterpret_cast<char*>(&g), 1);
            ifs.read(reinterpret_cast<char*>(&b), 1);
            image.red[i] = r;
            image.green[i] = g;
            image.blue[i] = b;
        }
    } else {
        // Leer 2 bytes por componente de color en formato little-endian
        for (int i = 0; i < pixelCount; ++i) {
            image.red[i] = readLittleEndian(ifs, 2);
            image.green[i] = readLittleEndian(ifs, 2);
            image.blue[i] = readLittleEndian(ifs, 2);
        }
    }
    return true;
}


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
        auto scaleAndClamp = [&](unsigned short color) {
            float scaledValue = round(color * static_cast<float>(newMaxIntensity) / oldMaxIntensity);
            return static_cast<unsigned short>(min(max(0.0f, scaledValue), static_cast<float>(newMaxIntensity)));
        };
        image.red[i] = scaleAndClamp(image.red[i]);
        image.green[i] = scaleAndClamp(image.green[i]);
        image.blue[i] = scaleAndClamp(image.blue[i]);
    }
}

unsigned short interpolatePixel(const vector<unsigned short> &color, int xl, int yl, int xh, int yh, float x, float y, int width) {
    float c1 = color[yl * width + xl] * (xh - x) + color[yl * width + xh] * (x - xl);
    float c2 = color[yh * width + xl] * (xh - x) + color[yh * width + xh] * (x - xl);
    return static_cast<unsigned short>(c1 * (yh - y) + c2 * (y - yl));
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
vector<int> calculateColorFrequencies(const SOA &image) {
    map<tuple<int, int, int>, int> colorFrequency;
    vector<int> freq(image.red.size(), 0);

    for (size_t i = 0; i < image.red.size(); ++i) {
        tuple<int, int, int> color = make_tuple(image.red[i], image.green[i], image.blue[i]);
        colorFrequency[color]++;
    }

    for (size_t i = 0; i < image.red.size(); ++i) {
        tuple<int, int, int> color = make_tuple(image.red[i], image.green[i], image.blue[i]);
        freq[i] = colorFrequency[color];
    }
    return freq;
}

// Find the closest color by Euclidean distance
int findClosestColor(const SOA &image, int idx, const vector<int> &excludedIndices) {
    double minDist = numeric_limits<double>::max();
    int closestIdx = -1;
    for (int j = 0; j < image.width * image.height; ++j) {
        if (find(excludedIndices.begin(), excludedIndices.end(), j) != excludedIndices.end()) continue;

        double dist = sqrt(pow(image.red[idx] - image.red[j], 2) +
                           pow(image.green[idx] - image.green[j], 2) +
                           pow(image.blue[idx] - image.blue[j], 2));

        if (dist < minDist) {
            minDist = dist;
            closestIdx = j;
        }
    }
    return closestIdx;
}

// Remove least frequent colors and replace with the closest color by Euclidean distance
void removeLeastFrequentColors(SOA &image, const int n) {
    vector<int> freq = calculateColorFrequencies(image);
    vector<int> indices(image.red.size());
    iota(indices.begin(), indices.end(), 0);

    // Sort indices by frequency in ascending order
    sort(indices.begin(), indices.end(), [&](int a, int b) {
        return freq[a] < freq[b];
    });

    // Get the indices of the least frequent colors
    vector<int> colorsToRemove(indices.begin(), indices.begin() + n);

    // Replace each least frequent color with the closest color by distance
    for (int idx : colorsToRemove) {
        int closestIdx = findClosestColor(image, idx, colorsToRemove);
        if (closestIdx != -1) { // Ensure a valid closest color is found
            image.red[idx] = image.red[closestIdx];
            image.green[idx] = image.green[closestIdx];
            image.blue[idx] = image.blue[closestIdx];
        }
    }
}
void writeLittleEndian(std::ofstream &ofs, unsigned short value, int byteCount) {
    for (int i = 0; i < byteCount; ++i) {
        unsigned char byte = value & 0xFF;  // Extract the least significant byte
        ofs.write(reinterpret_cast<const char*>(&byte), 1);
        value >>= 8;  // Shift right 8 bits for the next byte
    }
}

void writePixelIndices(std::ofstream &ofs, const std::vector<int> &pixelIndices, int colorTableSize) {
    for (int colorIndex : pixelIndices) {
        if (colorTableSize <= MAX_BYTE_VALUE + 1) {
            unsigned char index = static_cast<unsigned char>(colorIndex);
            ofs.write(reinterpret_cast<const char*>(&index), 1);
        } else if (colorTableSize <= MAX_COLOR_VALUE) {
            writeLittleEndian(ofs, static_cast<unsigned short>(colorIndex), 2);
        } else if (colorTableSize <= 4294967296) {
            writeLittleEndian(ofs, static_cast<unsigned int>(colorIndex), 4);
        }
    }
}

void writeColorTable(std::ofstream &ofs, const SOA &colorTable, int bytesPerColor) {
    for (int i = 0; i < colorTable.red.size(); ++i) {
        if (bytesPerColor == 1) {
            unsigned char r = static_cast<unsigned char>(colorTable.red[i]);
            unsigned char g = static_cast<unsigned char>(colorTable.green[i]);
            unsigned char b = static_cast<unsigned char>(colorTable.blue[i]);
            ofs.write(reinterpret_cast<const char*>(&r), 1);
            ofs.write(reinterpret_cast<const char*>(&g), 1);
            ofs.write(reinterpret_cast<const char*>(&b), 1);
        } else {
            // Write in little-endian format
            writeLittleEndian(ofs, colorTable.red[i], 2);
            writeLittleEndian(ofs, colorTable.green[i], 2);
            writeLittleEndian(ofs, colorTable.blue[i], 2);
        }
    }
}

void compressionCPPM(const std::string &outputFile, SOA &image) {
    std::ofstream ofs(outputFile, std::ios::binary);
    if (!ofs || image.maxval <= 0 || image.maxval >= MAX_COLOR_VALUE) {
        std::cerr << "Error: Could not open file or maxval out of range" << std::endl;
        return;
    }

    SOA colorTable;
    colorTable.width = image.width;
    colorTable.height = image.height;
    colorTable.maxval = image.maxval;
    std::vector<int> pixelIndices;
    int totalPixels = image.width * image.height;
    std::unordered_map<std::tuple<unsigned short, unsigned short, unsigned short>, int> colorMap;
    for (int i = 0; i < totalPixels; i++) {
        auto color = std::make_tuple(image.red[i], image.green[i], image.blue[i]);

        // Buscar el color en el mapa
        auto it = colorMap.find(color);
        if (it != colorMap.end()) {
            // Si el color ya existe, añade el índice correspondiente
            pixelIndices.push_back(it->second);
        } else {
            // Si el color es nuevo, agrégalo a la tabla de colores y al mapa
            int newIndex = colorTable.red.size();
            colorTable.red.push_back(image.red[i]);
            colorTable.green.push_back(image.green[i]);
            colorTable.blue.push_back(image.blue[i]);
            colorMap[color] = newIndex;
            pixelIndices.push_back(newIndex);
        }
    }

    ofs << "C6 " << image.width << " " << image.height << " " << image.maxval << " " << colorTable.red.size() << "\n";
    int bytesPerColor = (image.maxval <= MAX_BYTE_VALUE) ? 1 : 2;
    writeColorTable(ofs, colorTable, bytesPerColor);
    writePixelIndices(ofs, pixelIndices, colorTable.red.size());
    ofs.close();
}
bool decompressCPPM(const std::string &inputFile, const std::string &outputFile) {
    std::ifstream ifs(inputFile, std::ios::binary);
    if (!ifs) {
        std::cerr << "Error: No se pudo abrir el archivo de entrada " << inputFile << std::endl;
        return false;
    }

    std::string format;
    int width, height, maxval, colorTableSize;
    ifs >> format >> width >> height >> maxval >> colorTableSize;

    if (format != "C6") {
        std::cerr << "Error: Formato de archivo incorrecto. Se esperaba 'C6'" << std::endl;
        return false;
    }

    ifs.ignore();  // Ignorar salto de línea después del encabezado

    SOA colorTable;
    colorTable.width = width;
    colorTable.height = height;
    colorTable.maxval = maxval;
    colorTable.red.resize(colorTableSize);
    colorTable.green.resize(colorTableSize);
    colorTable.blue.resize(colorTableSize);

    // Leer la tabla de colores
    int bytesPerColor = (maxval <= 255) ? 1 : 2;
    for (int i = 0; i < colorTableSize; ++i) {
        if (bytesPerColor == 1) {
            unsigned char r, g, b;
            ifs.read(reinterpret_cast<char*>(&r), 1);
            ifs.read(reinterpret_cast<char*>(&g), 1);
            ifs.read(reinterpret_cast<char*>(&b), 1);
            colorTable.red[i] = r;
            colorTable.green[i] = g;
            colorTable.blue[i] = b;
        } else {
            unsigned short r, g, b;
            ifs.read(reinterpret_cast<char*>(&r), 2);
            ifs.read(reinterpret_cast<char*>(&g), 2);
            ifs.read(reinterpret_cast<char*>(&b), 2);
            colorTable.red[i] = r;
            colorTable.green[i] = g;
            colorTable.blue[i] = b;
        }
    }

    // Leer los índices de píxeles
    std::vector<int> pixelIndices(width * height);
    int bitsPerIndex = static_cast<int>(ceil(log2(colorTableSize)));
    int indicesPerByte = 8 / bitsPerIndex;
    int bitPos = 0;
    unsigned char buffer;
    ifs.read(reinterpret_cast<char*>(&buffer), 1);

    for (int &index : pixelIndices) {
        index = (buffer >> bitPos) & ((1 << bitsPerIndex) - 1);
        bitPos += bitsPerIndex;
        if (bitPos >= 8) {
            bitPos -= 8;
            ifs.read(reinterpret_cast<char*>(&buffer), 1);
            index |= (buffer & ((1 << bitPos) - 1)) << (bitsPerIndex - bitPos);
        }
    }

    ifs.close();

    // Guardar la imagen descomprimida en formato PPM
    std::ofstream ofs(outputFile, std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n" << maxval << "\n";
    for (int index : pixelIndices) {
        ofs.put(static_cast<unsigned char>(colorTable.red[index]));
        ofs.put(static_cast<unsigned char>(colorTable.green[index]));
        ofs.put(static_cast<unsigned char>(colorTable.blue[index]));
    }
    ofs.close();
    return true;
}