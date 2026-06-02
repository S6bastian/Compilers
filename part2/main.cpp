#include "Grammar.h"
#include "LR1Parser.h"
#include <iostream>
#include <string>

using namespace std;

int main() {
    string grammarFilename = "grammar.txt";
    Grammar myGrammar(grammarFilename);

    cout << "\nStarting LR(1) Parser Construction...\n";
    LR1Parser lr1(&myGrammar);

    cout << "\n=== LR(1) PARSER READY ===\n";

    string inputFile = "example.txt";
    bool success = lr1.parse(inputFile);

    if (success) {
        cout << "\n==================================================\n";
        cout << "===       FASÉ DE TRADUCCIÓN A LATEX           ===\n";
        cout << "==================================================\n\n";

        // Disparamos nuestro nuevo Backend traductor
        string outputLatexFile = "output.tex";
        lr1.generateLatex(outputLatexFile);

        cout << "\n==================================================\n";
    } else {
        cout << "\nTraducción cancelada debido a errores sintácticos.\n";
    }

    return 0;
}
