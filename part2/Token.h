#pragma once

#include <string>


enum class TokenType {
    HASH,           // #
    DOUBLE_AST,     // **
    ASTERISK,       // *
    NEWLINE,        // \n
    PLAIN_TEXT,     // Cualquier corrida de texto libre
    END_OF_FILE,    // Fin del archivo (EOF)
    TOKEN_ERROR,     // Para manejo de errores léxicos
    IMG_START,     // ! [
    R_BRACKET,     // ]
    L_PAREN,       // (
    R_PAREN       // )
};


struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;



    std::string toGrammarString() const {
        switch (type) {
            case TokenType::HASH:        return "HASH";
            case TokenType::DOUBLE_AST:  return "DOUBLE_AST";
            case TokenType::ASTERISK:    return "ASTERISK";
            case TokenType::NEWLINE:     return "NEWLINE";
            case TokenType::PLAIN_TEXT:  return "PLAIN_TEXT";
            case TokenType::END_OF_FILE: return "$";
            case TokenType::IMG_START:   return "IMG_START";
            case TokenType::R_BRACKET:   return "R_BRACKET";
            case TokenType::L_PAREN:     return "L_PAREN";
            case TokenType::R_PAREN:     return "R_PAREN";
            default:                     return "ERROR";
        }
    }
};
