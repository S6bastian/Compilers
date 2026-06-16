import os
import subprocess

EJECUTABLE = "./transpiler"
ARCHIVO_TEX = "output.tex"
FUENTES_CPP = ["main.cpp", "Scanner.cpp", "LR1Parser.cpp", "Grammar.cpp"]


def main():
    # 1. Compilar el ejecutable de C++
    archivos_cpp = " ".join(FUENTES_CPP)
    comando_compilar = f"g++ -std=c++17 {archivos_cpp} -o {EJECUTABLE}"
    res_compilar = subprocess.run(
        comando_compilar, shell=True, capture_output=True, text=True
    )

    if res_compilar.returncode != 0:
        print("Error de compilacion en C++:")
        print(res_compilar.stderr)
        return

    # 2. Ejecutar el transpiler para generar el archivo .tex
    res_transpiler = subprocess.run(
        EJECUTABLE, shell=True, capture_output=True, text=True
    )
    if res_transpiler.returncode != 0:
        print("Error durante la ejecucion del transpiler:")
        print(res_transpiler.stderr)
        return

    # 3. Validar existencia de output.tex y compilarlo con pdflatex
    if not os.path.exists(ARCHIVO_TEX):
        print(f"Error: No se encontro el archivo {ARCHIVO_TEX}")
        return

    # Ejecuta pdflatex de forma limpia sobre el archivo puro de C++
    comando_latex = f"pdflatex -interaction=nonstopmode {ARCHIVO_TEX}"
    res_latex = subprocess.run(
        comando_latex, shell=True, capture_output=True, text=True
    )

    if res_latex.returncode != 0:
        print("Error en la compilacion de LaTeX:")
        print(res_latex.stderr)
        return

    # 4. Limpieza de archivos auxiliares (.aux, .log, etc.)
    nombre_base = os.path.splitext(ARCHIVO_TEX)[0]
    for ext in [".aux", ".log", ".toc", ".out"]:
        archivo_basura = nombre_base + ext
        if os.path.exists(archivo_basura):
            os.remove(archivo_basura)

    print("Proceso finalizado. PDF generado correctamente.")


if __name__ == "__main__":
    main()
