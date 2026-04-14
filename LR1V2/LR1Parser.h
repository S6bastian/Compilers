#pragma once
#include "Grammar.h"
#include <set>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>

using namespace std;


struct LR1Item {
    string head;
    vector<string> body;
    int dot;
    string lookahead;

    bool operator<(const LR1Item& other) const {
        if (head != other.head) return head < other.head;
        if (body != other.body) return body < other.body;
        if (dot != other.dot) return dot < other.dot;
        return lookahead < other.lookahead;
    }

    bool operator==(const LR1Item& other) const {
        return head == other.head && body == other.body && 
               dot == other.dot && lookahead == other.lookahead;
    }
};


typedef set<LR1Item> State;

class LR1Parser {
public:
    LR1Parser(Grammar *g);

    void buildCanonicalCollection();
    void printStates() const;

private:
    Grammar *grammar;
    
    // Esta es tu Colección Canónica (Lista de Estados)
    vector<State> states; 
    
    // Transiciones entre estados: [ID_Origen][Simbolo] -> ID_Destino
    map<int, map<string, int>> transitions;

    // Funciones núcleo para la lógica de estados
    State closure(State I);
    State goTo(const State& I, const string& X);
    
    // Calcula el FIRST de lo que queda después de un No Terminal + el lookahead actual
    set<string> computeFirstChain(vector<string> beta, string lookahead);
};