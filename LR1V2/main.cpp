#include "Grammar.h"
#include "LR1Parser.h"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main(){
    string filename = "grammar.txt";
    Grammar myGrammar(filename);
    //myGrammar.printDebug();
    LR1Parser lr1(&myGrammar);

    lr1.printStates();


    
    
    return 0;
}