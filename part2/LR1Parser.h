#pragma once
#include "Grammar.h"
#include "Token.h"
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <deque>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>

using namespace std;

struct LR1Item {
    string head;
    vector<string> body;
    int dot;
    set<string> lookahead;

    bool operator<(const LR1Item& other) const {
        if (head != other.head) return head < other.head;
        if (body != other.body) return body < other.body;
        if (dot != other.dot) return dot < other.dot;
        return lookahead < other.lookahead;
    }

    bool operator==(const LR1Item& other) const {
        return head == other.head && body == other.body &&
               dot == other.dot && lookahead == other.lookahead;
    }
};


struct TreeNode {
    string symbol;
    vector<TreeNode*> children;

    TreeNode(const string& sym) : symbol(sym) {}

    ~TreeNode() {
        for (TreeNode* child : children) {
            delete child;
        }
    }
};


typedef vector<LR1Item> State;

class LR1Parser {
public:
    LR1Parser(Grammar *g);

    TreeNode* getParseTreeRoot() const { return parseTreeRoot; }

    void buildStates();
    void printStates() const;
    void buildTable();
    void printTable() const;

    bool parse(const string& input);
    bool parse(const std::vector<Token>& tokens);
    void printParseTrace(const string& input);
    void printParseTree(TreeNode* node, int depth = 0) const;
    void deleteTree(TreeNode* node);

    // export
    void exportFirstSetsToJSON(const std::string& filename) const;
    void exportCanonicalCollectionToJSON(const std::string& filename) const;
    void exportTableToJSON(const std::string& filename) const;
    void exportTraceToJSON(const string& filename) const;
    void exportParseTreeToJSON(const string& filename) const;
    void exportTreeNodeToJSON(ofstream& out, const TreeNode* node, int depth) const;

    // latex
    string generateLatex();

private:
    Grammar* grammar;
    TreeNode* parseTreeRoot;

    vector<State> states;
    map<int, map<string, int>> transitions;
    map<int, map<string, string>> actionTable;
    map<int, map<string, int>> gotoTable;
    vector<vector<string>> traceTable;

    set<string> computeLookahead(LR1Item item);
    vector<LR1Item> closure(vector<LR1Item> kernels);
    State goTo(const State& state, const string& symbol);

    vector<string> tokenize(const string& input);
    string translateNode(TreeNode* node);
};




class LatexGenerator {
private:
    // Método recursivo para recorrer el árbol y acumular la traducción
    std::string translateNode(TreeNode* node) {
        if (!node) return "";

        // CASO 1: Es un nodo hoja (Terminal)
        if (node->children.empty()) {
            // Nota: El parser solo guarda el nombre del token (ej: "PLAIN_TEXT" o "HASH").
            // Si necesitas el LEXEMA exacto guardado durante el escaneo, usualmente se pasa
            // al árbol en el proceso de reducción. Asumiendo que tu TreeNode actual guarda el
            // símbolo o token evaluado, procesamos según las reglas:
            return "";
        }

        // CASO 2: Nodo No Terminal. Evaluamos las reglas de producción.
        std::string symbol = node->symbol;

        if (symbol == "DOCUMENT") {
            std::string body = "";
            for (TreeNode* child : node->children) {
                body += translateNode(child);
            }
            // Envolvemos el cuerpo en el template de LaTeX
            std::string result = "\\documentclass{article}\n";
            result += "\\usepackage[utf8]{inputenc}\n";
            result += "\\begin{document}\n\n";
            result += body;
            result += "\\end{document}\n";
            return result;
        }

        if (symbol == "HEADING") {
            // HEADING -> HASH TEXT NEWLINE
            // El texto real estará en el segundo hijo (índice 1)
            if (node->children.size() >= 2) {
                return "\\section{" + translateNode(node->children[1]) + "}\n\n";
            }
        }

        if (symbol == "PARAGRAPH") {
            // PARAGRAPH -> TEXT NEWLINE
            if (!node->children.empty()) {
                return translateNode(node->children[0]) + "\n\n";
            }
        }

        if (symbol == "BOLD") {
            // BOLD -> DOUBLE_AST PLAIN_TEXT DOUBLE_AST
            // Extraemos el contenido de PLAIN_TEXT (hijo del medio)
            if (node->children.size() == 3) {
                return "\\textbf{" + node->children[1]->symbol + "}";
            }
        }

        if (symbol == "ITALICS") {
            // ITALICS -> ASTERISK PLAIN_TEXT ASTERISK
            if (node->children.size() == 3) {
                return "\\textit{" + node->children[1]->symbol + "}";
            }
        }

        if (symbol == "ELEMENT") {
            // ELEMENT puede derivar en BOLD, ITALICS o PLAIN_TEXT plano
            if (!node->children.empty()) {
                TreeNode* child = node->children[0];
                // Si es texto plano directo (hoja) y no una estructura anidada
                if (child->symbol != "BOLD" && child->symbol != "ITALICS" && child->children.empty()) {
                    return child->symbol; // Aquí va el lexema original
                }
                return translateNode(child);
            }
        }

        // Por defecto, concatenar el resultado de todos los hijos (ej. BLOCKLIST, TEXT)
        std::string concat = "";
        for (TreeNode* child : node->children) {
            concat += translateNode(child);
        }
        return concat;
    }

public:
    std::string generate(TreeNode* root) {
        return translateNode(root);
    }
};
