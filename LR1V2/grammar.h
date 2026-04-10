#pragma once
#include <string>
#include <vector>
#include <set>
using namespace std;

class Grammar {
public:
    void loadFromFile(const string& filename);
    const vector<pair<string, vector<string>>>& getProductions() const;
    const string& getStartSymbol() const;

private:
    string startSymbol;
    vector<pair<string, vector<string>>> productions;
    set<string> nonTerminals;
    set<string> terminals;
};