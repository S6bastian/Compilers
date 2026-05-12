#include "Grammar.h"
#include "LR1Parser.h"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main(){
    string filename = "grammar.txt";
    Grammar myGrammar(filename);
    myGrammar.printDebug();
    
    cout << "\nStarting LR(1) Parser Construction...\n";
    LR1Parser lr1(&myGrammar);
    
    string input;
    cout << "\n=== LR(1) PARSER READY ===\n";
    cout << "Enter input tokens separated by spaces (or 'quit' to exit):\n";
    
    while (true) {
        cout << "\n> ";
        getline(cin, input);
        
        if (input == "quit" || input == "exit") {
            break;
        }
        
        if (input.empty()) {
            continue;
        }
        
        lr1.printParseTrace(input);  
    }

    return 0;
}