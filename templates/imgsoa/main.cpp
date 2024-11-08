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

int main() {
    // Initialize an SOA structure to hold the image data
    SOA image;

    // Example file path to a PPM file
    string filePath = "example.ppm";

    try {
        // Load the image from a PPM file
        if (!filePMM(filePath, image)) {
            cerr << "Failed to load the image from " << filePath << endl;
            return 1;
        }
        cout << "Image loaded successfully!" << endl;
        
        // Display the original image information
        cout << "Original Image - Width: " << image.width 
                  << ", Height: " << image.height 
                  << ", Maxval: " << image.maxval << endl;

        // Scale the intensity of the image colors
        int oldMaxIntensity = image.maxval;
        int newMaxIntensity = 128; // Example target max intensity
        scaleIntensitySOA(image, oldMaxIntensity, newMaxIntensity);
        cout << "Image intensity scaled to new max intensity: " << newMaxIntensity << endl;

        // Create a new SOA struct to hold the resized image
        SOA resizedImage;

        // Resize the image to new dimensions (example values)
        int newWidth = image.width / 2;
        int newHeight = image.height / 2;
        sizescaling(newHeight, newWidth, image, resizedImage);
        cout << "Image resized to new dimensions - Width: " << newWidth 
                  << ", Height: " << newHeight << endl;

        // Remove the least frequent colors
        int leastFrequentColorsToRemove = 10; // Example number of colors to remove
        removeLeastFrequentColors(resizedImage, leastFrequentColorsToRemove);
        cout << "Removed " << leastFrequentColorsToRemove << " least frequent colors from the image." << endl;

        // Process complete
        cout << "Image processing complete!" << endl;

    } catch (const exception &e) {
        // Catch and print any errors encountered during processing
        cerr << "An error occurred: " << e.what() << endl;
        return 1;
    }

    return 0;
}
