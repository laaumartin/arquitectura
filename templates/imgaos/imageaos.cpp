#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <tuple>
#include <algorithm> // for std::clamp
#include <unordered_map>
#include <cmath>
#include <numeric> // for std::iota
#include "imageaos.h"
#include <map>

using namespace std;


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

// Declaración de funciones
void skipComments(ifstream &ifs);
bool filePPM(const string &file , AOS &image);
void scaleIntensityAOS(AOS& image,int oldMaxIntensity, int newMaxIntensity);
void sizescaling(const int newWidth, const int newHeight, const AOS &image, AOS &newImage);
unsigned short interpolatePixel(const AOS &image, int xl, int yl, int xh, int yh, float x, float y, char colorComponent);
vector<int> calculateColorFrequencies(const AOS &image, const int pixelCount);
int findClosestColor(const AOS &image, int idx, const vector<int> &excludedIndices);
void removeLeastFrequentColors(AOS &image, const int n);
void compressionCPPM(const string &outputFile, AOS &image);
void writeColorTable(ofstream &ofs, const vector<Pixel> &colorTable, int bytesPerColor);
void writePixelIndices(ofstream &ofs, const vector<int> &pixelIndices, int colorTableSize);
void writeLittleEndian(ofstream &ofs, uint32_t value, int byteCount);

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
bool filePPM(const string &file , AOS &image) {
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
  image.pixels.resize(pixelCount);

  if (image.maxval <= 255) {
    // Leer 1 byte por componente de color
    for (int i = 0; i < pixelCount; ++i) {
      unsigned char r, g, b;
      ifs.read(reinterpret_cast<char*>(&r), 1);
      ifs.read(reinterpret_cast<char*>(&g), 1);
      ifs.read(reinterpret_cast<char*>(&b), 1);
      image.pixels[i].red = r;
      image.pixels[i].green = g;
      image.pixels[i].blue = b;
    }
  } else {
    // Leer 2 bytes por componente de color en formato little-endian
    for (int i = 0; i < pixelCount; ++i) {
      image.pixels[i].red = readLittleEndian(ifs, 2);
      image.pixels[i].green = readLittleEndian(ifs, 2);
      image.pixels[i].blue = readLittleEndian(ifs, 2);
    }
  }

  return true;
}
// Escalar la intensidad
void scaleIntensityAOS(AOS& image, int oldMaxIntensity, int newMaxIntensity) {
  int pixelCount = image.pixels.size();

  for (int i = 0; i < pixelCount; ++i) {
    image.pixels[i].red = static_cast<unsigned char>(round(image.pixels[i].red * newMaxIntensity / oldMaxIntensity));
    image.pixels[i].green = static_cast<unsigned char>(round(image.pixels[i].green * newMaxIntensity / oldMaxIntensity));
    image.pixels[i].blue = static_cast<unsigned char>(round(image.pixels[i].blue * newMaxIntensity / oldMaxIntensity));

   	image.pixels[i].red = clamp(image.pixels[i].red, static_cast<unsigned short>(0), static_cast<unsigned short>(newMaxIntensity));
    image.pixels[i].green = clamp(image.pixels[i].green, static_cast<unsigned short>(0), static_cast<unsigned short>(newMaxIntensity));
    image.pixels[i].blue = clamp(image.pixels[i].blue, static_cast<unsigned short>(0), static_cast<unsigned short>(newMaxIntensity));

  }
}

// Escalar el tamaño con interpolación bilineal
void sizescaling(const int newHeight, const int newWidth, const AOS &image, AOS &newImage) {
  newImage.height = newHeight;
  newImage.width = newWidth;
  newImage.pixels.resize(newHeight * newWidth);

  for (int ynew = 0; ynew < newHeight; ++ynew) {
    for (int xnew = 0; xnew < newWidth; ++xnew) {
      float x = xnew * (image.width - 1) / static_cast<float>(newWidth - 1);
      float y = ynew * (image.height - 1) / static_cast<float>(newHeight - 1);
      int xl = static_cast<int>(floor(x));
      int yl = static_cast<int>(floor(y));
      int xh = static_cast<int>(ceil(x));
      int yh = static_cast<int>(ceil(y));

      int newIndex = ynew * newWidth + xnew;
      newImage.pixels[newIndex].red = interpolatePixel(image, xl, yl, xh, yh, x, y, 'r');
      newImage.pixels[newIndex].green = interpolatePixel(image, xl, yl, xh, yh, x, y, 'g');
      newImage.pixels[newIndex].blue = interpolatePixel(image, xl, yl, xh, yh, x, y, 'b');
    }
  }
}

// Interpolación para el escalado
unsigned short interpolatePixel(const AOS &image, int xl, int yl, int xh, int yh, float x, float y, char colorComponent) {
  int idx_ll = yl * image.width + xl;
  int idx_hl = yl * image.width + xh;
  int idx_lh = yh * image.width + xl;
  int idx_hh = yh * image.width + xh;

  unsigned short c_ll, c_hl, c_lh, c_hh;

  if (colorComponent == 'r') {
    c_ll = image.pixels[idx_ll].red;
    c_hl = image.pixels[idx_hl].red;
    c_lh = image.pixels[idx_lh].red;
    c_hh = image.pixels[idx_hh].red;
  } else if (colorComponent == 'g') {
    c_ll = image.pixels[idx_ll].green;
    c_hl = image.pixels[idx_hl].green;
    c_lh = image.pixels[idx_lh].green;
    c_hh = image.pixels[idx_hh].green;
  } else {
    c_ll = image.pixels[idx_ll].blue;
    c_hl = image.pixels[idx_hl].blue;
    c_lh = image.pixels[idx_lh].blue;
    c_hh = image.pixels[idx_hh].blue;
  }

  float c1 = c_ll + (c_hl - c_ll) * (x - xl);
  float c2 = c_lh + (c_hh - c_lh) * (x - xl);
  float interpolatedColor = c1 + (c2 - c1) * (y - yl);

  return static_cast<unsigned short>(round(interpolatedColor));
}
// Función auxiliar para calcular la frecuencia de cada color en AOS
vector<int> calculateColorFrequencies(const AOS &image) {
    map<tuple<int, int, int>, int> colorFrequency;
    vector<int> freq(image.pixels.size(), 0);

    for (const auto& pixel : image.pixels) {
        tuple<int, int, int> color = make_tuple(pixel.red, pixel.green, pixel.blue);
        colorFrequency[color]++;
    }

    for (size_t i = 0; i < image.pixels.size(); ++i) {
        tuple<int, int, int> color = make_tuple(image.pixels[i].red, image.pixels[i].green, image.pixels[i].blue);
        freq[i] = colorFrequency[color];
    }
    return freq;
}

// Encontrar el color más cercano en base a la distancia euclidiana
int findClosestColor(const AOS &image, int idx, const vector<int> &excludedIndices) {
    double minDist = numeric_limits<double>::max();
    int closestIdx = -1;

    for (int j = 0; j < image.pixels.size(); ++j) {
        if (find(excludedIndices.begin(), excludedIndices.end(), j) != excludedIndices.end()) continue;

        double dist = sqrt(pow(image.pixels[idx].red - image.pixels[j].red, 2) +
                           pow(image.pixels[idx].green - image.pixels[j].green, 2) +
                           pow(image.pixels[idx].blue - image.pixels[j].blue, 2));

        if (dist < minDist) {
            minDist = dist;
            closestIdx = j;
        }
    }
    return closestIdx;
}

// Eliminar los colores menos frecuentes y reemplazarlos con el color más cercano
void removeLeastFrequentColors(AOS &image, const int n) {
    vector<int> freq = calculateColorFrequencies(image);
    vector<int> indices(image.pixels.size());
    iota(indices.begin(), indices.end(), 0);

    // Ordenar los índices por frecuencia en orden ascendente
    sort(indices.begin(), indices.end(), [&](int a, int b) {
        return freq[a] < freq[b];
    });

    // Obtener los índices de los colores menos frecuentes
    vector<int> colorsToRemove(indices.begin(), indices.begin() + n);

    // Reemplazar cada color menos frecuente con el color más cercano
    for (int idx : colorsToRemove) {
        int closestIdx = findClosestColor(image, idx, colorsToRemove);
        if (closestIdx != -1) { // Asegurarse de que se encontró un color válido
            image.pixels[idx].red = image.pixels[closestIdx].red;
            image.pixels[idx].green = image.pixels[closestIdx].green;
            image.pixels[idx].blue = image.pixels[closestIdx].blue;
        }
    }
}

// Función de compresión
void compressionCPPM(const std::string &outputFile, AOS &image) {
    std::ofstream ofs(outputFile, std::ios::binary);
    if (!ofs || image.maxval <= 0 || image.maxval >= MAX_COLOR_VALUE) {
        std::cerr << "Error: Could not open file or maxval out of range" << std::endl;
        return;
    }

    AOS colorTable;
    colorTable.width = image.width;
    colorTable.height = image.height;
    colorTable.maxval = image.maxval;

    std::vector<int> pixelIndices;
    int totalPixels = image.width * image.height;
    std::unordered_map<std::tuple<unsigned short, unsigned short, unsigned short>, int> colorMap;

    for (int i = 0; i < totalPixels; i++) {
        auto &pixel = image.pixels[i];
        auto color = std::make_tuple(pixel.red, pixel.green, pixel.blue);

        // Check if color already exists in map
        auto it = colorMap.find(color);
        if (it != colorMap.end()) {
            // If color exists, add the corresponding index
            pixelIndices.push_back(it->second);
        } else {
            // If color is new, add it to the color table and map
            int newIndex = colorTable.pixels.size();
            colorTable.pixels.push_back(pixel);
            colorMap[color] = newIndex;
            pixelIndices.push_back(newIndex);
        }
    }

    ofs << "C6 " << image.width << " " << image.height << " " << image.maxval << " " << colorTable.pixels.size() << "\n";
    int bytesPerColor = (image.maxval <= 255) ? 1 : 2;
    writeColorTable(ofs, colorTable.pixels, bytesPerColor);
    writePixelIndices(ofs, pixelIndices, colorTable.pixels.size());
    ofs.close();
}


// Escribir tabla de colores
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

// Escribir índices de píxeles
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

// Escribir en formato little-endian
void writeLittleEndian(std::ofstream &ofs, unsigned int value, int byteCount) {
    for (int i = 0; i < byteCount; ++i) {
        unsigned char byte = value & 0xFF;
        ofs.write(reinterpret_cast<const char*>(&byte), 1);
        value >>= 8;
    }
}

