#include "Scanner.h"
#include <iostream>
#include <sstream>

// Constructor: Abre el archivo y carga todo su contenido en 'sourceCode'
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

// Verifica si ya procesamos todo el archivo
bool Scanner::isAtEnd() const {
    return currentIndex >= sourceCode.length();
}

// Devuelve el carácter actual y avanza el puntero de lectura
char Scanner::getchar() {
    if (isAtEnd()) return '\0';

    char c = sourceCode[currentIndex];
    currentIndex++;

    // Si es un salto de línea, aumentamos el contador de líneas y reiniciamos columna
    if (c == '\n') {
        currentLine++;
        currentColumn = 1;
    } else {
        currentColumn++;
    }

    return c;
}

// Mira el siguiente carácter sin mover el puntero de lectura
char Scanner::peekchar() {
    if (isAtEnd()) return '\0';
    return sourceCode[currentIndex];
}

// Motor principal
Token Scanner::gettoken() {
    // 1. Si llegamos al final del archivo, devolvemos EOF
    if (isAtEnd()) {
        return Token{TokenType::END_OF_FILE, "EOF", currentLine, currentColumn};
    }

    // Guardamos la posición exacta donde inicia ESTE token para el reporte
    int tokenLine = currentLine;
    int tokenColumn = currentColumn;

    // Leemos el carácter actual
    char c = getchar();

    // CASO A: Saltos de línea
    if (c == '\n') {
        return Token{TokenType::NEWLINE, "\\n", tokenLine, tokenColumn};
    }

    // CASO B: El encabezado '#'
    if (c == '#') {
        return Token{TokenType::HASH, "#", tokenLine, tokenColumn};
    }

    // CASO C: Asteriscos (Negrita '**' o Cursiva '*')
    if (c == '*') {
        if (peekchar() == '*') {
            getchar(); // Consumimos el segundo asterisco
            return Token{TokenType::DOUBLE_AST, "**", tokenLine, tokenColumn};
        }
        return Token{TokenType::ASTERISK, "*", tokenLine, tokenColumn};
    }

    // CASO D: Texto Plano (PLAIN_TEXT)
    // Si llegamos aquí, 'c' es el inicio de una secuencia de texto libre.
    std::string lexeme = "";
    lexeme += c; // Agregamos el primer carácter ya leído

    // Mientras el siguiente carácter NO sea el fin del archivo
    // Y TAMPOCO sea un token especial (#, *, \n), lo acumulamos.
    while (!isAtEnd()) {
        char next = peekchar();

        if (next == '\n' || next == '#' || next == '*') {
            break; // Detenemos la acumulación para que el siguiente ciclo maneje la marca
        }

        // Si es un carácter normal, lo consumimos oficialmente y lo sumamos al lexema
        lexeme += getchar();
    }

    return Token{TokenType::PLAIN_TEXT, lexeme, tokenLine, tokenColumn};
}
