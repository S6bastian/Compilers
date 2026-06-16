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

                // 1. BUSCAR ESTADO ANCLA (Retroceder en la pila)
                // Evaluamos desde el tope de la pila hacia abajo qué No Terminal podemos usar
                while (!stateStack.empty()) {
                    int topState = stateStack.back();

                    // Intentamos sincronizar con los No Terminales de menor a mayor jerarquía
                    // para no destruir árboles de derivación superiores si el error es local.
                    // En tu gramática: F (factores), T (términos), E (expresiones)
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

                    // Si el estado actual de la pila no tiene GOTO para nuestros No Terminales, lo sacamos
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

                // 2. DESCARTAR TOKENS EN LA ENTRADA (Sincronización con el FOLLOW del No Terminal)
                cout << "  [PANIC MODE] Descartando tokens usando el FOLLOW(" << syncNonTerminal << ")...\n";

                // Obtenemos el conjunto FOLLOW que acabamos de implementar en la clase Grammar
                const auto& followSets = grammar->getFollows();
                set<string> validFollows;
                if (followSets.find(syncNonTerminal) != followSets.end()) {
                    validFollows = followSets.at(syncNonTerminal);
                }

                // El símbolo '$' siempre es un punto de parada seguro para evitar desbordar la entrada
                validFollows.insert("$");

                while (inputPos < tokens.size()) {
                    currentToken = tokens[inputPos];

                    // Si el token actual está en el FOLLOW del No Terminal elegido, nos detenemos aquí.
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

                // 3. AISLAR EL ERROR EN EL ÁRBOL Y REANUDAR
                // Insertamos un nodo placeholder en el árbol sintáctico para que la estructura no se rompa
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
    // Limpieza e inicialización clásica de tus pilas
    vector<int> stateStack = {0};
    vector<TreeNode*> treeStack; // Asegúrate de tener una pila de nodos para armar el árbol

    size_t tokenIndex = 0;

    while (true) {
        int topState = stateStack.back();

        // Obtener el token actual del vector. Si llegamos al final, usamos el símbolo "$" (EOF)
        Token currentToken = (tokenIndex < tokens.size()) ? tokens[tokenIndex] : Token{TokenType::END_OF_FILE, "$", 0, 0};
        string terminal = currentToken.toGrammarString();

        // Verificar la acción en tu tabla
        if (actionTable[topState].find(terminal) == actionTable[topState].end()) {
            // 1. REPORTAR EL ERROR SINTÁCTICO
            cout << "[Modo Panico] Error sintactico en linea " << currentToken.line
                 << ", columna " << currentToken.column
                 << ". Token inesperado: '" << currentToken.lexeme << "'\n";

            // Si ya llegamos al fin del archivo, no hay nada que recuperar
            if (terminal == "$") return false;

            // 2. AVANZAR EL SCANNER HASTA UN TOKEN DE SINCRONIZACIÓN (Hacia adelante)
            // Buscaremos el siguiente NEWLINE para intentar procesar la siguiente línea/bloque
            cout << "-> Descartando tokens conflictivos hasta encontrar un salto de linea (NEWLINE)..." << endl;
            while (tokenIndex < tokens.size() && tokens[tokenIndex].toGrammarString() != "NEWLINE") {
                cout << "   Omitiendo: '" << tokens[tokenIndex].lexeme << "'\n";
                tokenIndex++;
            }

            // Si nos comimos todos los tokens y llegamos al final del archivo
            if (tokenIndex >= tokens.size()) {
                cout << "-> No se encontro un punto de sincronizacion. Abortando.\n";
                return false;
            }

            // Consumimos también el NEWLINE para posicionarnos al inicio de la siguiente línea limpia
            tokenIndex++;

            // 3. LIMPIAR PILAS HASTA ENCONTRAR UN ESTADO COMPATIBLE (Hacia atrás)
            // Queremos regresar a un estado donde sea seguro insertar un bloque (BLOCKLIST o DOCUMENT)
            cout << "-> Buscando un estado seguro en la pila para reanudar el analisis..." << endl;
            while (!stateStack.empty()) {
                int state = stateStack.back();

                // Comprobamos si desde este estado podemos saltar al manejo de un bloque ("BLOCK")
                if (gotoTable[state].find("BLOCK") != gotoTable[state].end()) {
                    int nextState = gotoTable[state]["BLOCK"];

                    // Creamos un nodo ficticio en el árbol para representar el bloque que tenía errores
                    // Así el LatexGenerator no se rompe al recorrerlo, simplemente ignorará el contenido vacío
                    TreeNode* errorBlock = new TreeNode("BLOCK");
                    TreeNode* errorNode = new TreeNode("% [Error de Sintaxis Omitido en esta linea]");
                    errorBlock->children.push_back(errorNode);

                    // Forzamos la transición sintáctica en la pila
                    stateStack.push_back(nextState);
                    treeStack.push_back(errorBlock);

                    cout << "-> ¡Sincronizacion exitosa! Reanudando analisis en el estado " << nextState << "\n\n";
                    break; // Salimos del modo pánico y el bucle principal 'while(true)' continuará normalmente
                }

                // Si el estado de la pila no sabe qué hacer con un "BLOCK", lo desapilamos
                if (stateStack.size() > 1) {
                    stateStack.pop_back();
                    if (!treeStack.empty()) {
                        delete treeStack.back(); // Limpieza de memoria para no dejar colgados nodos huérfanos
                        treeStack.pop_back();
                    }
                } else {
                    // Si llegamos al estado inicial (0) y ni ahí se puede sincronizar
                    cout << "-> Error crítico: Imposible recuperarse en el estado raíz.\n";
                    return false;
                }
            }

            // Continuamos el bucle while(true) principal
            continue;
        }

        string action = actionTable[topState][terminal];

        if (action[0] == 's') { // SHIFT
            int nextState = stoi(action.substr(1));
            stateStack.push_back(nextState);

            // ¡AQUÍ ESTÁ EL TRUCO!
            // Si el terminal es PLAIN_TEXT, guardamos su LEXEMA REAL en el nodo del árbol
            TreeNode* terminalNode = nullptr;
            if (terminal == "PLAIN_TEXT") {
                terminalNode = new TreeNode(currentToken.lexeme); // Guarda "Hello World", no "PLAIN_TEXT"
            } else {
                terminalNode = new TreeNode(terminal); // Guarda "HASH", "DOUBLE_AST", etc.
            }
            treeStack.push_back(terminalNode);

            tokenIndex++; // Avanzamos al siguiente token real
        }
        else if (action[0] == 'r') { // REDUCE
            int prodIndex = stoi(action.substr(1));
            string head = grammar->getProductions()[prodIndex].first;
            vector<string> body = grammar->getProductions()[prodIndex].second;

            TreeNode* parentNode = new TreeNode(head);

            // Desapilamos los estados y recolectamos los hijos del árbol
            size_t numSymbolsToPop = body.size();
            if (body.size() == 1 && body[0] == grammar->getEmptySymbol()) {
                numSymbolsToPop = 0; // Épsilon no desapila
            }

            // Los hijos salen en orden inverso de la pila
            vector<TreeNode*> childrenTemp;
            for (size_t i = 0; i < numSymbolsToPop; ++i) {
                stateStack.pop_back();
                childrenTemp.push_back(treeStack.back());
                treeStack.pop_back();
            }
            // Los invertimos para que queden en el orden correcto de izquierda a derecha
            for (auto it = childrenTemp.rbegin(); it != childrenTemp.rend(); ++it) {
                parentNode->children.push_back(*it);
            }

            // Hacemos el GOTO
            int topStateAfterPop = stateStack.back();
            int nextState = gotoTable[topStateAfterPop][head];
            stateStack.push_back(nextState);

            // Colocamos el nuevo nodo no terminal en la pila del árbol
            treeStack.push_back(parentNode);
        }
        else if (action == "acc") { // ACCEPT
            if (!treeStack.empty()) {
                parseTreeRoot = treeStack.back(); // La raíz final del documento
            }
            return true;
        }
    }
    return false;
}

// Método público: Verifica que el árbol exista e inicia la recursión desde la raíz
string LR1Parser::generateLatex() {
    if (!parseTreeRoot) {
        return "% Error: No se ha construido el arbol de derivacion sintactica.\n";
    }
    return translateNode(parseTreeRoot);
}

// Método privado: Analiza el símbolo de cada TreeNode y genera su código equivalente
string LR1Parser::translateNode(TreeNode* node) {
    if (!node) return "";

    string symbol = node->symbol;

    // 1. Estructura base del documento
    if (symbol == "DOCUMENT") {
        string body = "";
        for (TreeNode* child : node->children) {
            body += translateNode(child);
        }
        string result = "\\documentclass{article}\n";
        result += "\\usepackage[utf8]{inputenc}\n";
        result += "\\begin{document}\n\n";
        result += body;
        result += "\\end{document}\n";
        return result;
    }

    // 2. Títulos / Encabezados: HEADING -> HASH TEXT NEWLINE
    if (symbol == "HEADING") {
        if (node->children.size() >= 2) {
            // El hijo en el índice 1 es el nodo No Terminal "TEXT"
            return "\\section{" + translateNode(node->children[1]) + "}\n\n";
        }
    }

    // 3. Párrafos tradicionales: PARAGRAPH -> TEXT NEWLINE
    if (symbol == "PARAGRAPH") {
        if (!node->children.empty()) {
            return translateNode(node->children[0]) + "\n\n";
        }
    }

    // 4. Negritas: BOLD -> DOUBLE_AST PLAIN_TEXT DOUBLE_AST
    if (symbol == "BOLD") {
        if (node->children.size() == 3) {
            // Gracias al cambio en el Shift, node->children[1]->symbol contiene el texto real
            return "\\textbf{" + node->children[1]->symbol + "}";
        }
    }

    // 5. Cursivas: ITALICS -> ASTERISK PLAIN_TEXT ASTERISK
    if (symbol == "ITALICS") {
        if (node->children.size() == 3) {
            return "\\textit{" + node->children[1]->symbol + "}";
        }
    }

    // 6. Hojas Terminales
    if (node->children.empty()) {
        // Ignoramos los tokens de control de markdown en la salida final porque ya los procesamos arriba
        if (symbol == "HASH" || symbol == "DOUBLE_AST" || symbol == "ASTERISK" || symbol == "NEWLINE" || symbol == "$") {
            return "";
        }
        // Si es texto plano puro retenido en el shift, lo imprimimos directamente
        return symbol;
    }

    // Caso base por defecto: Concatenar el resultado de subestructuras (BLOCKLIST, TEXT, ELEMENT)
    string concat = "";
    for (TreeNode* child : node->children) {
        concat += translateNode(child);
    }
    return concat;
}
