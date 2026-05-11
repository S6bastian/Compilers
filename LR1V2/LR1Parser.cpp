#include "LR1Parser.h"

bool operator==(const State& a, const State& b) {
    if (a.size() != b.size()) return false;
    
    for (size_t i = 0; i < a.size(); ++i) {
        if (!(a[i] == b[i])) return false;
    }
    return true;
}



LR1Parser::LR1Parser(Grammar *g) {
    grammar = g;
    buildStates();
}


void LR1Parser::buildStates() {
    states.clear();
    transitions.clear();
    
    // Crear el item inicial de la gramática aumentada
    LR1Item startItem;
    startItem.head = grammar->getStartSymbol();
    startItem.body = grammar->getProductions()[0].second;
    startItem.dot = 0;
    startItem.lookahead = {"$"};
    
    // Estado inicial = closure del item inicial
    states.push_back(closure({startItem}));
    
    cout << "Building LR(1) states...\n";
    
    // Construir el resto de estados
    for (size_t i = 0; i < states.size(); ++i) {
        const State& currentState = states[i];
        cout << "i:    " << i << "\n";
        // Recopilar todos los símbolos que aparecen después del punto
        set<string> symbols;
        for (const auto& item : currentState) {
            if (item.dot < (int)item.body.size()) {
                symbols.insert(item.body[item.dot]);
            }
        }

        for(auto& symbol : symbols){
            cout << symbol << " ";
        }
        cout << "\n";
        
        // Para cada símbolo, calcular el estado goto
        for (const string& symbol : symbols) {
            State nextState = goTo(currentState, symbol);
            
            cout << "GOTO USED\n";

            if (nextState.empty()) continue;
            
            // Verificar si este estado ya existe
            int existingStateIndex = -1;
            for (size_t j = 0; j < states.size(); ++j) {
                if (states[j].size() == nextState.size()) {
                    bool isEqual = true;
                    for (size_t k = 0; k < states[j].size(); ++k) {
                        if (!(states[j][k] == nextState[k])) {
                            isEqual = false;
                            break;
                        }
                    }
                    if (isEqual) {
                        existingStateIndex = j;
                        break;
                    }
                }
            }
            cout << "VERIFICATION DONE\n";
            // Agregar nuevo estado si no existe
            if (existingStateIndex == -1) {
                states.push_back(nextState);
                transitions[i][symbol] = states.size() - 1;
                cout << "  Created new state " << states.size() - 1 
                     << " from state " << i << " with symbol '" << symbol << "'\n";
            } else {
                transitions[i][symbol] = existingStateIndex;
                cout << "  Transition from state " << i << " to existing state " 
                     << existingStateIndex << " with symbol '" << symbol << "'\n";
            }
            cout << "****************************************\n";
            printStates();
            cout << "****************************************\n";
        }
    }
    
    cout << "\nTotal states built: " << states.size() << "\n";
    printStates();
}

void LR1Parser::printStates() const {
    cout << "\n=== LR(1) AUTOMATON ===\n";
    cout << "Total states: " << states.size() << "\n\n";
    
    for (size_t i = 0; i < states.size(); ++i) {
        cout << "State " << i << ":\n";
        for (const auto& item : states[i]) {
            cout << "  " << item.head << " -> ";
            
            // Imprimir el cuerpo con el punto
            for (size_t j = 0; j < item.body.size(); ++j) {
                if (j == (size_t)item.dot) cout << ". ";
                cout << item.body[j] << " ";
            }
            if (item.dot == (int)item.body.size()) cout << ".";
            
            // Imprimir lookaheads
            cout << ", { ";
            for (auto it = item.lookahead.begin(); it != item.lookahead.end(); ++it) {
                cout << *it;
                if (next(it) != item.lookahead.end()) cout << ", ";
            }
            cout << " }\n";
        }
        
        // Imprimir transiciones
        if (transitions.find(i) != transitions.end() && !transitions.at(i).empty()) {
            cout << "  Transitions:\n";
            for (const auto& [symbol, target] : transitions.at(i)) {
                cout << "    " << symbol << " -> State " << target << "\n";
            }
        }
        cout << "\n";
    }
    cout << "========================\n";
}



set<string> LR1Parser::computeLookahead(LR1Item item){
    if(item.dot + 1 >= item.body.size()) return item.lookahead;

    string nextSymbol = item.body[item.dot + 1];
    set<string> lookaheads;

    if(grammar->isNonTerminal(nextSymbol)){
        set<string> candidates = grammar->getFirsts(nextSymbol);
        for(const string& candidate : candidates){
            if(candidate == grammar->getEmptySymbol()){
                LR1Item tempItem = item;
                tempItem.dot++;
                set<string> rest = computeLookahead(tempItem);
                lookaheads.insert(rest.begin(), rest.end());
            }
            else lookaheads.insert(candidate);
        }
    }
    else
        lookaheads.insert(nextSymbol);

    return lookaheads;

}

State LR1Parser::closure(vector<LR1Item> kernels) {
    vector<LR1Item> state;
    deque<LR1Item> pending;
    
    for (const auto& item : kernels) {
        pending.push_back(item);
    }
    
    auto findItemByKey = [](const vector<LR1Item>& items, const LR1Item& target) -> int {
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].head == target.head && 
                items[i].body == target.body && 
                items[i].dot == target.dot) {
                return i;
            }
        }
        return -1;
    };
    
    while (!pending.empty()) {
        LR1Item current = pending.front();
        pending.pop_front();
        
        int existingIndex = findItemByKey(state, current);
        
        if (existingIndex == -1) {
            state.push_back(current);
            
            // Solo generamos nuevos items si este es realmente nuevo
            if (current.dot < current.body.size()) {
                string nextSymbol = current.body[current.dot];
                
                if (grammar->isNonTerminal(nextSymbol)) {
                    set<string> lookaheads = computeLookahead(current);


                    // DEBUG
                    cout << "  Expanding " << nextSymbol << " from item: ";
                    for(auto s : current.body) cout << s << " ";
                    cout << ", lookaheads: { ";
                    for(auto l : lookaheads) cout << l << " ";
                    cout << "}\n";

                    
                    for (const auto& prod : grammar->getProductions()) {
                        if (prod.first == nextSymbol) {
                            LR1Item newItem{prod.first, prod.second, 0, lookaheads};
                            pending.push_back(newItem);
                        }
                    }
                }
            }
        } else {
            // Unir lookaheads
            size_t oldSize = state[existingIndex].lookahead.size();
            state[existingIndex].lookahead.insert(current.lookahead.begin(), current.lookahead.end());
            
            // Si crecieron los lookaheads, regenerar pendientes para este item
            if (state[existingIndex].lookahead.size() > oldSize && 
                state[existingIndex].dot < state[existingIndex].body.size()) {
                // Re-procesar este item con los nuevos lookaheads
                
                pending.push_back(state[existingIndex]);
            }
        }
    }
    
    return state;
}


State LR1Parser::goTo(const State& state, const string& symbol) {
    vector<LR1Item> kernels;
    
    // Buscar todos los items donde el símbolo después del punto es 'symbol'
    for (const auto& item : state) {
        if (item.dot < (int)item.body.size() && item.body[item.dot] == symbol) {
            LR1Item newItem = item;
            newItem.dot++;  // Avanzar el punto
            kernels.push_back(newItem);
        }
    }
    
    if (kernels.empty()) {
        return State();  // Estado vacío
    }
    
    // Aplicar closure a los kernels
    return closure(kernels);
}
