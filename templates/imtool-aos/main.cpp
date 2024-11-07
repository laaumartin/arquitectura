#include <iostream>
#include <vector>
#include <string>
#include "proargs.cpp" // Incluye la cabecera adecuada si la tienes

using namespace std;

int main() {
    vector<string> words;
    string w;
    cout << "end them with a '.': ";
    while (cin >> w && w != ".") {
        words.push_back(w);
    }
     // Initialize an SOA structure to hold the image data
    SOA image;

    // Example file path to a PPM file
    std::string filePath = "example.ppm";

    try {
        // Load the image from a PPM file
        if (!filePMM(filePath, image)) {
            std::cerr << "Failed to load the image from " << filePath << std::endl;
            return 1;
        }
        std::cout << "Image loaded successfully!" << std::endl;
        
        // Display the original image information
        std::cout << "Original Image - Width: " << image.width 
                  << ", Height: " << image.height 
                  << ", Maxval: " << image.maxval << std::endl;

        // Scale the intensity of the image colors
        int oldMaxIntensity = image.maxval;
        int newMaxIntensity = 128; // Example target max intensity
        scaleIntensitySOA(image, oldMaxIntensity, newMaxIntensity);
        std::cout << "Image intensity scaled to new max intensity: " << newMaxIntensity << std::endl;

        // Create a new SOA struct to hold the resized image
        SOA resizedImage;

        // Resize the image to new dimensions (example values)
        int newWidth = image.width / 2;
        int newHeight = image.height / 2;
        sizescaling(newHeight, newWidth, image, resizedImage);
        std::cout << "Image resized to new dimensions - Width: " << newWidth 
                  << ", Height: " << newHeight << std::endl;

        // Remove the least frequent colors
        int leastFrequentColorsToRemove = 10; // Example number of colors to remove
        removeLeastFrequentColors(resizedImage, leastFrequentColorsToRemove);
        std::cout << "Removed " << leastFrequentColorsToRemove << " least frequent colors from the image." << std::endl;

        // Process complete
        std::cout << "Image processing complete!" << std::endl;

    } catch (const std::exception &e) {
        // Catch and print any errors encountered during processing
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    
    int result = processArguments(words);
    if (result != 0) {
        return result;  // Salir con el código de error si algo falla
    }

    // Aquí puedes agregar la lógica adicional que desees tras la validación

    return 0;
}
