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

    return 0;
}