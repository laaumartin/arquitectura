#include <vector>
#include <string>
#include<stdexcept>
#include <fstream>
#include <iostream>
#include<cmath>
#include <algorithm> 
using namespace std;

struct SOA {
    int width;
    int height;
    int maxval;
    vector<unsigned char> red;
    vector<unsigned char> green;
    vector<unsigned char> blue;

};
// function 2.1
bool filePMM(const string &file , SOA &image) {
    ifstream ifs(file.c_str(), ios::binary);
    if (!ifs.is_open() ) {
        cerr<<"not possible to open the file" <<file<<endl;
        return false;
    }
    //check if the magic number identifying the file type is  P6
    string magicnb;
    ifs >> magicnb;
    if(magicnb.compare("P6") !=0){
        cerr<<"The PMM format is not the correct one"<<endl;
        return false;
    }
    ifs>>image.width>>image.height>>image.maxval;
    if (image.maxval<=0 || image.maxval >=65536) {
        cerr<<"the maximum color value is not inside the range"<<endl;
        return false;
    }
    ifs.ignore(); //ignoring white spaces
    int pixel = image.height*image.width;
    int bytes = 1 ;//if the image maxval<255
    if(image.maxval>255) {
        bytes=2; // if it is bigger than 255 then it will take 2 bytes
    }
    // resize the bytes of each color depending on the maximum value
    image.red.resize(bytes *pixel);
    image.green.resize(bytes *pixel);
    image.blue.resize(bytes *pixel);
    // order each pixel color component into arrays
    for (int i = 0; i < pixel; i++) {
        ifs.read(reinterpret_cast<char*>(&image.red[i * bytes]), bytes);
        ifs.read(reinterpret_cast<char*>(&image.green[i * bytes]), bytes);
        ifs.read(reinterpret_cast<char*>(&image.blue[i * bytes]), bytes);

    }
}

//function2.2 
//vector de rojos, vector de verdes, vector de azules, antigua intensidad, nueva intensidad
void scaleIntensitySOA(SOA& image, int oldMaxIntensity, int newMaxIntensity) {
    int pixelCount = image.red.size(); //suponiendo que todos tienen el mismo numero

    for (int i = 0; i < pixelCount; ++i) {
        //formula que da el profe. round es para que sea entero
        image.red[i] = static_cast<unsigned char>(round(image.red[i] * newMaxIntensity / oldMaxIntensity));
        image.green[i] = static_cast<unsigned char>(round(image.green[i] * newMaxIntensity / oldMaxIntensity));
        image.blue[i] = static_cast<unsigned char>(round(image.blue[i] * newMaxIntensity / oldMaxIntensity));

        // esto es para corroborar que esta dentro del rango nuevo lo ha añadido chatgpt pero yo lo quitaria
        image.red[i] = clamp(image.red[i], 0, newMaxIntensity);
        image.green[i] = clamp(image.green[i], 0, newMaxIntensity);
        image.blue[i] = clamp(image.blue[i], 0, newMaxIntensity);
    }
}

//function 2.3 
void sizescaling ( int newheight, int newwidth, SOA &image , SOA &newimage) {
    newimage.height = newheight;
    newimage.width = newwidth;

    for (int ynew=0; ynew< newheight; ++ynew) {
        for (int xnew = 0; xnew < newwidth; ++xnew) {
            //the division for obtaining x and y
            float x = xnew*(image.width-1) / static_cast<float>(newwidth-1); //static is for transforming an element into a float point
            float y = ynew*(image.height-1) / static_cast<float>(newheight-1);
            //floors of each variable
            int xl = static_cast<int>(floor(x));
            int yl = static_cast<int>(floor(y));
            //ceils of each variable
            int xh = static_cast<int>(ceil(x));
            int yh = static_cast<int>(ceil(y));
            // bilineal interpolation 
            auto interpolate = [&](const vector<unsigned char> color ){ //color refers to the different colors of the image
                float c1= color[yl*image.width + xl]* (xh-x) + color[yl*image.width + xh]*(x-xl);
                float c2= color[yh*image.width + xl]* (xh-x) + color[yh*image.width + xh]*(x-xl);
                unsigned char interpolatedcolor = static_cast<unsigned char>(c1 * (yh - y) + c2 * (y - yl));
                return interpolatedcolor;
            };
            //New pixel index for the new image
            int newindex =ynew* newwidth + xnew;
            // resize colors
            newimage.red[newindex] = interpolate(image.red);
            newimage.green[newindex] = interpolate(image.green);
            newimage.blue[newindex] = interpolate(image.blue);
        }
    }
}
// function 2.4 
void removeLeastFrequentColors(SOA& image, int n) {
    int pixelCount = image.width * image.height;
    vector<int> freq(pixelCount, 0);
    for (int i = 0; i < pixelCount; ++i)  // frecuencia de cada color en cada pixel
        for (int j = 0; j < pixelCount; ++j)
            if (image.red[i] == image.red[j] && image.green[i] == image.green[j] && image.blue[i] == image.blue[j])
                freq[i]++;
    vector<int> indices(pixelCount);
    iota(indices.begin(), indices.end(), 0); // crear vector con indices ordenados
    sort(indices.begin(), indices.end(), [&](int a, int b) { // ordenar menos frecuentes y si es empate se miran los colores
        return freq[a] < freq[b] || (freq[a] == freq[b] && tie(image.blue[a], image.green[a], image.red[a]) > tie(image.blue[b], image.green[b], image.red[b]));
    });
    vector<int> colorsToRemove(indices.begin(), indices.begin() + n); // se menten los menos frecuentes para eliminarlos
    vector<int> replacements(pixelCount, -1);//se inicia con menos uno pq indica q no se reemplazan
    for (int idx : colorsToRemove) { // ver cual es el color mas cercano
        double minDist = numeric_limits<double>::max();//max para q cualquiera sea menor
        for (int j = 0; j < pixelCount; ++j) {
            if (find(colorsToRemove.begin(), colorsToRemove.end(), j) != colorsToRemove.end()) continue; //pasar de los menos frecuentes
            double dist = sqrt(pow(image.red[idx] - image.red[j], 2) + pow(image.green[idx] - image.green[j], 2) + pow(image.blue[idx] - image.blue[j], 2)); //calcular la distancia
            if (dist < minDist) minDist = dist, replacements[idx] = j; //actualizarla
        }
    }
    for (int i = 0; i < pixelCount; ++i) { //cambiar los pixeles
        if (replacements[i] != -1) {//pa ver si hay q reemplazarlo
            int replaceIdx = replacements[i];
            image.red[i] = image.red[replaceIdx]; //y esto es pa cambiar el menos frecuente por el que este mas cerca
            image.green[i] = image.green[replaceIdx];
            image.blue[i] = image.blue[replaceIdx];
        }
    }
}

