#pragma once

#include <string>

// Categorías de tokens requeridas por nuestra gramática
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

// Estructura del Token con tipo, lexema y posicionamiento (línea:columna)
struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    // Función auxiliar opcional para imprimir el token de forma amigable (Útil para el Debug)
    std::string toString() const {
        switch (type) {
            case TokenType::HASH:        return "HASH";
            case TokenType::DOUBLE_AST:  return "DOUBLE_AST";
            case TokenType::ASTERISK:    return "ASTERISK";
            case TokenType::NEWLINE:     return "NEWLINE";
            case TokenType::PLAIN_TEXT:  return "PLAIN_TEXT";
            case TokenType::END_OF_FILE: return "EOF";
            case TokenType::TOKEN_ERROR: return "ERROR";
            default:                     return "UNKNOWN";
        }
    }

    // Esta función devuelve el string EXACTO que espera el archivo grammar.txt
    std::string toGrammarString() const {
        switch (type) {
            case TokenType::HASH:        return "HASH";
            case TokenType::DOUBLE_AST:  return "DOUBLE_AST";
            case TokenType::ASTERISK:    return "ASTERISK";
            case TokenType::NEWLINE:     return "NEWLINE";
            case TokenType::PLAIN_TEXT:  return "PLAIN_TEXT";
            case TokenType::END_OF_FILE: return "$";
            // === NUEVOS MAPEOS ===
            case TokenType::IMG_START:   return "IMG_START";
            case TokenType::R_BRACKET:   return "R_BRACKET";
            case TokenType::L_PAREN:     return "L_PAREN";
            case TokenType::R_PAREN:     return "R_PAREN";
            default:                     return "ERROR";
        }
    }
};
