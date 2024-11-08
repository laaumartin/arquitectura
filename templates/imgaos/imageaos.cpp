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

using namespace std;

struct Pixel {
  unsigned short red;
  unsigned short green;
  unsigned short blue;
};

bool operator==(const Pixel& lhs, const Pixel& rhs) {
  return lhs.red == rhs.red && lhs.green == rhs.green && lhs.blue == rhs.blue;
}

struct AOS {
  int width;
  int height;
  int maxval;
  vector<Pixel> pixels; // Vector que almacena cada píxel completo
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

// Declaración de funciones
void skipComments(ifstream &ifs);
bool filePPM(const string &file , AOS &image);
void scaleIntensityAOS(AOS& image, int newMaxIntensity);
void sizescaling(const int newWidth, const int newHeight, const AOS &image, AOS &newImage);
unsigned short interpolatePixel(const AOS &image, int xl, int yl, int xh, int yh, float x, float y, char colorComponent);
vector<int> calculateColorFrequencies(const AOS &image, const int pixelCount);
int findClosestColor(const AOS &image, int idx, const vector<int> &excludedIndices);
void removeLeastFrequentColors(AOS &image, const int n);
void compressionCPPM(const string &outputFile, AOS &image);
void writeColorTable(ofstream &ofs, const vector<Pixel> &colorTable, int bytesPerColor);
void writePixelIndices(ofstream &ofs, const vector<int> &pixelIndices, int colorTableSize);
void writeLittleEndian(ofstream &ofs, uint32_t value, int byteCount);

// Función para leer un archivo PPM
bool filePMM(const string &file , AOS &image) {
  ifstream ifs(file.c_str(), ios::binary);
  if (!ifs.is_open()) {
    cerr << "No es posible abrir el archivo " << file << '\n';
    return false;
  }
  string magicnb;
  ifs >> magicnb;
  const string MAGIC_NUMBER = "P6";
  if (magicnb != MAGIC_NUMBER) {
    cerr << "El formato del archivo no es correcto (debería ser P6)" << '\n';
    return false;
  }

  ifs >> image.width >> image.height >> image.maxval;
  const int MAX_COLOR_VALUE = 65536;
  if (image.maxval <= 0 || image.maxval > MAX_COLOR_VALUE) {
    cerr << "El valor máximo de color no está en el rango (1-65535)" << '\n';
    return false;
  }

  ifs.ignore();

  int pixelCount = image.width * image.height;
  image.pixels.resize(pixelCount);

  int bytesPerColor = (image.maxval <= 255) ? 1 : 2;
  for (int i = 0; i < pixelCount; ++i) {
    if (bytesPerColor == 1) {
      unsigned char r, g, b;
      ifs.read(reinterpret_cast<char*>(&r), 1);
      ifs.read(reinterpret_cast<char*>(&g), 1);
      ifs.read(reinterpret_cast<char*>(&b), 1);
      image.pixels[i].red = r;
      image.pixels[i].green = g;
      image.pixels[i].blue = b;
    } else {
      unsigned char buffer[2];
      ifs.read(reinterpret_cast<char*>(buffer), 2);
      image.pixels[i].red = buffer[0] | (buffer[1] << 8);

      ifs.read(reinterpret_cast<char*>(buffer), 2);
      image.pixels[i].green = buffer[0] | (buffer[1] << 8);

      ifs.read(reinterpret_cast<char*>(buffer), 2);
      image.pixels[i].blue = buffer[0] | (buffer[1] << 8);
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

    //image.pixels[i].red = clamp(image.pixels[i].red, 0, newMaxIntensity);
    //image.pixels[i].green = clamp(image.pixels[i].green, 0, newMaxIntensity);
    //image.pixels[i].blue = clamp(image.pixels[i].blue, 0, newMaxIntensity);
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

// Función para calcular frecuencias de color
vector<int> calculateColorFrequencies(const AOS &image, const int pixelCount) {
  unordered_map<tuple<unsigned short, unsigned short, unsigned short>, int> freqMap;

  for (const auto& pixel : image.pixels) {
    auto color = make_tuple(pixel.red, pixel.green, pixel.blue);
    freqMap[color]++;
  }

  vector<int> freq(pixelCount);
  for (int i = 0; i < pixelCount; ++i) {
    auto color = make_tuple(image.pixels[i].red, image.pixels[i].green, image.pixels[i].blue);
    freq[i] = freqMap[color];
  }
  return freq;
}

// Buscar color más cercano
int findClosestColor(const AOS &image, int idx, const vector<int> &excludedIndices) {
  double minDist = numeric_limits<double>::max();
  int closestIdx = -1;
  for (int j = 0; j < image.width * image.height; ++j) {
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

// Eliminar colores menos frecuentes
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

// Función de compresión
void compressionCPPM(const string &outputFile, AOS &image) {
  ofstream ofs(outputFile, ios::binary);
  if (!ofs || image.maxval <= 0 || image.maxval >= 65536) {
    cerr << "Error al abrir el archivo o valor máximo fuera de rango" << '\n';
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
  ofs << "C6 " << image.width << " " << image.height << " " << image.maxval << " " << colorTable.size() << '\n';
  writeColorTable(ofs, colorTable, (image.maxval <= 255) ? 1 : 2);
  writePixelIndices(ofs, pixelIndices, colorTable.size());

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
void writeLittleEndian(ofstream &ofs, unsigned short value, int byteCount) {
  for (int i = 0; i < byteCount; ++i) {
    unsigned char byte = value & 0xFF;
    ofs.write(reinterpret_cast<const char*>(&byte), 1);
    value >>= 8;
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
