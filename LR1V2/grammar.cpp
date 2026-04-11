#include "Grammar.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;

void Grammar::loadFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "No se pudo abrir: " << filename << endl;
        return;
    }

    string line;
    bool firstLine = true;
    int startSymbolCount = 0;
    
    while (getline(file, line)) {
        if (line.empty()) continue;

        size_t arrow = line.find("->");
        string head  = line.substr(0, arrow);
        string right = line.substr(arrow + 2);

        head.erase(remove(head.begin(), head.end(), ' '), head.end());

        vector<string> body;
        istringstream ss(right);
        string symbol;
        while (ss >> symbol) body.push_back(symbol);

        productions.push_back({head, body});
        nonTerminals.insert(head);
        
        if (firstLine){
            startSymbol = head;
            firstLine = false;     
        }

        if (head == startSymbol) startSymbolCount++;
    }

    if(startSymbolCount > 1){
        string head = startSymbol + "+";
        productions.push_back({head,{startSymbol}});
        nonTerminals.insert(head);
        startSymbol = head;
    }

    stable_sort(productions.begin(), productions.end(), [this](const auto& a, const auto& b){
        // Si 'a' es el símbolo inicial y 'b' no, 'a' va primero
        if (a.first == startSymbol && b.first != startSymbol) return true;
        // Si 'b' es el símbolo inicial y 'a' no, 'b' va primero
        if (b.first == startSymbol && a.first != startSymbol) return false;
        
        // Para los demás, ordenamos alfabéticamente
        return a.first < b.first;
    });
}



const vector<pair<string, vector<string>>>& Grammar::getProductions() const{
    return productions;
}



const string& Grammar::getStartSymbol() const{
    return startSymbol;
}