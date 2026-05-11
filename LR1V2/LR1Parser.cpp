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
            lookaheads.insert(candidate);
        }
    }
    else
        lookaheads.insert(nextSymbol);

    return lookaheads;

}

vector<LR1Item> LR1Parser::closure(vector<LR1Item> kernels) {
    vector<LR1Item> state;
    queue<LR1Item> pending;
    
    for (const auto& item : kernels) {
        pending.push(item);
    }
    
    while (!pending.empty()) {
        LR1Item current = pending.front();
        pending.pop();
        
        bool alreadyExists = false;
        for (const auto& existing : state) {
            if (existing.head == current.head && 
                existing.body == current.body && 
                existing.dot == current.dot) {
                alreadyExists = true;
                break;
            }
        }
        
        if (!alreadyExists) {
            state.push_back(current);
        }
        
        if (current.dot >= current.body.size()) {
            continue;
        }
        
        string nextSymbol = current.body[current.dot];
        
        if (grammar->isNonTerminal(nextSymbol)) {
            for (const auto& prod : grammar->getProductions()) {
                if (prod.first == nextSymbol) {
                    set<string> lookaheads = computeLookahead(current);
                    
                    LR1Item newItem;
                    newItem.head = prod.first;
                    newItem.body = prod.second;
                    newItem.dot = 0;
                    newItem.lookahead = lookaheads;
                    
                    // Verificar si ya tenemos este item (por head, body, dot)
                    bool itemExists = false;
                    for (const auto& existing : state) {
                        if (existing.head == newItem.head && 
                            existing.body == newItem.body && 
                            existing.dot == newItem.dot) {
                            itemExists = true;
                            break;
                        }
                    }
                    
                    if (!itemExists) {
                        pending.push(newItem);
                    }
                }
            }
        }
    }
    
    return state;
}

