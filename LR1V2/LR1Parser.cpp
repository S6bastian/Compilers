#include "LR1Parser.h"

LR1Parser::LR1Parser(Grammar *g) {
    grammar = g;
    buildCanonicalCollection();
}

void LR1Parser::buildCanonicalCollection() {
    states.clear();
    
    // 1. Crear el ítem inicial del Estado 0: [S' -> . S, $]
    // Usamos el símbolo inicial aumentado de tu Gramática
    LR1Item startItem;
    startItem.head = grammar->getStartSymbol(); 
    // Aquí asumimos que la primera producción de grammar es S' -> S
    startItem.body = grammar->getProductions()[0].second; 
    startItem.dot = 0;
    startItem.lookahead = "$";

    // 2. Estado 0 = Clausura del ítem inicial
    states.push_back(closure({startItem}));

    // 3. Bucle de expansión (mientras se encuentren nuevos estados)
    for (size_t i = 0; i < states.size(); ++i) {
        // Buscamos qué símbolos tienen un punto antes en el estado actual
        set<string> symbols;
        for (const auto& item : states[i]) {
            if (item.dot < item.body.size()) {
                symbols.insert(item.body[item.dot]);
            }
        }

        // Para cada símbolo, calculamos a qué estado vamos
        for (const string& X : symbols) {
            State next = goTo(states[i], X);
            if (next.empty()) continue;

            // Revisamos si el conjunto de ítems (estado) ya existe
            int existingState = -1;
            for (size_t j = 0; j < states.size(); ++j) {
                if (states[j] == next) {
                    existingState = j;
                    break;
                }
            }

            if (existingState == -1) {
                states.push_back(next);
                transitions[i][X] = states.size() - 1;
            } else {
                transitions[i][X] = existingState;
            }
        }
    }
}

State LR1Parser::closure(State I) {
    State J = I;
    bool changed = true;

    while (changed) {
        changed = false;
        State currentJ = J; 

        for (const auto& item : currentJ) {
            // Buscamos ítems del tipo [A -> alpha . B beta, a]
            if (item.dot < item.body.size()) {
                string B = item.body[item.dot];
                
                if (grammar->isNonTerminal(B)) { 
                    // Extraemos beta (lo que sigue a B)
                    vector<string> beta;
                    for (size_t i = item.dot + 1; i < item.body.size(); ++i) {
                        beta.push_back(item.body[i]);
                    }

                    // El nuevo lookahead b es FIRST(beta + a)
                    set<string> lookaheads = computeFirstChain(beta, item.lookahead);

                    // Añadimos B -> . gamma, b
                    for (const auto& prod : grammar->getProductions()) {
                        if (prod.first == B) {
                            for (const string& b : lookaheads) {
                                LR1Item newItem{B, prod.second, 0, b};
                                if (J.find(newItem) == J.end()) {
                                    J.insert(newItem);
                                    changed = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return J;
}

State LR1Parser::goTo(const State& I, const string& X) {
    State J;
    for (const auto& item : I) {
        if (item.dot < item.body.size() && item.body[item.dot] == X) {
            LR1Item movedItem = item;
            movedItem.dot++;
            J.insert(movedItem);
        }
    }
    return closure(J);
}

set<string> LR1Parser::computeFirstChain(vector<string> beta, string lookahead) {
    set<string> result;
    bool allCanBeEmpty = true;

    for (const string& symbol : beta) {
        bool hasEpsilon = false;
        // Accedemos a los FIRST calculados previamente en Grammar
        for (const string& f : grammar->getFirsts(symbol)) {
            if (f == grammar->getEmptySymbol()) hasEpsilon = true;
            else result.insert(f);
        }
        if (!hasEpsilon) {
            allCanBeEmpty = false;
            break;
        }
    }

    if (allCanBeEmpty) result.insert(lookahead);
    return result;
}

void LR1Parser::printStates() const {
    for (size_t i = 0; i < states.size(); ++i) {
        cout << "ESTADO " << i << ":" << endl;
        for (const auto& item : states[i]) {
            cout << "  [" << item.head << " -> ";
            for (int k = 0; k < item.body.size(); ++k) {
                if (k == item.dot) cout << ". ";
                cout << item.body[k] << " ";
            }
            if (item.dot == item.body.size()) cout << ". ";
            cout << ", " << item.lookahead << "]" << endl;
        }
        cout << endl;
    }
}