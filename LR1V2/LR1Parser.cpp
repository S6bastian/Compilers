#include "LR1Parser.h"

LR1Parser::LR1Parser(Grammar *g) : grammar(g) {
    buildCanonicalCollection();
    buildTable();
}

void LR1Parser::buildCanonicalCollection() {
    states.clear();
    LR1Item startItem;
    startItem.head = grammar->getStartSymbol(); 
    startItem.body = grammar->getProductions()[0].second; 
    startItem.dot = 0;
    startItem.lookahead = "$";

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

void LR1Parser::buildTable() {
    for (int i = 0; i < (int)states.size(); ++i) {
        for (auto const& [symbol, nextState] : transitions[i]) {
            parsingTable[i][symbol] = {SHIFT, nextState};
        }
        for (const auto& item : states[i]) {
            if (item.dot == (int)item.body.size()) {
                if (item.head == grammar->getStartSymbol() && item.lookahead == "$") {
                    parsingTable[i]["$"] = {ACCEPT, 0};
                } else {
                    auto prods = grammar->getProductions();
                    for (int k = 0; k < (int)prods.size(); ++k) {
                        if (prods[k].first == item.head && prods[k].second == item.body) {
                            parsingTable[i][item.lookahead] = {REDUCE, k};
                            break;
                        }
                    }
                }
            }
        }
    }
}

void LR1Parser::parse(const vector<string>& input) {
    vector<string> tokens = input;
    tokens.push_back("$");
    vector<int> stateStack = {0};
    vector<Node*> nodeStack;
    int cursor = 0;

    while (true) {
        int currentState = stateStack.back();
        string currentSymbol = tokens[cursor];
        if (parsingTable[currentState].find(currentSymbol) == parsingTable[currentState].end()) return;
        TableEntry entry = parsingTable[currentState][currentSymbol];

        if (entry.type == SHIFT) {
            stateStack.push_back(entry.id);
            nodeStack.push_back(new Node(currentSymbol));
            cursor++;
        } else if (entry.type == REDUCE) {
            auto prod = grammar->getProductions()[entry.id];
            Node* parent = new Node(prod.first);
            int numToPop = (prod.second.size() == 1 && prod.second[0] == grammar->getEmptySymbol()) ? 0 : prod.second.size();
            vector<Node*> children;
            for(int k = 0; k < numToPop; ++k) {
                children.push_back(nodeStack.back());
                nodeStack.pop_back();
                stateStack.pop_back();
            }
            for(int k = (int)children.size()-1; k >= 0; --k) parent->children.push_back(children[k]);
            if(numToPop == 0) parent->children.push_back(new Node(grammar->getEmptySymbol()));
            nodeStack.push_back(parent);
            stateStack.push_back(parsingTable[stateStack.back()][prod.first].id);
        } else if (entry.type == ACCEPT) {
            cout << "\nARBOL DE DERIVACION:" << endl;
            printTree(nodeStack.back(), 0);
            break;
        }
    }
}

void LR1Parser::printStates() const {
    for (size_t i = 0; i < states.size(); ++i) {
        cout << "ESTADO " << i << ":" << endl;
        for (const auto& item : states[i]) {
            cout << "  [" << item.head << " -> ";
            for (int k = 0; k < (int)item.body.size(); ++k) {
                if (k == item.dot) cout << ". ";
                cout << item.body[k] << " ";
            }
            if (item.dot == (int)item.body.size()) cout << ". ";
            cout << ", " << item.lookahead << "]" << endl;
        }
        cout << endl;
    }
}

void LR1Parser::printTable() const {
    set<string> symbols;
    for (auto const& [st, row] : parsingTable) for (auto const& [sym, e] : row) symbols.insert(sym);
    cout << "\nTABLA LR(1):" << endl;
    for (const auto& s : symbols) cout << setw(8) << s;
    cout << endl;
    for (int i = 0; i < (int)states.size(); ++i) {
        cout << i << ":";
        for (const auto& s : symbols) {
            if (parsingTable.at(i).count(s)) {
                TableEntry e = parsingTable.at(i).at(s);
                if (e.type == SHIFT) cout << setw(8) << "s" + to_string(e.id);
                else if (e.type == REDUCE) cout << setw(8) << "r" + to_string(e.id);
                else if (e.type == ACCEPT) cout << setw(8) << "acc";
            } else cout << setw(8) << "-";
        }
        cout << endl;
    }
}

void LR1Parser::printTree(Node* node, int depth) const {
    if (!node) return;
    for (int i = 0; i < depth; ++i) cout << (i == depth - 1 ? "|-- " : "    ");
    cout << node->value << endl;
    for (Node* child : node->children) printTree(child, depth + 1);
}