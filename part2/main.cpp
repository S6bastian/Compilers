#include "Grammar.h"
#include "LR1Parser.h"
#include "Scanner.h"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() {
    // 1. Definimos la gramática exacta de tu procesador de texto (Markdown a LaTeX)
    vector<string> rawGrammar = {
        "DOCUMENT -> BLOCKLIST",
        "BLOCKLIST -> BLOCKLIST BLOCK",
        "BLOCKLIST -> BLOCK",
        "BLOCK -> HEADING",
        "BLOCK -> PARAGRAPH",
        "HEADING -> HASH TEXT NEWLINE",
        "PARAGRAPH -> TEXT NEWLINE",
        "TEXT -> TEXT ELEMENT",
        "TEXT -> ELEMENT",
        "ELEMENT -> BOLD",
        "ELEMENT -> ITALICS",
        "ELEMENT -> PLAIN_TEXT",
        "BOLD -> DOUBLE_AST PLAIN_TEXT DOUBLE_AST",
        "ITALICS -> ASTERISK PLAIN_TEXT ASTERISK"
    };

    // Inicializamos la estructura de la gramática
    Grammar myGrammar(rawGrammar);

    cout << "=== [Fase 1] Construyendo Tablas de Analisis LR(1) ===" << endl;
    LR1Parser lr1(&myGrammar);
    cout << "-> ¡Tablas LR(1) generadas con exito!\n" << endl;

    // 2. Inicializamos el Scanner apuntando a tu archivo real de pruebas
    string archivoPrueba = "example.txt";
    cout << "=== [Fase 2] Inicializando Scanner con '" << archivoPrueba << "' ===" << endl;
    Scanner scanner(archivoPrueba);

    // 3. Extraemos todos los tokens del archivo usando gettoken()
    vector<Token> listaTokens;
    Token t = scanner.gettoken();

    while (t.type != TokenType::END_OF_FILE && t.type != TokenType::TOKEN_ERROR) {
        listaTokens.push_back(t);
        t = scanner.gettoken();
    }

    // Si el ciclo terminó por un error léxico, avisamos
    if (t.type == TokenType::TOKEN_ERROR) {
        cout << "[Error Lexico] Se detecto un token invalido en la linea "
             << t.line << ", columna " << t.column << "." << endl;
        return 1;
    }

    cout << "-> Scanner finalizado. Se encontraron " << listaTokens.size() << " tokens validos." << endl;

    // Mostramos la cadena de tokens formateada para debug
    cout << "[Tokens detectados]: ";
    for (const auto& tok : listaTokens) {
        cout << tok.toGrammarString() << " ";
    }
    cout << "$\n" << endl;

    // 4. Ejecutamos el Parser LR(1) pasándole el vector con los objetos Token completos
    cout << "=== [Fase 3] Ejecutando Analisis Sintactico LR(1) ===" << endl;
    bool success = lr1.parse(listaTokens);

    if (success) {
        cout << "-> ¡Analisis sintactico exitoso! El documento es estructuralmente valido." << endl;
        cout << "-> El arbol de derivacion sintactica (Parse Tree) ha sido construido en memoria.\n" << endl;

        // === [FASE FINAL: TRANSPILACIÓN DESDE EL PARSER] ===
        cout << "=== [Fase 4] Generando Codigo LaTeX desde LR1Parser ===" << endl;

        // Llamamos directamente al método interno de tu parser
        string codigoLatex = lr1.generateLatex();

        // Imprimimos el resultado en la consola para validar
        cout << "\n================ SALIDA LATEX EN CONSOLA ================\n";
        cout << codigoLatex;
        cout << "=========================================================\n";

        // Guardamos el código resultante en el archivo output.tex
        ofstream archivoSalida("output.tex");
        if (archivoSalida.is_open()) {
            archivoSalida << codigoLatex;
            archivoSalida.close();
            cout << "\n-> [Transpiler] ¡Exito! Archivo 'output.tex' guardado correctamente." << endl;
        } else {
            cout << "\n[Error] No se pudo escribir el archivo de salida." << endl;
        }

    } else {
        cout << "\n[Error Sintactico] La secuencia de tokens no respeta las reglas de la gramatica." << endl;
    }

    return 0;
}
