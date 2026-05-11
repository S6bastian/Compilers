#include "LR1Parser.h"

LR1Parser::LR1Parser(Grammar *g) {
    grammar = g;
    buildStates();
}

void LR1Parser::buildStates() {
    states.clear();
    
    LR1Item startItem;
    startItem.head = grammar->getStartSymbol(); 
    startItem.body = grammar->getProductions()[0].second; 
    startItem.dot = 0;
    startItem.lookahead = {"$"};

    

    states.push_back(closure({startItem}));
    
    State test = states[0];
    cout << "PARSER TEST!!!!!!!!!!!!!!!!! \n";
    for(LR1Item item : test){
        cout << item.head << " -> "; 
        for(auto body : item.body) cout << body << " ";
        cout << ", { ";
        for(auto l : item.lookahead) cout << l << " ";
        cout << "}    dot = " << item.dot << "\n";
    }

    /*
    for (size_t i = 0; i < states.size(); ++i) {
        set<string> symbols;
        for (const auto& item : states[i]) {
            if (item.dot < item.body.size()) {
                symbols.insert(item.body[item.dot]);
            }
        }

        
        for (const string& X : symbols) {
            State next = goTo(states[i], X);
            if (next.empty()) continue;

            
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

    */
}

set<string> LR1Parser::computeLookahead(LR1Item item){
    if(item.dot + 1 >= item.body.size()) return item.lookahead;

    string nextSymbol = item.body[item.dot + 1];
    set<string> lookaheads;

    if(grammar->isNonTerminal(nextSymbol)){
        set<string> candidates = grammar->getFirsts(nextSymbol);
        for(const string& candidate : candidates){
            if(candidate == grammar->getEmptySymbol()){
                item.dot++;
                lookaheads.merge(computeLookahead(item));
            }
            else lookaheads.insert(candidate);
        }
    }
    else
        lookaheads.insert(nextSymbol);

    return lookaheads;

}

vector<LR1Item> LR1Parser::closure(vector<LR1Item> kernels) {
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

