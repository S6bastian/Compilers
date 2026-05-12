#pragma once
#include "Grammar.h"
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

    void buildStates();
    void printStates() const;
    void buildTable();
    void printTable() const;

    bool parse(const string& input);
    void printParseTrace(const string& input);
    void printParseTree(TreeNode* node, int depth = 0) const;
    void deleteTree(TreeNode* node);

    // export
    void exportCanonicalCollectionToJSON(const std::string& filename) const;
    void exportTableToJSON(const std::string& filename) const;
    void exportTraceToJSON(const string& filename) const;
    void exportParseTreeToJSON(const string& filename) const;
    void exportTreeNodeToJSON(ofstream& out, const TreeNode* node, int depth) const;
    
    
private:
    Grammar* grammar;
    TreeNode* parseTreeRoot;
    
    vector<State> states;
    map<int, map<string, int>> transitions;
    map<int, map<string, string>> actionTable;  // actionTable[state][symbol] = "sX" o "rX" o "acc"
    map<int, map<string, int>> gotoTable;       // gotoTable[state][nonTerminal] = nextState
    vector<vector<string>> traceTable;

    set<string> computeLookahead(LR1Item item);
    vector<LR1Item> closure(vector<LR1Item> kernels);
    State goTo(const State& state, const string& symbol);

    vector<string> tokenize(const string& input);
};