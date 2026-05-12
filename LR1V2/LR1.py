import json
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from tkinter.scrolledtext import ScrolledText
import subprocess
import os
import re
from pathlib import Path

class LR1GrammarApp:
    def __init__(self, root):
        self.root = root
        self.root.title("LR(1) Parser - Grammar Processor")
        self.root.geometry("1400x800")
        
        # Variables
        self.grammar_file = None
        self.canonical_file = None
        self.table_file = None
        
        # Configurar estilos
        self.setup_styles()
        
        # Crear interfaz
        self.create_widgets()
        
    def setup_styles(self):
        """Configurar estilos de la interfaz"""
        style = ttk.Style()
        style.theme_use('clam')
        
        # Configurar colores
        self.bg_color = "#2b2b2b"
        self.fg_color = "#f8f8f2"
        self.accent_color = "#6272a4"
        self.error_color = "#ff5555"
        self.success_color = "#50fa7b"
        
        self.root.configure(bg=self.bg_color)
        
    def create_widgets(self):
        """Crear todos los widgets de la interfaz"""
        # Frame principal con paneles
        self.main_paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        self.main_paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Panel izquierdo - Entrada de gramática
        self.create_input_panel()
        
        # Panel derecho - Visualización
        self.create_output_panel()
        
    def create_input_panel(self):
        """Panel para entrada de gramática"""
        left_frame = ttk.Frame(self.main_paned)
        self.main_paned.add(left_frame, weight=1)
        
        # Título
        title_label = tk.Label(left_frame, text="GRAMMAR INPUT", 
                               font=("Arial", 14, "bold"),
                               bg=self.bg_color, fg=self.accent_color)
        title_label.pack(pady=10)
        
        # Frame para el editor de gramática
        editor_frame = ttk.LabelFrame(left_frame, text="Grammar Rules")
        editor_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        # Área de texto para gramática
        self.grammar_text = ScrolledText(editor_frame, height=15, width=50,
                                        font=("Courier", 10),
                                        bg="#1e1e1e", fg=self.fg_color,
                                        insertbackground=self.fg_color)
        self.grammar_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Insertar gramática de ejemplo
        example_grammar = """P' → P
P → ( P )
P → a

# Terminales: a, (, )
# No terminales: P, P'
# Símbolo inicial: P'"""
        self.grammar_text.insert("1.0", example_grammar)
        
        # Botones
        button_frame = ttk.Frame(left_frame)
        button_frame.pack(pady=10)
        
        self.process_btn = tk.Button(button_frame, text="PROCESS GRAMMAR", 
                                     command=self.process_grammar,
                                     bg=self.accent_color, fg="white",
                                     font=("Arial", 10, "bold"),
                                     padx=20, pady=5)
        self.process_btn.pack(side=tk.LEFT, padx=5)
        
        self.load_json_btn = tk.Button(button_frame, text="LOAD JSON FILES",
                                       command=self.load_json_files,
                                       bg="#44475a", fg="white",
                                       font=("Arial", 10, "bold"),
                                       padx=20, pady=5)
        self.load_json_btn.pack(side=tk.LEFT, padx=5)
        
        # Frame para archivos generados
        files_frame = ttk.LabelFrame(left_frame, text="Generated Files")
        files_frame.pack(fill=tk.X, padx=10, pady=5)
        
        self.files_listbox = tk.Listbox(files_frame, height=4,
                                        bg="#1e1e1e", fg=self.fg_color)
        self.files_listbox.pack(fill=tk.X, padx=5, pady=5)
        
    def create_output_panel(self):
        """Panel para visualización de resultados"""
        right_frame = ttk.Frame(self.main_paned)
        self.main_paned.add(right_frame, weight=2)
        
        # Notebook para pestañas
        self.notebook = ttk.Notebook(right_frame)
        self.notebook.pack(fill=tk.BOTH, expand=True)
        
        # Pestaña: Gramática formateada
        self.create_grammar_tab()
        
        # Pestaña: Canonical Collection
        self.create_canonical_tab()
        
        # Pestaña: LR Table
        self.create_lr_table_tab()
        
        # Pestaña: Trace (simulación)
        self.create_trace_tab()
        
    def create_grammar_tab(self):
        """Pestaña para mostrar gramática formateada"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="📖 Grammar")
        
        # Área de texto con scroll
        self.grammar_display = ScrolledText(tab, font=("Courier", 10),
                                           bg="#1e1e1e", fg=self.fg_color)
        self.grammar_display.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
    def create_canonical_tab(self):
        """Pestaña para mostrar Canonical Collection"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="🔍 Canonical Collection")
        
        # Treeview para mostrar estados
        self.canonical_tree = ttk.Treeview(tab, columns=("ID", "Items"), show="tree headings")
        self.canonical_tree.heading("#0", text="")
        self.canonical_tree.heading("ID", text="State ID")
        self.canonical_tree.heading("Items", text="LR(1) Items")
        self.canonical_tree.column("#0", width=0, stretch=False)
        self.canonical_tree.column("ID", width=80)
        self.canonical_tree.column("Items", width=600)
        
        # Scrollbars
        v_scroll = ttk.Scrollbar(tab, orient="vertical", command=self.canonical_tree.yview)
        h_scroll = ttk.Scrollbar(tab, orient="horizontal", command=self.canonical_tree.xview)
        self.canonical_tree.configure(yscrollcommand=v_scroll.set, xscrollcommand=h_scroll.set)
        
        self.canonical_tree.grid(row=0, column=0, sticky="nsew")
        v_scroll.grid(row=0, column=1, sticky="ns")
        h_scroll.grid(row=1, column=0, sticky="ew")
        
        tab.grid_rowconfigure(0, weight=1)
        tab.grid_columnconfigure(0, weight=1)
        
    def create_lr_table_tab(self):
        """Pestaña para mostrar LR Table"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="📊 LR(1) Table")
        
        # Frame para tabla
        table_frame = ttk.Frame(tab)
        table_frame.pack(fill=tk.BOTH, expand=True)
        
        # Canvas con scroll para tabla grande
        canvas = tk.Canvas(table_frame, bg=self.bg_color)
        h_scroll = ttk.Scrollbar(table_frame, orient="horizontal", command=canvas.xview)
        v_scroll = ttk.Scrollbar(table_frame, orient="vertical", command=canvas.yview)
        
        self.table_container = ttk.Frame(canvas)
        self.table_container.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        
        canvas.create_window((0, 0), window=self.table_container, anchor="nw")
        canvas.configure(xscrollcommand=h_scroll.set, yscrollcommand=v_scroll.set)
        
        canvas.grid(row=0, column=0, sticky="nsew")
        v_scroll.grid(row=0, column=1, sticky="ns")
        h_scroll.grid(row=1, column=0, sticky="ew")
        
        table_frame.grid_rowconfigure(0, weight=1)
        table_frame.grid_columnconfigure(0, weight=1)
        
        self.lr_table_labels = {}  # Para almacenar labels de la tabla
        
    def create_trace_tab(self):
        """Pestaña para simulación y trace"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="⚙️ Parse Trace")
        
        # Frame de entrada
        input_frame = ttk.Frame(tab)
        input_frame.pack(fill=tk.X, padx=10, pady=10)
        
        ttk.Label(input_frame, text="Input String:").pack(side=tk.LEFT, padx=5)
        self.input_entry = ttk.Entry(input_frame, width=40, font=("Courier", 10))
        self.input_entry.pack(side=tk.LEFT, padx=5)
        self.input_entry.insert(0, "( a )")
        
        self.parse_btn = tk.Button(input_frame, text="PARSE", 
                                   command=self.parse_input,
                                   bg=self.success_color, fg="black",
                                   font=("Arial", 9, "bold"))
        self.parse_btn.pack(side=tk.LEFT, padx=10)
        
        self.steps_spinbox = ttk.Spinbox(input_frame, from_=1, to=100, width=10)
        self.steps_spinbox.set(50)
        self.steps_spinbox.pack(side=tk.LEFT, padx=5)
        ttk.Label(input_frame, text="max steps").pack(side=tk.LEFT)
        
        # Área de trace
        self.trace_text = ScrolledText(tab, font=("Courier", 9),
                                      bg="#1e1e1e", fg=self.fg_color,
                                      height=20)
        self.trace_text.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
    def process_grammar(self):
        """Procesar gramática ingresada"""
        grammar = self.grammar_text.get("1.0", tk.END).strip()
        
        if not grammar:
            messagebox.showerror("Error", "Please enter a grammar")
            return
        
        # Guardar gramática en archivo
        self.grammar_file = "grammar.txt"
        with open(self.grammar_file, "w", encoding="utf-8") as f:
            f.write(grammar)
        
        # Formatear y mostrar gramática
        self.display_formatted_grammar(grammar)
        
        # Simular procesamiento (aquí llamarías a tu parser en C++)
        # Por ahora, creamos JSONs de ejemplo si no existen
        self.simulate_parser_output()
        
        # Cargar y mostrar los JSONs
        self.load_and_display_jsons()
        
        messagebox.showinfo("Success", "Grammar processed successfully!")
        
    def display_formatted_grammar(self, grammar):
        """Mostrar gramática formateada"""
        self.grammar_display.delete("1.0", tk.END)
        
        # Formatear gramática
        lines = grammar.split('\n')
        formatted = []
        formatted.append("=" * 60)
        formatted.append("LR(1) GRAMMAR")
        formatted.append("=" * 60)
        
        rule_num = 0
        for line in lines:
            line = line.strip()
            if line and not line.startswith('#'):
                if '→' in line:
                    formatted.append(f"| ({rule_num}) | {line} |")
                    rule_num += 1
                elif line:
                    formatted.append(f"|   | {line} |")
            elif line.startswith('#'):
                formatted.append(f"|   | {line[1:].strip()} |")
        
        formatted.append("=" * 60)
        
        self.grammar_display.insert("1.0", "\n".join(formatted))
        
    def simulate_parser_output(self):
        """Simular salida del parser (reemplazar con llamada real a C++)"""
        # Crear archivos JSON de ejemplo si no existen
        if not os.path.exists("canonical_collection.json"):
            # Este es el JSON que proporcionaste
            canonical_data = {
                "states": [
                    {"id": 0, "items": [{"head": "P'", "body": ["P"], "dot": 0, "lookahead": ["$"]},
                                       {"head": "P", "body": ["(", "P", ")"], "dot": 0, "lookahead": ["$"]},
                                       {"head": "P", "body": ["a"], "dot": 0, "lookahead": ["$"]}]},
                    {"id": 1, "items": [{"head": "P", "body": ["(", "P", ")"], "dot": 1, "lookahead": ["$"]},
                                       {"head": "P", "body": ["(", "P", ")"], "dot": 0, "lookahead": [")"]},
                                       {"head": "P", "body": ["a"], "dot": 0, "lookahead": [")"]}]},
                    {"id": 2, "items": [{"head": "P'", "body": ["P"], "dot": 1, "lookahead": ["$"]}]}
                ]
            }
            with open("canonical_collection.json", "w") as f:
                json.dump(canonical_data, f, indent=2)
        
        if not os.path.exists("lr1_table.json"):
            table_data = {
                "states": [
                    {"id": 0, "action": {"(": "s1", "a": "s3"}, "goto": {"P": 2}},
                    {"id": 1, "action": {"(": "s4", "a": "s6"}, "goto": {"P": 5}},
                    {"id": 2, "action": {"$": "acc"}, "goto": {}},
                    {"id": 3, "action": {"$": "r2"}, "goto": {}}
                ]
            }
            with open("lr1_table.json", "w") as f:
                json.dump(table_data, f, indent=2)
        
        # Actualizar lista de archivos
        self.update_files_list()
        
    def load_json_files(self):
        """Cargar archivos JSON existentes"""
        self.canonical_file = filedialog.askopenfilename(
            title="Select Canonical Collection JSON",
            filetypes=[("JSON files", "*.json")]
        )
        
        self.table_file = filedialog.askopenfilename(
            title="Select LR Table JSON",
            filetypes=[("JSON files", "*.json")]
        )
        
        if self.canonical_file and self.table_file:
            self.load_and_display_jsons()
            
    def load_and_display_jsons(self):
        """Cargar y mostrar ambos JSONs"""
        # Cargar Canonical Collection
        if os.path.exists("canonical_collection.json"):
            with open("canonical_collection.json", "r") as f:
                canonical_data = json.load(f)
                self.display_canonical_collection(canonical_data)
        
        # Cargar LR Table
        if os.path.exists("lr1_table.json"):
            with open("lr1_table.json", "r") as f:
                table_data = json.load(f)
                self.display_lr_table(table_data)
                
        self.update_files_list()
        
    def display_canonical_collection(self, data):
        """Mostrar Canonical Collection en treeview"""
        # Limpiar treeview
        for item in self.canonical_tree.get_children():
            self.canonical_tree.delete(item)
        
        for state in data['states']:
            state_id = state['id']
            items_text = []
            for item in state['items']:
                # Formatear item
                body = item['body']
                dot_pos = item['dot']
                body_with_dot = body[:dot_pos] + ['•'] + body[dot_pos:]
                prod = f"{item['head']} → {' '.join(body_with_dot)}"
                lookahead = f"[{', '.join(item['lookahead'])}]"
                items_text.append(f"{prod}  {lookahead}")
            
            items_display = "\n".join(items_text)
            self.canonical_tree.insert("", "end", values=(state_id, items_display))
            
    def display_lr_table(self, data):
        """Mostrar LR Table como grid"""
        # Limpiar container
        for widget in self.table_container.winfo_children():
            widget.destroy()
        
        # Obtener todos los símbolos
        all_actions = set()
        all_gotos = set()
        for state in data['states']:
            all_actions.update(state['action'].keys())
            all_gotos.update(state['goto'].keys())
        
        # Ordenar terminales y no terminales
        terminals = sorted([t for t in all_actions if t != '$'])
        terminals.append('$')
        nonterminals = sorted(all_gotos)
        headers = ['State'] + terminals + nonterminals
        
        # Crear tabla con Labels
        for i, header in enumerate(headers):
            label = tk.Label(self.table_container, text=header,
                           font=("Arial", 10, "bold"),
                           bg=self.accent_color, fg="white",
                           padx=10, pady=5, relief="ridge")
            label.grid(row=0, column=i, sticky="nsew")
        
        # Llenar datos
        for state in sorted(data['states'], key=lambda x: x['id']):
            row_idx = state['id'] + 1
            # State ID
            state_label = tk.Label(self.table_container, text=f"{state['id']}",
                                  font=("Courier", 9),
                                  bg="#44475a", fg=self.fg_color,
                                  padx=5, pady=3, relief="ridge")
            state_label.grid(row=row_idx, column=0, sticky="nsew")
            
            # Actions
            for col_idx, symbol in enumerate(terminals, start=1):
                value = state['action'].get(symbol, "")
                if value:
                    color = self.success_color if value == "acc" else self.accent_color
                    label = tk.Label(self.table_container, text=value,
                                   font=("Courier", 9),
                                   bg="#2b2b2b", fg=color,
                                   padx=5, pady=3, relief="ridge")
                else:
                    label = tk.Label(self.table_container, text="",
                                   bg="#2b2b2b", padx=5, pady=3, relief="ridge")
                label.grid(row=row_idx, column=col_idx, sticky="nsew")
            
            # Gotos
            for col_idx, symbol in enumerate(nonterminals, start=len(terminals)+1):
                value = state['goto'].get(symbol, "")
                if value:
                    label = tk.Label(self.table_container, text=str(value),
                                   font=("Courier", 9),
                                   bg="#2b2b2b", fg="#ffb86c",
                                   padx=5, pady=3, relief="ridge")
                else:
                    label = tk.Label(self.table_container, text="",
                                   bg="#2b2b2b", padx=5, pady=3, relief="ridge")
                label.grid(row=row_idx, column=col_idx, sticky="nsew")
        
        # Configurar pesos de columnas
        for i in range(len(headers)):
            self.table_container.grid_columnconfigure(i, weight=1)
            
    def parse_input(self):
        """Simular parsing del input"""
        input_string = self.input_entry.get().strip()
        max_steps = int(self.steps_spinbox.get())
        
        self.trace_text.delete("1.0", tk.END)
        
        # Simular trace
        trace = []
        trace.append("=" * 80)
        trace.append("LR(1) PARSING TRACE")
        trace.append("=" * 80)
        trace.append(f"{'Step':<6} {'Stack':<30} {'Input':<30} {'Action':<20}")
        trace.append("-" * 80)
        
        # Simulación simple
        tokens = input_string.split()
        stack = [0]
        pos = 0
        
        for step in range(1, max_steps + 1):
            if pos >= len(tokens):
                remaining = "$"
            else:
                remaining = " ".join(tokens[pos:]) + " $"
            
            stack_str = " ".join(str(s) for s in stack)
            trace.append(f"{step:<6} {stack_str:<30} {remaining:<30} ...")
            
            if step >= 10:  # Simulación terminada
                trace.append("-" * 80)
                trace.append("✓ Parse completed successfully!")
                break
        
        self.trace_text.insert("1.0", "\n".join(trace))
        
    def update_files_list(self):
        """Actualizar lista de archivos generados"""
        self.files_listbox.delete(0, tk.END)
        
        files = ["grammar.txt", "canonical_collection.json", "lr1_table.json"]
        for file in files:
            if os.path.exists(file):
                self.files_listbox.insert(tk.END, f"✓ {file}")
            else:
                self.files_listbox.insert(tk.END, f"✗ {file}")

def main():
    root = tk.Tk()
    app = LR1GrammarApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()