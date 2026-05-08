#include "LR1Parser.h"

LR1Parser::LR1Parser(Grammar *g) {
    grammar = g;
    buildCanonicalCollection();
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
    if(item.dot + 1 == item.body.size()) return item.lookahead;

    //string dotSymbol = item.body[item.dot];
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
        return lookaheads;
    }
    else
        return {nextSymbol};

}

vector<LR1Item> LR1Parser::closureXd(LR1Item kernel) {
    queue<pair<string,vector<string>>> next;    // new head and vector of lookahead
    vector<LR1Item> state;
    //vector<pair<string,vector<string>>> state;

    

    if(grammar->isTerminal(kernel.body[0])) return {kernel};      // REVISAR!!!!!!!!!!!!!!!!!!!!!!!!!
    
    if(kernel.dot < kernel.body.size()){
        for(size_t cDot = kernel.dot; cDot < kernel.body.size(); cDot++){

        }
    }
    next.push({kernel.body[kernel.dot],})
    while(!next.empty()){

    }
}


State LR1Parser::closure(State I) {
    State J = I;
    bool changed = true;

    while (changed) {
        changed = false;
        State currentJ = J; 

        for (const auto& item : currentJ) {
           
            if (item.dot < item.body.size()) {
                string B = item.body[item.dot];
                
                if (grammar->isNonTerminal(B)) { 
                    vector<string> beta;
                    for (size_t i = item.dot + 1; i < item.body.size(); ++i) {
                        beta.push_back(item.body[i]);
                    }


                    set<string> lookaheads = computeFirstChain(beta, item.lookahead);


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