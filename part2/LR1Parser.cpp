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
    // Inicializamos el Scanner apuntando al archivo que viene en el parámetro 'input'
    Scanner scanner(input);

    // DECLARACIÓN LOCAL DE LAS PILAS (Como lo tenías originalmente)
    vector<int> stateStack;
    vector<TreeNode*> symbolStack;

    // Limpieza de estados y árboles previos
    if (parseTreeRoot) {
        deleteTree(parseTreeRoot);
        parseTreeRoot = nullptr;
    }
    traceTable.clear();

    // El estado inicial del parser LR(1) siempre empieza en 0
    stateStack.push_back(0);

    // Pedimos el primer token real al Scanner
    Token currentToken = scanner.gettoken();

    int steps = 0;
    int maxSteps = 2000; // Evita bucles infinitos en caso de errores graves

    while (steps < maxSteps) {
        steps++;
        int topState = stateStack.back();

        // Convertimos el tipo de token a la cadena de texto exacta que espera tu tabla LR(1)
        string currentSymbol = currentToken.toGrammarString();

        // Si el Scanner detectó un error léxico, detenemos el parser inmediatamente
        if (currentSymbol == "ERROR") {
            cout << "Error léxico en la línea " << currentToken.line
                 << ", columna " << currentToken.column
                 << ": Lexema inválido '" << currentToken.lexeme << "'\n";
            return false;
        }

        // Verificamos si existe una acción asignada para el estado actual con este símbolo
        if (actionTable[topState].find(currentSymbol) == actionTable[topState].end()) {
            cout << "Error sintáctico en la línea " << currentToken.line
                 << ", columna " << currentToken.column
                 << ": No se esperaba el token '" << currentSymbol
                 << "' ( '" << currentToken.lexeme << "' )\n";
            return false;
        }

        string action = actionTable[topState][currentSymbol];

        // Llenamos la fila de la traza para el reporte en consola
        vector<string> row;
        string stateStr = "";
        for (int s : stateStack) stateStr += to_string(s) + " ";
        row.push_back(stateStr);

        string symStr = "";
        for (TreeNode* n : symbolStack) symStr += n->symbol + " ";
        row.push_back(symStr);
        row.push_back(currentSymbol);
        row.push_back(action);

        // --- CASO 1: ACCIÓN SHIFT (DESPLAZAMIENTO) ---
        if (action[0] == 's') {
            int nextState = stoi(action.substr(1));
            stateStack.push_back(nextState);

            // Creamos un nodo del árbol para este token terminal guardando su lexema real
            TreeNode* leafNode = new TreeNode(currentSymbol);
            symbolStack.push_back(leafNode);

            cout << "Step " << steps << ": Shift " << nextState << " with token " << currentSymbol << " [ " << currentToken.lexeme << " ]\n";

            traceTable.push_back(row);

            // ¡AVANZAMOS al siguiente token físico del archivo!
            currentToken = scanner.gettoken();
        }
        // --- CASO 2: ACCIÓN REDUCE (REDUCCIÓN) ---
        else if (action[0] == 'r') {
            int prodIndex = stoi(action.substr(1));
            auto const& production = grammar->getProductions()[prodIndex];
            string head = production.first;
            vector<string> body = production.second;

            // Creamos el nodo padre no terminal
            TreeNode* parentNode = new TreeNode(head);
            vector<TreeNode*> children;

            // Si la regla no produce epsilon (vacío), sacamos elementos de la pila
            if (!(body.size() == 1 && body[0] == grammar->getEmptySymbol())) {
                for (size_t i = 0; i < body.size(); ++i) {
                    stateStack.pop_back();
                    children.push_back(symbolStack.back());
                    symbolStack.pop_back();
                }
                // Invertimos el orden para que los hijos queden de izquierda a derecha en el árbol
                reverse(children.begin(), children.end());
                parentNode->children = children;
            } else {
                // Caso épsilon: añadimos un nodo hijo vacío
                parentNode->children.push_back(new TreeNode(grammar->getEmptySymbol()));
            }

            symbolStack.push_back(parentNode);

            // Consultamos la tabla GOTO para saber a qué estado saltar tras la reducción
            int topStateAfterPop = stateStack.back();
            if (gotoTable[topStateAfterPop].find(head) == gotoTable[topStateAfterPop].end()) {
                cout << "Error: No existe transición GOTO para el No Terminal " << head << "\n";
                return false;
            }

            int nextState = gotoTable[topStateAfterPop][head];
            stateStack.push_back(nextState);

            cout << "Step " << steps << ": Reduce by " << head << " -> ";
            for (const string& s : body) cout << s << " ";
            cout << ", goto " << nextState << "\n";

            traceTable.push_back(row);
            // NOTA: Aquí NO llamamos a scanner.gettoken() porque el token actual debe ser
            // reevaluado en el siguiente ciclo con el nuevo estado del Parser.
        }
        // --- CASO 3: ACCIÓN ACCEPT (ACEPTACIÓN) ---
        else if (action == "acc") {
            traceTable.push_back(row);
            parseTreeRoot = symbolStack.back(); // El nodo raíz final queda en la cima
            cout << "\nSUCCESS: El archivo 'example.txt' fue procesado y parseado CORRECTAMENTE.\n";
            return true;
        }
    }

    cout << "Error: Se excedió el número máximo de pasos en el Parser.\n";
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
