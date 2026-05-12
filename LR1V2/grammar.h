#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
using namespace std;

class Grammar {
public:
    Grammar(const string& filename, string emptySym = "e");
    void loadFromFile(const string& filename);

    const vector<pair<string, vector<string>>>& getProductions() const;
    const string& getStartSymbol() const;
    const string& getEmptySymbol() const;
    const set<string> getFirsts(const string& head) const;
    bool isTerminal(const string& symbol) const;
    bool isNonTerminal(const string& symbol) const;
    void printDebug() const;

private:
    void extractTerminals();
    void extractFirsts();

    string startSymbol;
    string emptySymbol;
    vector<pair<string, vector<string>>> productions;
    set<string> nonTerminals;
    set<string> terminals;
    map<string, set<string>> firsts;
};