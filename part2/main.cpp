#include <iostream>
#include "Scanner.h"
#include "Token.h"

int main() {
    std::cout << "INFO: SCAN Start scanning...\n" << std::endl;

    // Instanciamos nuestro scanner apuntando al archivo de prueba
    Scanner scanner("example.txt");
    Token token;

    // Bucle para pedir tokens uno por uno (emulando la función gettoken() que usará el parser)
    do {
        token = scanner.gettoken();

        // Imprimimos el token con el formato detallado (Debug) requerido por tu PDF
        std::cout << "DEBUG: " << token.toString()
                  << " [ " << token.lexeme << " ] found at ("
                  << token.line << ":" << token.column << ")" << std::endl;

    } while (token.type != TokenType::END_OF_FILE && token.type != TokenType::TOKEN_ERROR);

    if (token.type == TokenType::TOKEN_ERROR) {
        std::cout << "\nINFO: Completed with errors." << std::endl;
    } else {
        std::cout << "\nINFO: Completed with 0 errors" << std::endl;
    }

    return 0;
}
