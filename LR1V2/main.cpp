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
    
    // Configurar recuperación de errores
    lr1.setPanicMode(true);    // Activar modo pánico
    lr1.setMaxErrors(5);       // Máximo 5 errores antes de abortar
    
    cout << "\n=== LR(1) PARSER WITH ERROR RECOVERY ===\n";
    cout << "Enter input tokens separated by spaces (or 'quit' to exit):\n";
    cout << "Commands:\n";
    cout << "  'panic on/off' - Toggle panic mode\n";
    cout << "  'errors N'     - Set maximum errors to N\n";
    cout << "  'quit'         - Exit program\n\n";
    
    string input;
    while (true) {
        cout << "\n> ";
        getline(cin, input);
        
        if (input == "quit" || input == "exit") {
            break;
        }
        
        if (input.empty()) {
            continue;
        }
        
        // Comandos especiales
        if (input == "panic on") {
            lr1.setPanicMode(true);
            cout << "Panic mode enabled\n";
            continue;
        }
        else if (input == "panic off") {
            lr1.setPanicMode(false);
            cout << "Panic mode disabled\n";
            continue;
        }
        else if (input.substr(0, 7) == "errors ") {
            int max = stoi(input.substr(7));
            lr1.setMaxErrors(max);
            cout << "Maximum errors set to " << max << "\n";
            continue;
        }
        
        bool result = lr1.parse(input);
        
        if (result) {
            cout << "\nParsing completed successfully!\n";
            if (lr1.getErrorCount() > 0) {
                cout << "Recovered from " << lr1.getErrorCount() << " error(s)\n";
            }
        } else {
            cout << "\nParsing failed!\n";
        }
    }

    return 0;
}