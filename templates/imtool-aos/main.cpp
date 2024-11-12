#include <iostream>
#include <string>
#include <vector>
#include "imageaos.cpp"   // Include the image processing functions for AOS

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <inputFile> <outputFile> <operation> [additional parameters]" << endl;
        return -1;
    }

    string inputFile = argv[1];
    string outputFile = argv[2];
    string operation = argv[3];
    AOS image;

    // Load the image from a PPM file
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
            if (argc < 5) {
                cerr << "Error: Missing new max intensity value for maxlevel operation" << endl;
                return -1;
            }
            int newMaxIntensity = stoi(argv[4]);
            scaleIntensityAOS(image, image.maxval, newMaxIntensity);
            filePPM(outputFile, image);  // Save the scaled image
        }
        else if (operation == "resize") {
            if (argc < 6) {
                cerr << "Error: Missing new width and height for resize operation" << endl;
                return -1;
            }
            int newWidth = stoi(argv[4]);
            int newHeight = stoi(argv[5]);
            AOS resizedImage;
            sizescaling(newWidth, newHeight, image, resizedImage);
            filePPM(outputFile, resizedImage);  // Save the resized image
        }
        else if (operation == "cutfreq") {
            if (argc < 5) {
                cerr << "Error: Missing number of colors to cut for cutfreq operation" << endl;
                return -1;
            }
            int n = stoi(argv[4]);
            removeLeastFrequentColors(image, n);
            filePPM(outputFile, image);  // Save the image with reduced colors
        }
        else if (operation == "compress") {
            compressionCPPM(outputFile, image);  // Compress and save in cppm format
        }
        else {
            cerr << "Error: Unknown operation " << operation << endl;
            return -1;
        }
    } catch (const invalid_argument& e) {
        cerr << "Error: invalid argument - " << e.what() << "\n";
        return -1;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return -1;
    }

    return 0;
}
