#pragma once

#include <string>
#include <fstream>
#include "Token.h"

class Scanner {
private:
    std::string sourceCode; // Aquí guardaremos todo el contenido del archivo
    size_t currentIndex;    // Índice de la posición actual en el string
    int currentLine;        // Línea actual (para el reporte de errores y tokens)
    int currentColumn;      // Columna actual

    // Métodos auxiliares privados para leer el string interno
    char getchar();
    char peekchar();
    bool isAtEnd() const;

public:
    // El constructor recibirá la ruta del archivo (ej. "example.txt")
    Scanner(const std::string& filePath);

    // El método estrella que invocará tu Parser LR(1)
    Token gettoken();
};
