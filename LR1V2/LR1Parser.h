#pragma once
#include "Grammar.h"
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <deque>
#include <iostream>
#include <iomanip>

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

typedef vector<LR1Item> State;

class LR1Parser {
public:
    LR1Parser(Grammar *g);

    void buildStates();
    void printStates() const;

private:
    Grammar *grammar;
    
    vector<State> states;
    map<int, map<string, int>> transitions;

    vector<LR1Item> closure(vector<LR1Item> kernels);
    State goTo(const State& state, const string& symbol);
    set<string> computeLookahead(LR1Item item);
};