#include "Grammar.h"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main(){
    Grammar myGrammar;
    string filename = "grammar.txt";

    // GRAMMAR TEST ----------------------------------------------------------------------------

    cout << "--- Cargando gramatica desde: " << filename << " ---" << "\n";
    myGrammar.loadFromFile(filename);

    cout << "Simbolo inicial: " << myGrammar.getStartSymbol() << "\n";

    for (const auto& prod : myGrammar.getProductions()) {
        cout << prod.first << " -> ";
        
        for (size_t i = 0; i < prod.second.size(); ++i) {
            cout << "[" << prod.second[i] << "]"; 
            if (i < prod.second.size() - 1) cout << " ";
        }
        cout << endl;
    }

    return 0;
}