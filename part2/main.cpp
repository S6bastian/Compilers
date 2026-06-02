#include "Grammar.h"
#include "LR1Parser.h"
#include <iostream>
#include <string>

using namespace std;

int main() {
    // 1. Cargamos la gramática optimizada para LR(1) que guardaste en grammar.txt
    string grammarFilename = "grammar.txt";
    Grammar myGrammar(grammarFilename);
    myGrammar.printDebug();

    cout << "\nStarting LR(1) Parser Construction...\\n";
    LR1Parser lr1(&myGrammar);

    cout << "\n=== LR(1) PARSER READY ===\n";
    cout << "Processing 'example.txt' through the Scanner...\n\n";

    // 2. Le pasamos el nombre del archivo de entrada a tu parser
    string inputFile = "example.txt";
    lr1.printParseTrace(inputFile);

    return 0;
}
