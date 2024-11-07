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
     
    int result = processArguments(words);
    if (result != 0) {
        return result;  // Salir con el código de error si algo falla
    }

    // Aquí puedes agregar la lógica adicional que desees tras la validación

    return 0;
}
