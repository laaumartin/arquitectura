#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <tuple>
#include <ctime>
#include "imageaos.h"

using namespace std;

// Constants to replace magic numbers
const int MAX_COLOR_VALUE = 65536;
const int MAX_BYTE_VALUE = 255;
const int COLOR_COMPONENTS = 3;
const float EPSILON = 1e-5;


int main() {
    AOS image;
    string inputFilePath = "lake-large.ppm";  // Cambia a la ruta de tu archivo de prueba

    try {
        clock_t start = clock();

        if (!filePMM(inputFilePath, image)) {
            cerr << "Error: No se pudo cargar la imagen desde " << inputFilePath << endl;
            return 1;
        }
        cout << "Imagen cargada exitosamente: " << inputFilePath << endl;
        cout << "Dimensiones originales: " << image.width << "x" << image.height << ", Valor máximo original: " << image.maxval << endl;

        int leastFrequentColorsToRemove = 2;
        removeLeastFrequentColors(image, leastFrequentColorsToRemove);

        clock_t end = clock();
        double elapsed_secs = double(end - start) / CLOCKS_PER_SEC;
        cout << "Tiempo de ejecución: " << elapsed_secs << " segundos" << endl;

        cout << "Resultados después de removeLeastFrequentColors:" << endl;
        for (int i = 0; i < min(10, static_cast<int>(image.pixels.size())); ++i) {
            cout << "Píxel " << i << " - Rojo: " << image.pixels[i].red
                 << ", Verde: " << image.pixels[i].green
                 << ", Azul: " << image.pixels[i].blue << endl;
        }

    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}