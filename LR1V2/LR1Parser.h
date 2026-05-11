#pragma once
#include "Grammar.h"
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <iostream>
#include <iomanip>

using namespace std;


struct LR1Item {
    string head;
    vector<string> body;
    int dot;
    set<string> lookahead;

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

    void buildStates(); //buildCanonicalCollection
    void printStates() const;

private:
    Grammar *grammar;
    
    vector<State> states; // Canonical Collection 
    
    // Transiciones entre estados: [ID_Origen][Simbolo] -> ID_Destino
    map<int, map<string, int>> transitions;

    // Funciones núcleo para la lógica de estados
    vector<LR1Item> closure(vector<LR1Item> kernels);
    set<string> computeLookahead(LR1Item item);

};