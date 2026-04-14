#pragma once
#include "Grammar.h"
#include <set>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>

using namespace std;

enum ActionType { SHIFT, REDUCE, ACCEPT, ERROR };

struct TableEntry {
    ActionType type = ERROR;
    int id = -1;
};

struct Node {
    string value;
    vector<Node*> children;
    Node(string v) : value(v) {}
};

struct LR1Item {
    string head;
    vector<string> body;
    int dot;
    string lookahead;

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

typedef set<LR1Item> State;

class LR1Parser {
public:
    LR1Parser(Grammar *g);
    void buildCanonicalCollection();
    void printStates() const;
    void buildTable();
    void printTable() const;
    void parse(const vector<string>& input);
    void printTree(Node* node, int depth) const;

private:
    Grammar *grammar;
    vector<State> states; 
    map<int, map<string, int>> transitions;
    map<int, map<string, TableEntry>> parsingTable;

    State closure(State I);
    State goTo(const State& I, const string& X);
    set<string> computeFirstChain(vector<string> beta, string lookahead);
};