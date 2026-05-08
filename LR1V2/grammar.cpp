#include "Grammar.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;


//********************************************************************************************************************
//CONSTRUCTOR ********************************************************************************************************
//********************************************************************************************************************

Grammar::Grammar(const string& filename, string emptySym){
    emptySymbol = emptySym;
    loadFromFile(filename);
    extractTerminals();
    extractFirsts();
};




//********************************************************************************************************************
//PUBLIC *************************************************************************************************************
//********************************************************************************************************************

void Grammar::loadFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "No se pudo abrir: " << filename << endl;
        return;
    }

    string line;
    bool firstLine = true;
    
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
        
        if (firstLine){
            string augmentedHead = head + "'";
            startSymbol = augmentedHead;
            productions.push_back({augmentedHead, {head}});
            nonTerminals.insert(augmentedHead);

            firstLine = false;     
        }

        productions.push_back({head, body});
        nonTerminals.insert(head);
    }

    stable_sort(productions.begin(), productions.end(), [this](const auto& a, const auto& b){
        if (a.first == startSymbol && b.first != startSymbol) return true;
        if (b.first == startSymbol && a.first != startSymbol) return false;
        return a.first < b.first;
    });
}



const vector<pair<string, vector<string>>>& Grammar::getProductions() const{
    return productions;
}

const string& Grammar::getStartSymbol() const{
    return startSymbol;
}

const string& Grammar::getEmptySymbol() const{
    return emptySymbol;
}

const set<string> Grammar::getFirsts(const string& head) const{
    return firsts.at(head);
}

bool Grammar::isTerminal(const string& symbol){
    return terminals.find(symbol) != terminals.end();
}

bool Grammar::isNonTerminal(const string& symbol){
    return nonTerminals.find(symbol) != nonTerminals.end();
}


//TESTING --------------------------------------------------------------------

void Grammar::printDebug() const {
    cout << "\n=== DEBUG GRAMMAR ===" << endl;
    
    // 1. Imprimir Producciones
    cout << "\n[PRODUCCIONES]:" << endl;
    for (const auto& prod : productions) {
        cout << "  " << prod.first << " -> ";
        if (prod.second.empty()) {
            cout << emptySymbol;
        } else {
            for (const auto& sym : prod.second) {
                cout << sym << " ";
            }
        }
        cout << endl;
    }

    // 2. Imprimir Terminales y No Terminales
    cout << "\n[TERMINALES]: ";
    for (const auto& t : terminals) cout << t << " ";
    
    cout << "\n[NO TERMINALES]: ";
    for (const auto& nt : nonTerminals) cout << nt << " ";
    cout << endl;

    // 3. Imprimir FIRST Sets
    cout << "\n[FIRST SETS]:" << endl;
    for (const auto& nt : nonTerminals) {
        cout << "  FIRST(" << nt << ") = { ";
        auto it = firsts.find(nt);
        if (it != firsts.end()) {
            const auto& s = it->second;
            for (auto symIt = s.begin(); symIt != s.end(); ++symIt) {
                cout << *symIt << (next(symIt) == s.end() ? "" : ", ");
            }
        }
        cout << " }" << endl;
    }
    cout << "=====================\n" << endl;
}





//********************************************************************************************************************
//PRIVATE ************************************************************************************************************
//********************************************************************************************************************

void Grammar::extractTerminals(){ 
    //terminals.insert("$");
    for(auto &production : productions){
        for(auto &symbol : production.second){
            if(nonTerminals.find(symbol) == nonTerminals.end() && symbol != emptySymbol){
                terminals.insert(symbol);
            }
        }
    }
}

void Grammar::extractFirsts(){
    for (const string& t : terminals) {
        firsts[t].insert(t);
    }

    bool changed = true;
    while(changed){
        changed = false;
        for(auto &production : productions){
            const string& head = production.first;
            const vector<string>& body = production.second;

            size_t beforeSize = firsts[head].size();

            if (body.empty() || body[0] == emptySymbol) {
                firsts[head].insert(emptySymbol);
            } else {
                for (const string& symbol : body) {
                    bool hasEpsilon = false;
                    
                    for (const string& f : firsts[symbol]) {
                        if (f == emptySymbol) hasEpsilon = true;
                        else firsts[head].insert(f);
                    }

                    if (!hasEpsilon) break;

                    if (&symbol == &body.back() && hasEpsilon) {
                        firsts[head].insert(emptySymbol);
                    }
                }
            }
            

            if (firsts[head].size() > beforeSize) {
                changed = true;
            }
        }
    }
}




