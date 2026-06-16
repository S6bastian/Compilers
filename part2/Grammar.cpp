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
    extractFollows();
};


Grammar::Grammar(const vector<string>& grammarLines, string emptySym) {
    emptySymbol = emptySym;
    loadFromLines(grammarLines);
    extractTerminals();
    extractFirsts();
    extractFollows(); // Tu función de follow implementada antes
}




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
    /*
    stable_sort(productions.begin(), productions.end(), [this](const auto& a, const auto& b){
        if (a.first == startSymbol && b.first != startSymbol) return true;
        if (b.first == startSymbol && a.first != startSymbol) return false;
        return a.first < b.first;
    });
    */
}


void Grammar::loadFromLines(const vector<string>& lines) {
    bool firstLine = true;

    for (string line : lines) {
        if (line.empty()) continue;

        // Limpieza básica de espacios o retornos de carro (\r) comunes en Windows
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) continue;

        size_t arrow = line.find("->");
        if (arrow == string::npos) continue; // Por si acaso hay una línea inválida

        string head  = line.substr(0, arrow);
        string right = line.substr(arrow + 2);

        // Quitar espacios de la cabeza (head)
        head.erase(remove_if(head.begin(), head.end(), ::isspace), head.end());

        if (firstLine) {
            startSymbol = head;
            firstLine = false;
        }

        nonTerminals.insert(head);

        stringstream ss(right);
        string symbol;
        vector<string> body;

        while (ss >> symbol) {
            body.push_back(symbol);
        }

        productions.push_back({head, body});
    }
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

bool Grammar::isTerminal(const string& symbol) const{
    return terminals.find(symbol) != terminals.end();
}

bool Grammar::isNonTerminal(const string& symbol) const{
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

    // 4. Imprimir FOLLOW Sets para verificación
        cout << "\n[FOLLOW SETS]:" << endl;
        for (const auto& nt : nonTerminals) {
            cout << "  FOLLOW(" << nt << ") = { ";
            auto it = follows.find(nt);
            if (it != follows.end()) {
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

                    firsts[head].insert(emptySymbol);
                    // if (&symbol == &body.back() && hasEpsilon) {
                    //     firsts[head].insert(emptySymbol);
                    // }
                }
            }


            if (firsts[head].size() > beforeSize) {
                changed = true;
            }
        }
    }
}

const map<string, set<string>>& Grammar::getFollows() const {
    return follows;
}

// Método Privado que calcula los conjuntos FOLLOW iterativamente
void Grammar::extractFollows() {
    // 1. Inicializar el contenedor para todos los No Terminales
    for (const string& nt : nonTerminals) {
        follows[nt] = set<string>();
    }

    // 2. Regla 1: Colocar el símbolo de fin de cadena '$' en el FOLLOW del símbolo inicial
    if (!startSymbol.empty()) {
        follows[startSymbol].insert("$");
    }

    bool changed = true;
    while (changed) {
        changed = false;

        // Evaluar cada producción del framework de la gramática
        for (const auto& production : productions) {
            const string& head = production.first;
            const vector<string>& body = production.second;

            // Iterar por cada símbolo del cuerpo de la producción
            for (size_t i = 0; i < body.size(); ++i) {
                const string& currentSymbol = body[i];

                // Solo nos interesan los No Terminales para calcular su FOLLOW
                if (isNonTerminal(currentSymbol)) {
                    size_t beforeSize = follows[currentSymbol].size();

                    // Regla 2: Si hay símbolos después de currentSymbol (A -> a B b)
                    // Añadir FIRST(b) - {epsilon} al FOLLOW(B)
                    bool deriveEpsilon = true;
                    for (size_t j = i + 1; j < body.size(); ++j) {
                        const string& nextSymbol = body[j];
                        bool hasEpsilon = false;

                        // Si es un terminal, su FIRST es él mismo
                        if (isTerminal(nextSymbol)) {
                            follows[currentSymbol].insert(nextSymbol);
                            deriveEpsilon = false;
                            break;
                        }
                        // Si es un No Terminal, añadimos su FIRST (excepto épsilon)
                        else if (isNonTerminal(nextSymbol)) {
                            for (const string& f : firsts[nextSymbol]) {
                                if (f == emptySymbol) {
                                    hasEpsilon = true;
                                } else {
                                    follows[currentSymbol].insert(f);
                                }
                            }
                        }

                        // Si el símbolo actual no transiciona a épsilon, rompemos la cadena
                        if (!hasEpsilon) {
                            deriveEpsilon = false;
                            break;
                        }
                    }

                    // Regla 3: Si todo lo que sigue a currentSymbol puede ser épsilon
                    // o si currentSymbol es el último elemento de la producción (A -> a B)
                    // Añadir FOLLOW(head) al FOLLOW(currentSymbol)
                    if (deriveEpsilon) {
                        for (const string& f : follows[head]) {
                            follows[currentSymbol].insert(f);
                        }
                    }

                    // Verificar si hubo cambios en este bucle
                    if (follows[currentSymbol].size() > beforeSize) {
                        changed = true;
                    }
                }
            }
        }
    }
}
