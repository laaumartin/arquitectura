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
using namespace std;

// Constants to replace magic numbers
const int MAX_COLOR_VALUE = 65536;
const int MAX_BYTE_VALUE = 255;
const int COLOR_COMPONENTS = 3;
const float EPSILON = 1e-5;

#include "imgsoa.h"  // Asegúrate de que imgsoa.h contiene las declaraciones de SOA y scaleIntensitySOA

using namespace std;

int main() {
    SOA image;
    string inputFilePath = "lake-large.ppm";  // Cambia a la ruta de tu archivo de prueba

    try {
        // Medir el tiempo antes de cargar la imagen
        clock_t start = clock();

        // Cargar la imagen desde el archivo .ppm
        if (!filePMM(inputFilePath, image)) {
            cerr << "Error: No se pudo cargar la imagen desde " << inputFilePath << endl;
            return 1;
        }
        cout << "Imagen cargada exitosamente: " << inputFilePath << endl;
        cout << "Dimensiones originales: " << image.width << "x" << image.height << ", Valor máximo original: " << image.maxval << endl;

        // Número de colores menos frecuentes a eliminar
        int leastFrequentColorsToRemove = 2;
        removeLeastFrequentColors(image, leastFrequentColorsToRemove);

        // Medir el tiempo después de realizar el procesamiento de colores
        clock_t end = clock();

        // Calcular el tiempo en segundos
        double elapsed_secs = double(end - start) / CLOCKS_PER_SEC;
        cout << "Tiempo de ejecución: " << elapsed_secs << " segundos" << endl;

        // Imprimir algunos resultados para verificar
        cout << "Resultados después de removeLeastFrequentColors:" << endl;
        for (int i = 0; i < min(10, static_cast<int>(image.red.size())); ++i) {
            cout << "Píxel " << i << " - Rojo: " << image.red[i]
                 << ", Verde: " << image.green[i]
                 << ", Azul: " << image.blue[i] << endl;
        }

    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}