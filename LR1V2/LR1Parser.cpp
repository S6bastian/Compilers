#include "LR1Parser.h"

//********************************************************************************************************************
//OVERLOAD ********************************************************************************************************
//********************************************************************************************************************


bool operator==(const State& a, const State& b) {
    if (a.size() != b.size()) return false;
    
    vector<LR1Item> sortedA = a;
    vector<LR1Item> sortedB = b;
    sort(sortedA.begin(), sortedA.end());
    sort(sortedB.begin(), sortedB.end());
    
    for (size_t i = 0; i < sortedA.size(); ++i) {
        if (!(sortedA[i] == sortedB[i])) return false;
    }
    return true;
}


//********************************************************************************************************************
//CONSTRUCTOR ********************************************************************************************************
//********************************************************************************************************************

LR1Parser::LR1Parser(Grammar *g) {
    grammar = g;
    buildStates();
    buildTable();
    
    parseTreeRoot = nullptr;  

    exportCanonicalCollectionToJSON("canonical_collection.json");
    exportTableToJSON("lr1_table.json");
}




//********************************************************************************************************************
//PUBLIC *************************************************************************************************************
//********************************************************************************************************************



void LR1Parser::buildStates() {
    states.clear();
    transitions.clear();
    
    LR1Item startItem;
    startItem.head = grammar->getStartSymbol();
    startItem.body = grammar->getProductions()[0].second;
    startItem.dot = 0;
    startItem.lookahead = {"$"};
    
    states.push_back(closure({startItem}));
    
    cout << "Building LR(1) states...\n";
    
    for (size_t i = 0; i < states.size(); ++i) {
        State currentState = states[i];
        set<string> symbols;
        for (const auto& item : currentState) {
            if(item.body[0] == grammar->getEmptySymbol()) continue;
            if (item.dot < (int)item.body.size()) {
                symbols.insert(item.body[item.dot]);
            }
        }

        for (const string& symbol : symbols) {
            State nextState = goTo(currentState, symbol);
            

            if (nextState.empty()) continue;
            
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
            
            if (existingStateIndex == -1) {
                states.push_back(nextState);
                transitions[i][symbol] = states.size() - 1;
            }
            else {
                transitions[i][symbol] = existingStateIndex;
            }
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
            
            for (size_t j = 0; j < item.body.size(); ++j) {
                if (j == (size_t)item.dot) cout << ". ";
                cout << item.body[j] << " ";
            }
            if (item.dot == (int)item.body.size()) cout << ".";
            
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


void LR1Parser::buildTable() {
    cout << "\nBuilding LR(1) Parse Table...\n";
    
    for (size_t i = 0; i < states.size(); ++i) {
        const State& state = states[i];
        
        if (transitions.find(i) != transitions.end()) {
            for (const auto& [symbol, target] : transitions.at(i)) {
                if (grammar->isTerminal(symbol)) { //shift
                    actionTable[i][symbol] = "s" + to_string(target);
                } else {    //goto
                    gotoTable[i][symbol] = target;
                }
            }
        }
        
        for (const LR1Item& item : state) { //reduction
            if (item.dot == (int)item.body.size() || item.body[0] == grammar->getEmptySymbol()) {

                if (item.head == grammar->getStartSymbol() &&   //acc
                    item.body.size() == 1 && 
                    item.lookahead.count("$")) {
                    actionTable[i]["$"] = "acc";
                    continue;
                }
                
                int prodIndex = -1;
                const auto& productions = grammar->getProductions();
                for (size_t p = 0; p < productions.size(); ++p) {
                    if (productions[p].first == item.head && 
                        productions[p].second == item.body) {
                        prodIndex = p;
                        break;
                    }
                }
                
                if (prodIndex == -1) {
                    cerr << "Error: Production not found for reduction\n";
                    continue;
                }
                
                for (const string& la : item.lookahead) { //apply reduction to lookaheads
                    if (actionTable[i].find(la) != actionTable[i].end()){   // conflict
                        cout << "  Conflict in state " << i << " on symbol '" << la 
                             << "': " << actionTable[i][la] << " vs r" << prodIndex << "\n";
                    }
                    else{
                        actionTable[i][la] = "r" + to_string(prodIndex);
                    }
                }
            }
        }
    }
    
    cout << "Table built successfully.\n";
    printTable();
}

void LR1Parser::printTable() const {
    if (actionTable.empty() && gotoTable.empty()) {
        cout << "Table not built yet!\n";
        return;
    }
    
    set<string> terminals;
    set<string> nonTerminals;
    
    for (const auto& [state, actions] : actionTable) {
        for (const auto& [symbol, action] : actions) {
            if (symbol != "$" && !grammar->isTerminal(symbol)) {
                nonTerminals.insert(symbol);
            } else {
                terminals.insert(symbol);
            }
        }
    }
    
    for (const auto& [state, gotos] : gotoTable) {
        for (const auto& [symbol, target] : gotos) {
            nonTerminals.insert(symbol);
        }
    }
    
    terminals.insert("$");
    
    cout << "\n=== LR(1) PARSE TABLE ===\n\n";
    
    // header
    cout << setw(6) << "State";
    for (const string& t : terminals) {
        cout << setw(8) << t;
    }
    for (const string& nt : nonTerminals) {
        cout << setw(8) << nt;
    }
    cout << "\n";
    
    cout << string(6 + 8 * (terminals.size() + nonTerminals.size()), '-') << "\n";
    
    // state x state
    for (size_t i = 0; i < states.size(); ++i) {
        cout << setw(6) << i;
        
        // action
        if (actionTable.find(i) != actionTable.end()) {
            for (const string& t : terminals) {
                auto it = actionTable.at(i).find(t);
                if (it != actionTable.at(i).end()) {
                    cout << setw(8) << it->second;
                } else {
                    cout << setw(8) << "";
                }
            }
        } else {
            for (size_t j = 0; j < terminals.size(); ++j) {
                cout << setw(8) << "";
            }
        }
        
        // goto
        if (gotoTable.find(i) != gotoTable.end()) {
            for (const string& nt : nonTerminals) {
                auto it = gotoTable.at(i).find(nt);
                if (it != gotoTable.at(i).end()) {
                    cout << setw(8) << it->second;
                } else {
                    cout << setw(8) << "";
                }
            }
        } else {
            for (size_t j = 0; j < nonTerminals.size(); ++j) {
                cout << setw(8) << "";
            }
        }
        
        cout << "\n";
    }
    
    cout << "\n=== PRODUCTIONS ===\n";
    const auto& productions = grammar->getProductions();
    for (size_t i = 0; i < productions.size(); ++i) {
        cout << i << ": " << productions[i].first << " -> ";
        if (productions[i].second.empty()) {
            cout << grammar->getEmptySymbol();
        } else {
            for (const string& s : productions[i].second) {
                cout << s << " ";
            }
        }
        cout << "\n";
    }
}

// export

void LR1Parser::exportCanonicalCollectionToJSON(const string& filename) const {
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Error: no se pudo abrir " << filename << " para escritura.\n";
        return;
    }

    out << "{\n  \"states\": [\n";
    for (size_t i = 0; i < states.size(); ++i) {
        out << "    {\n      \"id\": " << i << ",\n      \"items\": [\n";
        const State& state = states[i];
        for (size_t j = 0; j < state.size(); ++j) {
            const LR1Item& item = state[j];
            out << "        {\n";
            out << "          \"head\": \"" << item.head << "\",\n";
            out << "          \"body\": [";
            for (size_t k = 0; k < item.body.size(); ++k) {
                out << "\"" << item.body[k] << "\"";
                if (k != item.body.size() - 1) out << ", ";
            }
            out << "],\n";
            out << "          \"dot\": " << item.dot << ",\n";
            out << "          \"lookahead\": [";
            size_t laIdx = 0;
            for (const string& la : item.lookahead) {
                out << "\"" << la << "\"";
                if (++laIdx != item.lookahead.size()) out << ", ";
            }
            out << "]\n";
            out << "        }";
            if (j != state.size() - 1) out << ",";
            out << "\n";
        }
        out << "      ]\n";
        out << "    }";
        if (i != states.size() - 1) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
    out.close();
    cout << "Exported CC " << filename << "\n";
}

void LR1Parser::exportTableToJSON(const string& filename) const {
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Error: no se pudo abrir " << filename << " para escritura.\n";
        return;
    }

    out << "{\n  \"states\": [\n";
    for (size_t i = 0; i < states.size(); ++i) {
        out << "    {\n      \"id\": " << i << ",\n";

        // --- ACTION ---
        out << "      \"action\": {";
        if (actionTable.find(i) != actionTable.end()) {
            const auto& actions = actionTable.at(i);
            size_t count = 0;
            for (const auto& [symbol, action] : actions) {
                out << "\"" << symbol << "\": \"" << action << "\"";
                if (++count != actions.size()) out << ", ";
            }
        }
        out << "},\n";

        // --- GOTO ---
        out << "      \"goto\": {";
        if (gotoTable.find(i) != gotoTable.end()) {
            const auto& gotos = gotoTable.at(i);
            size_t count = 0;
            for (const auto& [symbol, target] : gotos) {
                out << "\"" << symbol << "\": " << target;
                if (++count != gotos.size()) out << ", ";
            }
        }
        out << "}\n";

        out << "    }";
        if (i != states.size() - 1) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
    out.close();

    cout << "Table ACTION/GOTO exported as " << filename << endl;
}


void LR1Parser::exportTraceToJSON(const string& filename) const {
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Error: no se pudo abrir " << filename << " para escritura.\n";
        return;
    }
    
    out << "{\n  \"trace\": [\n";
    for (size_t i = 0; i < traceTable.size(); ++i) {
        out << "    {\n";
        out << "      \"step\": \"" << traceTable[i][0] << "\",\n";
        out << "      \"stack\": \"" << traceTable[i][1] << "\",\n";
        out << "      \"input\": \"" << traceTable[i][2] << "\",\n";
        out << "      \"action\": \"" << traceTable[i][3] << "\"\n";
        out << "    }";
        if (i != traceTable.size() - 1) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
    out.close();
    
    cout << "Trace exported to " << filename << endl;
}


//********************************************************************************************************************
//PRIVATE ************************************************************************************************************
//********************************************************************************************************************


set<string> LR1Parser::computeLookahead(LR1Item item){
    if(item.dot + 1 >= (int)item.body.size()) return item.lookahead;

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

            if (current.dot < (int)current.body.size()) {
                string nextSymbol = current.body[current.dot];
                
                if (grammar->isNonTerminal(nextSymbol)) {
                    set<string> lookaheads = computeLookahead(current);


                    // DEBUG
                    // cout << "  Expanding " << nextSymbol << " from item: ";
                    // for(auto s : current.body) cout << s << " ";
                    // cout << ", lookaheads: { ";
                    // for(auto l : lookaheads) cout << l << " ";
                    // cout << "}\n";

                    
                    for (const auto& prod : grammar->getProductions()) {
                        if (prod.first == nextSymbol) {
                            LR1Item newItem{prod.first, prod.second, 0, lookaheads};
                            pending.push_back(newItem);
                        }
                    }
                }
            }
        } else {

            size_t oldSize = state[existingIndex].lookahead.size();
            state[existingIndex].lookahead.insert(current.lookahead.begin(), current.lookahead.end());
            
            if (state[existingIndex].lookahead.size() > oldSize && 
                state[existingIndex].dot < (int)state[existingIndex].body.size()) {

                pending.push_back(state[existingIndex]);
            }
        }
    }
    
    return state;
}


State LR1Parser::goTo(const State& state, const string& symbol) {
    vector<LR1Item> kernels;
    
    for (const auto& item : state) {
        if (item.dot < (int)item.body.size() && item.body[item.dot] == symbol) {
            LR1Item newItem = item;
            newItem.dot++;  
            kernels.push_back(newItem);
        }
    }
    
    if (kernels.empty()) {
        return State();  
    }
    
    return closure(kernels);
}





vector<string> LR1Parser::tokenize(const string& input) {
    vector<string> tokens;
    istringstream iss(input);
    string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    tokens.push_back("$"); 
    return tokens;
}

bool LR1Parser::parse(const string& input) {
    traceTable.clear();

    vector<string> tokens = tokenize(input);
    
    vector<int> stateStack;
    vector<TreeNode*> nodeStack;
    
    stateStack.push_back(0);
    size_t inputPos = 0;
    int stepCount = 0;
    const int MAX_STEPS = 100;
    
    cout << "\n=== PARSING TRACE ===\n";
    cout << left << setw(6) << "Step" 
         << setw(30) << "Stack" 
         << setw(25) << "Input" 
         << setw(10) << "Action" << "\n";
    cout << string(71, '-') << "\n";
    
    while (stepCount < MAX_STEPS) {
        stepCount++;
        vector<string> row;
        
        int currentState = stateStack.back();
        string currentToken = tokens[inputPos];
        
        string stackStr = "";
        for (size_t i = 0; i < stateStack.size(); i++) {
            if (i > 0) stackStr += " ";
            stackStr += to_string(stateStack[i]);
            if (i < nodeStack.size() && nodeStack[i] != nullptr) {
                stackStr += " " + nodeStack[i]->symbol;
            }
        }
        
        // Construir representación del input restante
        string inputStr = "";
        for (size_t i = inputPos; i < tokens.size(); i++) {
            if (i > inputPos) inputStr += " ";
            inputStr += tokens[i];
        }

        row.push_back(to_string(stepCount));
        row.push_back(stackStr);
        row.push_back(inputStr);


        // Imprimir paso actual
        cout << left << setw(6) << stepCount
             << setw(30) << stackStr
             << setw(25) << inputStr;

        
        // Buscar acción en la tabla
        if (actionTable.find(currentState) == actionTable.end() || 
            actionTable[currentState].find(currentToken) == actionTable[currentState].end()) {
            cout << setw(10) << "ERROR" << "\n";
            cout << "Error: No action for state " << currentState << " and token '" << currentToken << "'\n";
            
            row.push_back("ERROR");
            traceTable.push_back(row);

            return false;
        }
        
        string action = actionTable[currentState][currentToken];
        cout << setw(10) << action;

        row.push_back(action);
        
        if (action == "acc") {
            cout << "\n=== Input accepted! ===\n";
            if (!nodeStack.empty()) {
                cout << "\n=== PARSE TREE ===\n";
                printParseTree(nodeStack.back());
            }

            traceTable.push_back(row);

            return true;
        }
        else if (action[0] == 's') {
            // Shift
            int nextState = stoi(action.substr(1));
            
            // Crear nodo hoja para el terminal
            TreeNode* leaf = new TreeNode(currentToken);
            nodeStack.push_back(leaf);
            stateStack.push_back(nextState);

            traceTable.push_back(row);
            
            // Consumir token
            inputPos++;
            
            cout << "  (Shift to state " << nextState << ")\n";
        }
        else if (action[0] == 'r') {
            // Reduce
            int prodIndex = stoi(action.substr(1));
            const auto& productions = grammar->getProductions();
            const auto& [head, body] = productions[prodIndex];
            
            TreeNode* newNode = new TreeNode(head);
            
            // Pop estados y nodos según el tamaño del cuerpo
            int popCount = body.size();
            if (body.size() == 1 && body[0] == grammar->getEmptySymbol()) {
                // Producción epsilon
                popCount = 0;
                TreeNode* epsilonNode = new TreeNode(grammar->getEmptySymbol());
                newNode->children.push_back(epsilonNode);
            } else {
                // Insertar hijos en orden inverso
                for (int i = popCount - 1; i >= 0; i--) {
                    if (!nodeStack.empty()) {
                        newNode->children.insert(newNode->children.begin(), nodeStack.back());
                        nodeStack.pop_back();
                    }
                    if (!stateStack.empty()) {
                        stateStack.pop_back();
                    }
                }
            }
            
            nodeStack.push_back(newNode);
            
            // Calcular nuevo estado (GOTO)
            int topState = stateStack.back();
            if (gotoTable.find(topState) == gotoTable.end() || 
                gotoTable[topState].find(head) == gotoTable[topState].end()) {
                cout << "Error: No goto for state " << topState << " and non-terminal '" << head << "'\n";
                return false;
            }
            
            int nextState = gotoTable[topState][head];
            stateStack.push_back(nextState);
            
            cout << "  (Reduce by " << head << " -> ";
            if (body.empty() || (body.size() == 1 && body[0] == grammar->getEmptySymbol())) {
                cout << grammar->getEmptySymbol();
            } else {
                for (const string& s : body) cout << s << " ";
            }
            cout << ", goto " << nextState << ")\n";
            
            traceTable.push_back(row);
        }
    }
    
    cout << "Error: Maximum steps exceeded\n";
    return false;
}

void LR1Parser::printParseTrace(const string& input) {
    parse(input);
    exportTraceToJSON("trace_table.json");
}

void LR1Parser::printParseTree(TreeNode* node, int depth) const {
    if (!node) return;
    
    // Indentación
    for (int i = 0; i < depth; i++) {
        cout << "  ";
    }
    
    cout << node->symbol;
    
    if (!node->children.empty()) {
        cout << " ->";
        for (TreeNode* child : node->children) {
            cout << " " << child->symbol;
        }
        cout << "\n";
        
        for (TreeNode* child : node->children) {
            printParseTree(child, depth + 1);
        }
    } else {
        cout << "\n";
    }
}

void LR1Parser::deleteTree(TreeNode* node) {
    if (node) {
        for (TreeNode* child : node->children) {
            deleteTree(child);
        }
        delete node;
    }
}