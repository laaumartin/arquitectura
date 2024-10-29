#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdexcept>
using namespace std;


int cvalid(const string& w3, const vector<string>& words, int w4, int w5) {
    if (w3 == "info" || w3 == "compress") {
        if (words.size() != 6) {
            cout << "Error: invalid number of arguments"<< "'" << endl;
            return -1;
        }
    } else if (w3 == "cutfreq") {
        if (words.size() != 7 || w4 < 0) {
            cout << "Error: invalid number of arguments or input data" << endl;
            return -1;
        }
    } else if (w3 == "maxlevel") {
        if (words.size() != 7 || w4 < 0 || w4 > 65535) {
            cout << "Error: invalid number of arguments or input data" << endl;
            return -1;
        }
    } else if (w3 == "resize") {
        if (words.size() != 8 || w4 < 0 || w5 < 0) {
            cout << "Error: invalid number of arguments or input data" << endl;
            return -1;
        }
    } else {
        cout << "Error: Not existing command'" << w3 << "'" << endl;
        return -1;
    }
    return 0;
}

// Función para procesar los argumentos
int processArguments(const vector<string>& words) {
    if (words.size() < 3) {
        cout << "Error: Invalid number of arguments: " << words.size() << endl;
        return -1;
    }

    string wthree = words[2];
    if (wthree != "info" && wthree != "maxlevel" && wthree != "resize" && wthree != "cutfreq" && wthree != "compress") {
        cout << "Error: Invalid argument 3: " << wthree << endl;
        return -1;
    }

    int w4 = 0, w5 = 0;
    try {
        if (words.size() > 3) {
            w4 = stoi(words[3]);
        }
        if (words.size() > 4) {
            w5 = stoi(words[4]);
        }
    } catch (invalid_argument&) {
        cout << "Error: Invalid argument" << endl;
        return -1;
    }

    return cvalid(wthree, words, w4, w5);
}