#include "Grammar.h"
#include "LR1Parser.h"
#include "Scanner.h"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() {

    vector<string> rawGrammar = {
        "DOCUMENT -> BLOCKLIST",
        "BLOCKLIST -> BLOCKLIST BLOCK",
        "BLOCKLIST -> BLOCK",
        "BLOCK -> HEADING",
        "BLOCK -> PARAGRAPH",
        "BLOCK -> IMAGE",
        "HEADING -> HASH TEXT NEWLINE",
        "PARAGRAPH -> TEXT NEWLINE",
        "TEXT -> TEXT ELEMENT",
        "TEXT -> ELEMENT",
        "ELEMENT -> BOLD",
        "ELEMENT -> ITALICS",
        "ELEMENT -> PLAIN_TEXT",
        "BOLD -> DOUBLE_AST PLAIN_TEXT DOUBLE_AST",
        "ITALICS -> ASTERISK PLAIN_TEXT ASTERISK",
        "IMAGE -> IMG_START TEXT R_BRACKET L_PAREN PLAIN_TEXT R_PAREN NEWLINE"
    };


    Grammar myGrammar(rawGrammar);

    cout << "=== [Fase 1] Construyendo Tablas de Analisis LR(1) ===" << endl;
    LR1Parser lr1(&myGrammar);
    cout << "-> ¡Tablas LR(1) generadas con exito!\n" << endl;


    string archivoPrueba = "example.txt";
    cout << "=== [Fase 2] Inicializando Scanner con '" << archivoPrueba << "' ===" << endl;
    Scanner scanner(archivoPrueba);


    vector<Token> listaTokens;
    Token t = scanner.gettoken();


    cout << "INFO\tSCAN - Start scanning...\n";
    while (t.type != TokenType::END_OF_FILE && t.type != TokenType::TOKEN_ERROR) {
        cout << "DEBUG\tSCAN - " << t.toGrammarString() << " at ("
             << t.line << ":" << t.column << ")\n";
        listaTokens.push_back(t);
        t = scanner.gettoken();
    }


    if (t.type == TokenType::TOKEN_ERROR) {
        cout << "DEBUG\tSCAN - TOKEN ERROR at ("
             << t.line << ":" << t.column << ")\n";
        return 1;
    }

    cout << "-> Scanner finalizado. Se encontraron " << listaTokens.size() << " tokens validos." << endl;


    cout << "[Tokens detectados]: ";
    for (const auto& tok : listaTokens) {
        cout << tok.toGrammarString() << " ";
    }
    cout << "$\n" << endl;


    cout << "=== [Fase 3] Ejecutando Analisis Sintactico LR(1) ===" << endl;
    bool success = lr1.parse(listaTokens);

    if (success) {
        cout << "-> ¡Analisis sintactico exitoso! El documento es estructuralmente valido." << endl;
        cout << "-> El arbol de derivacion sintactica (Parse Tree) ha sido construido en memoria.\n" << endl;


        cout << "=== [Fase 4] Generando Codigo LaTeX desde LR1Parser ===" << endl;


        string codigoLatex = lr1.generateLatex();


        cout << "\n================ SALIDA LATEX EN CONSOLA ================\n";
        cout << codigoLatex;
        cout << "=========================================================\n";


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
