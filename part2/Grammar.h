#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
using namespace std;

class Grammar {
public:
    Grammar(const string& filename, string emptySym = "e");
    Grammar(const vector<string>& grammarLines, string emptySym = "e");
    void loadFromFile(const string& filename);

    const vector<pair<string, vector<string>>>& getProductions() const;
    const string& getStartSymbol() const;
    const string& getEmptySymbol() const;
    const map<string, set<string>>& getFollows() const;
    const set<string> getFirsts(const string& head) const;
    bool isTerminal(const string& symbol) const;
    bool isNonTerminal(const string& symbol) const;
    void printDebug() const;

private:
    void loadFromLines(const vector<string>& lines);

    void extractTerminals();
    void extractFirsts();

    string startSymbol;
    string emptySymbol;
    vector<pair<string, vector<string>>> productions;
    set<string> nonTerminals;
    set<string> terminals;
    map<string, set<string>> firsts;

    void extractFollows();
    map<string, set<string>> follows;
};
