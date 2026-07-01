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

    //exportFirstSetsToJSON("first_sets.json");
    //exportCanonicalCollectionToJSON("canonical_collection.json");
    //exportTableToJSON("lr1_table.json");
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
            cout << // Retroceder en la pila
                grammar->getEmptySymbol();
        } else {
            for (const string& s : productions[i].second) {
                cout << s << " ";
            }
        }
        cout << "\n";
    }
}

// export

void LR1Parser::exportFirstSetsToJSON(const string& filename) const {
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Error: no se pudo abrir " << filename << " para escritura.\n";
        return;
    }

    out << "{\n  \"firstSets\": {\n";

    const auto& productions = grammar->getProductions();
    set<string> nonTerminals;
    for (const auto& prod : productions) {
        nonTerminals.insert(prod.first);
    }

    int ntCount = 0;
    for (const string& nt : nonTerminals) {
        out << "    \"" << nt << "\": [";
        set<string> firstSet = grammar->getFirsts(nt);
        int laCount = 0;
        for (const string& sym : firstSet) {
            out << "\"" << sym << "\"";
            if (++laCount < (int)firstSet.size()) out << ", ";
        }
        out << "]";
        if (++ntCount < (int)nonTerminals.size()) out << ",";
        out << "\n";
    }

    out << "  }\n}\n";
    out.close();

    cout << "FIRST sets exported to " << filename << endl;
}

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


void LR1Parser::exportParseTreeToJSON(const string& filename) const {
    if (!parseTreeRoot) {
        cerr << "No parse tree available to export.\n";
        return;
    }

    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Error: couldn't open " << filename << " for writing.\n";
        return;
    }

    out << "{\n  \"parseTree\": ";
    exportTreeNodeToJSON(out, parseTreeRoot, 2);
    out << "\n}\n";
    out.close();

    cout << "Parse tree exported to " << filename << endl;
}

void LR1Parser::exportTreeNodeToJSON(ofstream& out, const TreeNode* node, int depth) const {
    if (!node) {
        out << "null";
        return;
    }

    string indent(depth, ' ');
    string childIndent(depth + 2, ' ');

    out << "{\n";
    out << childIndent << "\"symbol\": \"" << node->symbol << "\"";

    if (!node->children.empty()) {
        out << ",\n" << childIndent << "\"children\": [\n";
        for (size_t i = 0; i < node->children.size(); ++i) {
            exportTreeNodeToJSON(out, node->children[i], depth + 4);
            if (i != node->children.size() - 1) out << ",";
            out << "\n";
        }
        out << childIndent << "]";
    }

    out << "\n" << indent << "}";
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

    auto findItemByKey = [](const vector<LR1Item>& items, const LR1Item& target) -> int {
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].head == target.head &&
                items[i].body == target.body &&
                items[i].dot  == target.dot)
                return (int)i;
        }
        return -1;
    };

    auto addItem = [&](LR1Item item) -> bool {
        int idx = findItemByKey(state, item);
        if (idx == -1) {
            state.push_back(item);
            return true;
        }
        size_t before = state[idx].lookahead.size();
        state[idx].lookahead.insert(item.lookahead.begin(), item.lookahead.end());
        return state[idx].lookahead.size() > before;
    };

    deque<LR1Item> workList;
    for (const auto& k : kernels) {
        if (addItem(k)) workList.push_back(k);
    }

    while (!workList.empty()) {
        LR1Item current = workList.front();
        workList.pop_front();

        if (current.dot >= (int)current.body.size()) continue;
        if (current.body[0] == grammar->getEmptySymbol()) continue;

        string nextSymbol = current.body[current.dot];
        if (!grammar->isNonTerminal(nextSymbol)) continue;

        set<string> childLookaheads = computeLookahead(current);

        for (const auto& prod : grammar->getProductions()) {
            if (prod.first != nextSymbol) continue;

            LR1Item child{prod.first, prod.second, 0, childLookaheads};

            if (addItem(child)) {
                int idx = findItemByKey(state, child);
                LR1Item merged = state[idx];
                workList.push_back(merged);
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
    if (parseTreeRoot) {
        deleteTree(parseTreeRoot);
        parseTreeRoot = nullptr;
    }


    vector<string> tokens = tokenize(input);

    vector<int> stateStack;
    vector<TreeNode*> nodeStack;

    stateStack.push_back(0);
    size_t inputPos = 0;
    int stepCount = 0;
    int errorCount = 0;
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

        string inputStr = "";
        for (size_t i = inputPos; i < tokens.size(); i++) {
            if (i > inputPos) inputStr += " ";
            inputStr += tokens[i];
        }

        row.push_back(to_string(stepCount));
        row.push_back(stackStr);
        row.push_back(inputStr);


        cout << left << setw(6) << stepCount
             << setw(30) << stackStr
             << setw(25) << inputStr;


            if (actionTable.find(currentState) == actionTable.end() ||
                actionTable[currentState].find(currentToken) == actionTable[currentState].end()) {

                cout << setw(10) << "ERROR" << "\n";
                cout << "  [PANIC MODE] Error Sintactico en estado " << currentState
                    << ": token inesperado '" << currentToken << "'\n";
                errorCount++;

                row.push_back("ERROR");
                traceTable.push_back(row);

                bool recovered = false;
                string syncNonTerminal = "";
                int targetState = -1;

                cout << "  [PANIC MODE] Buscando estado ancla en la pila desapilando...\n";



                while (!stateStack.empty()) {
                    int topState = stateStack.back();


                    for (const string& nt : {"E"}) { // "F", "T", "E"
                        if (grammar->isNonTerminal(nt) &&
                            gotoTable.find(topState) != gotoTable.end() &&
                            gotoTable[topState].find(nt) != gotoTable[topState].end()) {

                            syncNonTerminal = nt;
                            targetState = gotoTable[topState][nt];
                            recovered = true;
                            break;
                        }
                    }

                    if (recovered) {
                        cout << "  [PANIC MODE] Estado ancla encontrado: Estado " << topState
                            << ". Sincronizando con No-Terminal '" << syncNonTerminal
                            << "' -> GOTO nos lleva al Estado " << targetState << "\n";
                        break;
                    }


                    stateStack.pop_back();
                    if (!nodeStack.empty()) {
                        delete nodeStack.back();
                        nodeStack.pop_back();
                    }
                }

                if (!recovered || stateStack.empty()) {
                    cout << "  [PANIC MODE] Error fatal: No se pudo encontrar un estado de recuperacion. Abortando.\n";
                    return false;
                }


                cout << "  [PANIC MODE] Descartando tokens usando el FOLLOW(" << syncNonTerminal << ")...\n";


                const auto& followSets = grammar->getFollows();
                set<string> validFollows;
                if (followSets.find(syncNonTerminal) != followSets.end()) {
                    validFollows = followSets.at(syncNonTerminal);
                }

                validFollows.insert("$");

                while (inputPos < tokens.size()) {
                    currentToken = tokens[inputPos];

                    if (validFollows.count(currentToken)) {
                        cout << "  [PANIC MODE] Símbolo de sincronización encontrado: '" << currentToken << "'\n";
                        break;
                    }

                    if (currentToken == "$") {
                        break;
                    }

                    cout << "  [PANIC MODE]   descartado: '" << currentToken << "'\n";
                    inputPos++;
                }



                TreeNode* errorNode = new TreeNode(syncNonTerminal + " (Recuperado)");
                nodeStack.push_back(errorNode);
                stateStack.push_back(targetState);

                cout << "  [PANIC MODE] Recuperacion completada. Reanudando parsing en Estado " << targetState << "\n\n";
                continue;
        }

        string action = actionTable[currentState][currentToken];
        cout << setw(10) << action;

        row.push_back(action);

        if (action == "acc") {
            if (errorCount > 0) {
                cout << "\n=== Input aceptado con " << errorCount
                     << " error(es) recuperado(s). ===\n";
            } else {
                cout << "\n=== Input accepted! ===\n";
            }
            if (!nodeStack.empty()) {
                cout << "\n=== PARSE TREE ===\n";
                parseTreeRoot = nodeStack.back();
                printParseTree(parseTreeRoot);
            }

            traceTable.push_back(row);

            return (errorCount == 0);
        }
        else if (action[0] == 's') {
            int nextState = stoi(action.substr(1));

            TreeNode* leaf = new TreeNode(currentToken);
            nodeStack.push_back(leaf);
            stateStack.push_back(nextState);

            traceTable.push_back(row);

            inputPos++;

            cout << "  (Shift to state " << nextState << ")\n";
        }
        else if (action[0] == 'r') {
            int prodIndex = stoi(action.substr(1));
            const auto& productions = grammar->getProductions();
            const auto& [head, body] = productions[prodIndex];

            TreeNode* newNode = new TreeNode(head);

            int popCount = body.size();
            if (body.size() == 1 && body[0] == grammar->getEmptySymbol()) {
               popCount = 0;
                TreeNode* epsilonNode = new TreeNode(grammar->getEmptySymbol());
                newNode->children.push_back(epsilonNode);
            } else {
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
    //exportTraceToJSON("trace_table.json");
    //exportParseTreeToJSON("parse_tree.json");
}

void LR1Parser::printParseTree(TreeNode* node, int depth) const {
    if (!node) return;

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



bool LR1Parser::parse(const vector<Token>& tokens) {

    vector<int> stateStack = {0};
    vector<TreeNode*> treeStack;

    size_t tokenIndex = 0;

    while (true) {
        int topState = stateStack.back();

        Token currentToken = (tokenIndex < tokens.size()) ? tokens[tokenIndex] : Token{TokenType::END_OF_FILE, "$", 0, 0};
        string terminal = currentToken.toGrammarString();


        if (actionTable[topState].find(terminal) == actionTable[topState].end()) {
            cout << "[Modo Panico] Error sintactico en linea " << currentToken.line
                 << ", columna " << currentToken.column
                 << ". Token inesperado: '" << currentToken.lexeme << "'\n";

            if (terminal == "$") return false;

            // 1. Descartar el token conflictivo actual para evitar bucles infinitos
            cout << "   Omitiendo: '" << tokens[tokenIndex].lexeme << "'\n";
            tokenIndex++;

            // Descartar hasta encontrar el token de sincronización (NEWLINE o Fin de archivo)
            while (tokenIndex < tokens.size() && tokens[tokenIndex].toGrammarString() != "NEWLINE") {
                cout << "   Omitiendo: '" << tokens[tokenIndex].lexeme << "'\n";
                tokenIndex++;
            }

            if (tokenIndex >= tokens.size()) {
                cout << "-> No se encontro un punto de sincronizacion (NEWLINE). Abortando.\n";
                return false;
            }

            // Avanzamos para consumir el NEWLINE y usarlo como punto de reinicio
            tokenIndex++;

            // 2. Buscar un estado ancla en la pila que acepte sincronización
            bool recovered = false;
            int targetState = -1;
            string syncToken = "BLOCK"; // Asegúrate de que tu gramática use este No Terminal para agrupar bloques

            while (!stateStack.empty()) {
                int state = stateStack.back();

                if (gotoTable[state].find(syncToken) != gotoTable[state].end()) {
                    targetState = gotoTable[state][syncToken];
                    recovered = true;
                    break;
                }

                // Si no sirve, desapilamos
                if (stateStack.size() > 1) {
                    stateStack.pop_back();
                    if (!treeStack.empty()) {
                        delete treeStack.back();
                        treeStack.pop_back();
                    }
                } else {
                    break; // Llegamos al fondo de la pila
                }
            }

            if (!recovered) {
                cout << "-> Error crítico: Imposible recuperarse en los estados actuales de la pila.\n";
                return false;
            }

            // 3. Insertar nodo de error en el árbol y actualizar el estado
            TreeNode* errorBlock = new TreeNode(syncToken);
            TreeNode* errorNode = new TreeNode("% [Error de Sintaxis Omitido en esta linea]");
            errorBlock->children.push_back(errorNode);

            stateStack.push_back(targetState);
            treeStack.push_back(errorBlock);

            cout << "-> ¡Sincronizacion exitosa! Reanudando analisis en el estado " << targetState << "\n\n";
            continue;
        }

        string action = actionTable[topState][terminal];

        if (action[0] == 's') { // SHIFT
            int nextState = stoi(action.substr(1));
            stateStack.push_back(nextState);


            TreeNode* terminalNode = nullptr;
            if (terminal == "PLAIN_TEXT") {
                terminalNode = new TreeNode(currentToken.lexeme);
            } else {
                terminalNode = new TreeNode(terminal);
            }
            treeStack.push_back(terminalNode);

            tokenIndex++;
        }
        else if (action[0] == 'r') { // REDUCE
            int prodIndex = stoi(action.substr(1));
            string head = grammar->getProductions()[prodIndex].first;
            vector<string> body = grammar->getProductions()[prodIndex].second;

            TreeNode* parentNode = new TreeNode(head);


            size_t numSymbolsToPop = body.size();
            if (body.size() == 1 && body[0] == grammar->getEmptySymbol()) {
                numSymbolsToPop = 0; // Épsilon no desapila
            }


            vector<TreeNode*> childrenTemp(numSymbolsToPop);
            for (int i = (int)numSymbolsToPop - 1; i >= 0; --i) {
                stateStack.pop_back();
                childrenTemp[i] = treeStack.back(); // Asigna directamente de derecha a izquierda
                treeStack.pop_back();
            }
            for (TreeNode* child : childrenTemp) {
                parentNode->children.push_back(child);
            }

            //  GOTO
            int topStateAfterPop = stateStack.back();
            int nextState = gotoTable[topStateAfterPop][head];
            stateStack.push_back(nextState);


            treeStack.push_back(parentNode);
        }
        else if (action == "acc") { // ACCEPT
            if (!treeStack.empty()) {
                parseTreeRoot = treeStack.back();
            }
            return true;
        }
    }
    return false;
}


string LR1Parser::generateLatex() {
    if (!parseTreeRoot) {
        return "";
    }


    string body = translateNode(parseTreeRoot);

    //
    string result = "\\documentclass{article}\n";
    result += "\\usepackage[utf8]{inputenc}\n";
    result += "\\usepackage{graphicx}\n";
    result += "\\begin{document}\n\n";

    result += body;

    result += "\n\\end{document}\n";

    return result;
}


string LR1Parser::translateNode(TreeNode* node) {
    if (!node) return "";

    string symbol = node->symbol;

    // 1. Estructura base del documento
    if (symbol == "DOCUMENT") {
        if (!node->children.empty()) {
            return translateNode(node->children[0]); // Solo traduce el BLOCKLIST hijo
        }
        return "";
    }


    if (symbol == "HEADING") {
        if (node->children.size() >= 2) {

            return "\\section{" + translateNode(node->children[1]) + "}\n\n";
        }
    }


    if (symbol == "PARAGRAPH") {
        if (!node->children.empty()) {
            return translateNode(node->children[0]) + "\n\n";
        }
    }


    if (symbol == "BOLD") {
        if (node->children.size() == 3) {

            return "\\textbf{" + node->children[1]->symbol + "}";
        }
    }


    if (symbol == "ITALICS") {
        if (node->children.size() == 3) {
            return "\\textit{" + node->children[1]->symbol + "}";
        }
    }


    if (symbol == "IMAGE") {
        // Estructura de hijos según la regla aplicada:
        // children[0] = IMG_START ("![")
        // children[1] = TEXT (El árbol del texto alternativo)
        // children[2] = R_BRACKET ("]")
        // children[3] = L_PAREN ("(")
        // children[4] = PLAIN_TEXT (El nodo hoja con la ruta/url de la imagen)
        // children[5] = R_PAREN (")")
        // children[6] = NEWLINE

        if (node->children.size() >= 6) {

            string altText = translateNode(node->children[1]);


            string url = node->children[4]->symbol;


            string latexImg = "\\begin{figure}[h]\n";
            latexImg += "  \\centering\n";
            latexImg += "  \\includegraphics[width=0.8\\textwidth]{" + url + "}\n";
            latexImg += "  \\caption{" + altText + "}\n";
            latexImg += "\\end{figure}\n\n";

            return latexImg;
        }
    }


    if (node->children.empty()) {
        if (symbol == "HASH" || symbol == "DOUBLE_AST" || symbol == "ASTERISK" ||
            symbol == "NEWLINE" || symbol == "$" ||
            symbol == "IMG_START" || symbol == "R_BRACKET" || symbol == "L_PAREN" || symbol == "R_PAREN") {
            return "";
        }
        return symbol;
    }


    string concat = "";
    for (TreeNode* child : node->children) {
        concat += translateNode(child);
    }
    return concat;
}
