#include <iostream>
#include <string>
#include <vector>
#include "imagesoa.cpp"   // Funciones de procesamiento de imágenes

using namespace std;

int main(int argc, char* argv[]) {
    vector<string> args(argv, argv + argc);  // Convierte los argumentos en un vector de strings

    string inputFile = args[1];
    string outputFile = args[2];
    string operation = args[3];
    SOA image;

    // Cargar la imagen desde el archivo PPM
    if (!filePPM(inputFile, image)) {
        cerr << "Error with the file " << inputFile << endl;
        return -1;
    }

    try {
        if (operation == "info") {
            cout << "Width: " << image.width << "\n";
            cout << "Height: " << image.height << "\n";
            cout << "Max Color Value: " << image.maxval << "\n";
        }
        else if (operation == "maxlevel") {
            int newMaxIntensity = stoi(args[4]);
            scaleIntensitySOA(image, image.maxval, newMaxIntensity);
            filePPM(outputFile, image);  // Guarda la imagen escalada
        }
        else if (operation == "resize") {
            int newWidth = stoi(args[4]);
            int newHeight = stoi(args[5]);
            SOA resizedImage;
            sizescaling(newWidth, newHeight, image);
            filePPM(outputFile, resizedImage);  // Guarda la imagen redimensionada
        }
        else if (operation == "cutfreq") {
            int n = stoi(args[4]);
            removeLeastFrequentColors(image, n);
            filePPM(outputFile, image);  // Guarda la imagen con colores eliminados
        }
        else if (operation == "compress") {
            compressionCPPM(outputFile, image);  // Comprime y guarda en formato cppm
        }
    } catch (const invalid_argument& e) {
        cerr << "Error: invalid argument " << e.what() << "\n";
        return -1;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return -1;
    }

    return 0;
}