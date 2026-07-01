#include "Scanner.h"
#include <iostream>
#include <sstream>


Scanner::Scanner(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << filePath << std::endl;
        sourceCode = "";
    } else {
        std::stringstream buffer;
        buffer << file.rdbuf();
        sourceCode = buffer.str();
    }

    currentIndex = 0;
    currentLine = 1;
    currentColumn = 1;
}


bool Scanner::isAtEnd() const {
    return currentIndex >= sourceCode.length();
}


char Scanner::getchar() {
    if (isAtEnd()) return '\0';

    char c = sourceCode[currentIndex];
    currentIndex++;

    if (c == '\n') {
        currentLine++;
        currentColumn = 1;
    } else {
        currentColumn++;
    }

    return c;
}


char Scanner::peekchar() {
    if (isAtEnd()) return '\0';
    return sourceCode[currentIndex];
}


Token Scanner::gettoken() {

    if (isAtEnd()) {
        return Token{TokenType::END_OF_FILE, "EOF", currentLine, currentColumn};
    }

    int tokenLine = currentLine;
    int tokenColumn = currentColumn;

    char c = getchar();

    if (c == '!' && peekchar() == '[') {
        getchar();
        return Token{TokenType::IMG_START, "![", tokenLine, tokenColumn};
    }

    if (c == ']') return Token{TokenType::R_BRACKET, "]", tokenLine, tokenColumn};
    if (c == '(') return Token{TokenType::L_PAREN, "(", tokenLine, tokenColumn};
    if (c == ')') return Token{TokenType::R_PAREN, ")", tokenLine, tokenColumn};

    if (c == '\n') {
        return Token{TokenType::NEWLINE, "\\n", tokenLine, tokenColumn};
    }

    if (c == '#') {
        return Token{TokenType::HASH, "#", tokenLine, tokenColumn};
    }

    if (c == '*') {
        if (peekchar() == '*') {
            getchar();
            return Token{TokenType::DOUBLE_AST, "**", tokenLine, tokenColumn};
        }
        return Token{TokenType::ASTERISK, "*", tokenLine, tokenColumn};
    }

    std::string lexeme = "";
    lexeme += c;

    while (!isAtEnd()) {
        char next = peekchar();

        if (next == '\n' || next == '#' || next == '*' || next == ']' || next == '(' || next == ')') {
            break;
        }


        lexeme += getchar();
    }

    return Token{TokenType::PLAIN_TEXT, lexeme, tokenLine, tokenColumn};
}
