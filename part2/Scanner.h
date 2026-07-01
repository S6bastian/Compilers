#pragma once

#include <string>
#include <fstream>
#include "Token.h"

class Scanner {
private:
    std::string sourceCode;
    size_t currentIndex;
    int currentLine;
    int currentColumn;


    char getchar();
    char peekchar();
    bool isAtEnd() const;

public:

    Scanner(const std::string& filePath);


    Token gettoken();
};
