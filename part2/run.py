import json
import os
import subprocess
import urllib.error
import urllib.request

EJECUTABLE = "./transpiler"
ARCHIVO_TEX = "output.tex"
ARCHIVO_PDF = "output.pdf"
FUENTES_CPP = ["main.cpp", "Scanner.cpp", "LR1Parser.cpp", "Grammar.cpp"]

URL_YTOTECH = "https://latex.ytotech.com/builds/sync"


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

    # 3. Validar existencia de output.tex
    if not os.path.exists(ARCHIVO_TEX):
        print(f"Error: No se encontro el archivo {ARCHIVO_TEX}")
        return

    # 4. Enviar el contenido a YtoTech y recibir el PDF
    print(f"Compilando {ARCHIVO_TEX} de forma remota a traves de YtoTech...")
    try:
        with open(ARCHIVO_TEX, "r", encoding="utf-8") as f:
            contenido_latex = f.read()

        # Estructura JSON requerida por la API de YtoTech
        payload = {
            "compiler": "pdflatex",
            "resources": [{"main": True, "content": contenido_latex}],
        }

        # Preparar la petición HTTP POST
        datos_json = json.dumps(payload).encode("utf-8")
        req = urllib.request.Request(
            URL_YTOTECH,
            data=datos_json,
            headers={"Content-Type": "application/json"},
            method="POST",
        )

        # Enviar y guardar la respuesta (el PDF binario)
        with urllib.request.urlopen(req) as response:
            with open(ARCHIVO_PDF, "wb") as f_pdf:
                f_pdf.write(response.read())

        print(f"Proceso finalizado. PDF generado exitosamente: {ARCHIVO_PDF}")

    except urllib.error.HTTPError as e:
        print(f"Error en el servidor de LaTeX (HTTP {e.code}):")
        # Intentar leer el error detallado del JSON de YtoTech si está disponible
        try:
            error_info = json.loads(e.read().decode("utf-8"))
            print(json.dumps(error_info, indent=2))
        except Exception:
            print(e.reason)
    except Exception as e:
        print(f"Ocurrio un error inesperado al conectar con el servidor: {e}")


if __name__ == "__main__":
    main()