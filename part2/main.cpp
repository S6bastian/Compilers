#include "Grammar.h"
#include "LR1Parser.h"
#include <iostream>
#include <string>

using namespace std;

int main() {
    // 1. Cargamos la gramática optimizada para LR(1) desde grammar.txt
    string grammarFilename = "grammar.txt";
    Grammar myGrammar(grammarFilename);
    myGrammar.printDebug();

    cout << "\nStarting LR(1) Parser Construction...\n";
    LR1Parser lr1(&myGrammar);

    cout << "\n=== LR(1) PARSER READY ===\n";
    cout << "Processing 'example.txt' through the Scanner...\n\n";

    // 2. Procesamos el archivo de entrada
    string inputFile = "example.txt";
    bool success = lr1.parse(inputFile);

    // 3. Si el análisis fue correcto, mostramos el Árbol Sintáctico
    if (success) {
        cout << "\n==================================================\n";
        cout << "=== ARBOL SINTACTICO DE DERIVACION (PARSE TREE) ===\n";
        cout << "==================================================\n\n";

        // Invocamos la función nativa que ya tenías en tu LR1Parser
        lr1.printParseTree(lr1.getParseTreeRoot());

        cout << "\n==================================================\n";
    }

    return 0;
}
