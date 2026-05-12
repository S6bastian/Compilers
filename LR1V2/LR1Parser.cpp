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
    usePanicMode = true;
    maxErrors = 10;
    errorCount = 0;
    buildStates();
    buildTable();
    computeSyncTokens();
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
            if(!item.body.empty() && item.body[0] == grammar->getEmptySymbol()) continue;
            if (item.dot < (int)item.body.size()) {
                symbols.insert(item.body[item.dot]);
            }
        }

        for (const string& symbol : symbols) {
            State nextState = goTo(currentState, symbol);
            

            if (nextState.empty()) continue;
            
            int existingStateIndex = -1;
            for (size_t j = 0; j < states.size(); ++j) {
                if (states[j] == nextState) {
                    existingStateIndex = j;
                    break;
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




// Al final de LR1Parser.cpp, agrega:

vector<string> LR1Parser::tokenize(const string& input) {
    vector<string> tokens;
    istringstream iss(input);
    string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    tokens.push_back("$"); // Agregar EOF
    return tokens;
}

// Función parse actualizada con recuperación de errores
bool LR1Parser::parse(const string& input, bool panicMode) {
    vector<string> tokens = tokenize(input);
    
    vector<int> stateStack;
    vector<TreeNode*> nodeStack;
    
    stateStack.push_back(0);
    size_t inputPos = 0;
    int stepCount = 0;
    const int MAX_STEPS = 200;
    errorCount = 0;
    bool inputAccepted = false;
    
    cout << "\n=== PARSING TRACE";
    if (usePanicMode) cout << " (Panic Mode Recovery Enabled)";
    cout << " ===\n";
    cout << left << setw(6) << "Step" 
         << setw(35) << "Stack" 
         << setw(25) << "Input" 
         << setw(12) << "Action" << "\n";
    cout << string(78, '-') << "\n";
    
    while (stepCount < MAX_STEPS && inputPos < tokens.size()) {
        stepCount++;
        
        int currentState = stateStack.empty() ? 0 : stateStack.back();
        string currentToken = inputPos < tokens.size() ? tokens[inputPos] : "$";
        
        // Construir representación de la pila
        stringstream stackSS;
        for (size_t i = 0; i < stateStack.size(); i++) {
            if (i > 0) stackSS << " ";
            stackSS << stateStack[i];
            if (i < nodeStack.size()) {
                stackSS << " " << nodeStack[i]->symbol;
            }
        }
        string stackStr = stackSS.str();
        if (stackStr.length() > 34) {
            stackStr = "..." + stackStr.substr(stackStr.length() - 31);
        }
        
        // Construir representación del input restante
        stringstream inputSS;
        for (size_t i = inputPos; i < tokens.size() && i < inputPos + 10; i++) {
            if (i > inputPos) inputSS << " ";
            inputSS << tokens[i];
        }
        if (inputPos + 10 < tokens.size()) inputSS << " ...";
        string inputStr = inputSS.str();
        
        // Imprimir paso actual
        cout << left << setw(6) << stepCount
             << setw(35) << stackStr
             << setw(25) << inputStr;
        
        // Verificar si hay acción definida
        if (actionTable.find(currentState) == actionTable.end() || 
            actionTable[currentState].find(currentToken) == actionTable[currentState].end()) {
            
            cout << setw(12) << "ERROR" << "\n";
            errorCount++;
            
            if (!usePanicMode || errorCount > maxErrors) {
                cout << "Error: No action for state " << currentState 
                     << " and token '" << currentToken << "'\n";
                if (errorCount > maxErrors) {
                    cout << "Maximum error count (" << maxErrors << ") exceeded. Stopping.\n";
                }
                
                // Limpiar memoria
                for (TreeNode* node : nodeStack) {
                    deleteTree(node);
                }
                return false;
            }
            
            // Intentar recuperación de errores
            cout << "  [ERROR " << errorCount << "] No action in state " 
                 << currentState << " for token '" << currentToken << "'\n";
            
            // Primero intentar recuperación a nivel de frase
            phraseLevelRecovery(stateStack, nodeStack, tokens, inputPos);
            
            continue;
        }
        
        string action = actionTable[currentState][currentToken];
        cout << setw(12) << action;
        
        if (action == "acc") {
            cout << "\n=== Input accepted! ===\n";
            if (errorCount > 0) {
                cout << "Note: " << errorCount << " error(s) were recovered\n";
            }
            if (!nodeStack.empty()) {
                cout << "\n=== PARSE TREE ===\n";
                printParseTree(nodeStack.back());
                deleteTree(nodeStack.back());
            }
            inputAccepted = true;
            break;
        }
        else if (action[0] == 's') {
            // Shift
            int nextState = stoi(action.substr(1));
            
            TreeNode* leaf = new TreeNode(currentToken);
            nodeStack.push_back(leaf);
            stateStack.push_back(nextState);
            
            inputPos++;
            
            cout << "  (Shift to " << nextState << ")\n";
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
            
            // Calcular nuevo estado (GOTO)
            int topState = stateStack.back();
            if (gotoTable.find(topState) == gotoTable.end() || 
                gotoTable[topState].find(head) == gotoTable[topState].end()) {
                cout << "Error: No goto for state " << topState 
                     << " and non-terminal '" << head << "'\n";
                
                // Limpiar memoria
                for (TreeNode* node : nodeStack) {
                    deleteTree(node);
                }
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
        }
    }
    
    if (!inputAccepted) {
        cout << "Error: Parsing incomplete or maximum steps exceeded\n";
        if (errorCount > 0) {
            cout << "Total errors encountered: " << errorCount << "\n";
        }
        
        // Limpiar memoria
        for (TreeNode* node : nodeStack) {
            deleteTree(node);
        }
        return false;
    }
    
    return true;
}

void LR1Parser::printParseTrace(const string& input) {
    parse(input);
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




set<string> LR1Parser::getFollowSet(const string& nonTerminal) {
    set<string> followSet;
    
    // Buscar en todos los estados los lookaheads de reducción para este no terminal
    for (const auto& state : states) {
        for (const auto& item : state) {
            if (item.head == nonTerminal && item.dot == (int)item.body.size()) {
                followSet.insert(item.lookahead.begin(), item.lookahead.end());
            }
        }
    }
    
    return followSet;
}


// Calcula los tokens de sincronización para cada estado
void LR1Parser::computeSyncTokens() {
    syncTokens.clear();
    stateSyncTokens.clear();
    
    // 1. Agregar FOLLOW de todos los no terminales como tokens de sincronización
    const auto& nonTerminals = grammar->getProductions();
    set<string> allNonTerminals;
    for (const auto& prod : nonTerminals) {
        allNonTerminals.insert(prod.first);
    }
    
    for (const string& nt : allNonTerminals) {
        set<string> followSet = getFollowSet(nt);
        syncTokens.insert(followSet.begin(), followSet.end());
    }
    
    // 2. Agregar símbolos de inicio de producciones (FIRST sets)
    for (size_t i = 0; i < states.size(); ++i) {
        for (const auto& item : states[i]) {
            if (item.dot < (int)item.body.size()) {
                string nextSymbol = item.body[item.dot];
                if (grammar->isTerminal(nextSymbol)) {
                    syncTokens.insert(nextSymbol);
                }
            }
        }
    }
    
    // 3. Para cada estado, calcular tokens de sincronización específicos
    for (size_t i = 0; i < states.size(); ++i) {
        stateSyncTokens[i] = set<string>();
        
        // Agregar todos los símbolos para los que el estado tiene acciones definidas
        if (actionTable.find(i) != actionTable.end()) {
            for (const auto& [symbol, action] : actionTable.at(i)) {
                if (action[0] == 's' || action[0] == 'r' || action == "acc") {
                    stateSyncTokens[i].insert(symbol);
                }
            }
        }
        
        // Agregar tokens de sincronización generales
        stateSyncTokens[i].insert(syncTokens.begin(), syncTokens.end());
    }
    
    // 4. Agregar delimitadores comunes
    syncTokens.insert("$");
    syncTokens.insert(";");
    syncTokens.insert(")");
    syncTokens.insert("}");
    syncTokens.insert("]");
    
    cout << "Sync tokens computed: ";
    for (const string& t : syncTokens) {
        cout << t << " ";
    }
    cout << "\n";
}


bool LR1Parser::isSyncToken(int state, const string& token) {
    // Verificar tokens específicos del estado
    if (stateSyncTokens.find(state) != stateSyncTokens.end()) {
        if (stateSyncTokens[state].find(token) != stateSyncTokens[state].end()) {
            return true;
        }
    }
    
    // Verificar tokens generales
    return syncTokens.find(token) != syncTokens.end();
}


// Recuperación en modo pánico
void LR1Parser::panicModeRecovery(vector<int>& stateStack, 
                                  vector<TreeNode*>& nodeStack,
                                  vector<string>& tokens, 
                                  size_t& inputPos) {
    cout << "  [PANIC MODE] Attempting recovery...\n";
    
    // Paso 1: Desapilar estados hasta encontrar uno que tenga GOTO para algún no terminal
    while (!stateStack.empty()) {
        int currentState = stateStack.back();
        bool hasGoto = false;
        
        if (gotoTable.find(currentState) != gotoTable.end()) {
            for (const auto& [nt, target] : gotoTable.at(currentState)) {
                // Verificar si este estado fue alcanzado desde el no terminal correcto
                if (!nodeStack.empty() && nodeStack.back()->symbol == nt) {
                    hasGoto = true;
                    break;
                }
            }
        }
        
        if (hasGoto) break;
        
        // Desapilar un estado
        if (!stateStack.empty()) stateStack.pop_back();
        if (!nodeStack.empty()) {
            // No eliminamos el nodo, solo lo sacamos de la pila
            nodeStack.pop_back();
        }
        
        if (stateStack.empty()) {
            cout << "  [PANIC MODE] Stack empty, cannot recover\n";
            return;
        }
    }
    
    // Paso 2: Avanzar en la entrada hasta encontrar un token de sincronización
    int currentState = stateStack.empty() ? 0 : stateStack.back();
    size_t startPos = inputPos;
    bool foundSync = false;
    
    while (inputPos < tokens.size()) {
        string currentToken = tokens[inputPos];
        
        if (isSyncToken(currentState, currentToken)) {
            foundSync = true;
            cout << "  [PANIC MODE] Found sync token '" << currentToken 
                 << "' at position " << inputPos << "\n";
            
            // Mostrar tokens descartados
            if (inputPos > startPos) {
                cout << "  [PANIC MODE] Discarded tokens: ";
                for (size_t i = startPos; i < inputPos; i++) {
                    cout << tokens[i] << " ";
                }
                cout << "\n";
            }
            break;
        }
        
        inputPos++;
    }
    
    if (!foundSync) {
        cout << "  [PANIC MODE] No sync token found, setting to end of input\n";
        inputPos = tokens.size() - 1; // Apuntar al EOF
    }
}


// Recuperación a nivel de frase (más sofisticada)
void LR1Parser::phraseLevelRecovery(vector<int>& stateStack,
                                   vector<TreeNode*>& nodeStack,
                                   vector<string>& tokens,
                                   size_t& inputPos) {
    cout << "  [PHRASE LEVEL] Attempting recovery...\n";
    
    int currentState = stateStack.empty() ? 0 : stateStack.back();
    
    // Intentar inserción de token
    if (actionTable.find(currentState) != actionTable.end()) {
        for (const auto& [token, action] : actionTable.at(currentState)) {
            if (token != "$" && action[0] == 's') {
                // Podríamos insertar este token
                cout << "  [PHRASE LEVEL] Could insert '" << token << "' before '" 
                     << tokens[inputPos] << "'\n";
                
                // Nota: La inserción real requeriría modificar el flujo de tokens
                // Por ahora solo lo reportamos
            }
        }
    }
    
    // Intentar reemplazo de token
    if (actionTable.find(currentState) != actionTable.end()) {
        const auto& actions = actionTable.at(currentState);
        for (const auto& [token, action] : actions) {
            if (token != "$" && token != tokens[inputPos] && action[0] == 's') {
                cout << "  [PHRASE LEVEL] Could replace '" << tokens[inputPos] 
                     << "' with '" << token << "'\n";
                
                // Reemplazar el token actual
                tokens[inputPos] = token;
                cout << "  [PHRASE LEVEL] Replaced token with '" << token << "'\n";
                return;
            }
        }
    }
    
    // Si no se puede recuperar a nivel de frase, usar modo pánico
    panicModeRecovery(stateStack, nodeStack, tokens, inputPos);
}