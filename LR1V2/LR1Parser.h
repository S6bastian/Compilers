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


    // Parsing methods
    bool parse(const string& input, bool panicMode = true);
    void printParseTrace(const string& input);
    void printParseTree(TreeNode* node, int depth = 0) const;
    void deleteTree(TreeNode* node);

    // Panic Mode methods
    void setPanicMode(bool enable) { usePanicMode = enable; }
    void setMaxErrors(int max) { maxErrors = max; }
    int getErrorCount() const { return errorCount; }

private:
    Grammar *grammar;
    
    vector<State> states;
    map<int, map<string, int>> transitions;
    map<int, map<string, string>> actionTable;  // actionTable[state][symbol] = "sX" o "rX" o "acc"
    map<int, map<string, int>> gotoTable;       // gotoTable[state][nonTerminal] = nextState

    // Panic Mode atributes
    bool usePanicMode;
    int maxErrors;
    int errorCount;
    set<string> syncTokens;
    map<int, set<string>> stateSyncTokens;




    set<string> computeLookahead(LR1Item item);
    vector<LR1Item> closure(vector<LR1Item> kernels);
    State goTo(const State& state, const string& symbol);

    // Parsing method
    vector<string> tokenize(const string& input);
    
    // Panic Mode method
    void computeSyncTokens();
    set<string> getFollowSet(const string& nonTerminal);
    bool isSyncToken(int state, const string& token);
    void panicModeRecovery(vector<int>& stateStack, 
                          vector<TreeNode*>& nodeStack,
                          vector<string>& tokens, 
                          size_t& inputPos);
    void phraseLevelRecovery(vector<int>& stateStack,
                            vector<TreeNode*>& nodeStack,
                            vector<string>& tokens,
                            size_t& inputPos);

};