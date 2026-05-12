import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox, font
import subprocess
import json
import os
import math

# ─────────────────────────────────────────────────────────────────────────────
# THEME
# ─────────────────────────────────────────────────────────────────────────────
BG         = "#f5f5f0"
PANEL      = "#ffffff"
BORDER     = "#c8c8c0"
ACCENT     = "#2e6da4"
ACCENT2    = "#1a4a72"
GREEN      = "#2a7a3b"
RED        = "#b03030"
TEXT       = "#1a1a1a"
TEXT_DIM   = "#666660"
MONO       = ("Courier New", 10)
MONO_SM    = ("Courier New", 9)
SANS       = ("Helvetica", 10)
SANS_B     = ("Helvetica", 10, "bold")
SANS_SM    = ("Helvetica", 9)
TITLE_F    = ("Helvetica", 11, "bold")

GRAMMAR_DEFAULT = "P -> ( P )\nP -> a"
EXE_NAME        = "lr1.exe"

# ─────────────────────────────────────────────────────────────────────────────
# HELPERS
# ─────────────────────────────────────────────────────────────────────────────
def load_json(path):
    if not os.path.exists(path):
        return None
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def write_grammar(text):
    clean = []
    for line in text.splitlines():
        line = line.strip()
        if line:
            clean.append(line)
    with open("grammar.txt", "w", encoding="ascii", errors="ignore") as f:
        f.write("\n".join(clean) + "\n")


def run_parser(input_tokens):
    """Run lr1.exe with the given token string, return (stdout, stderr, returncode)."""
    if not os.path.exists(EXE_NAME):
        return "", f"Executable '{EXE_NAME}' not found in current directory.", 1
    try:
        proc = subprocess.run(
            [EXE_NAME],
            input=input_tokens + "\nquit\n",
            capture_output=True,
            text=True,
            timeout=10
        )
        return proc.stdout, proc.stderr, proc.returncode
    except subprocess.TimeoutExpired:
        return "", "Timeout: executable took too long.", 1
    except Exception as e:
        return "", str(e), 1


# ─────────────────────────────────────────────────────────────────────────────
# TREEVIEW HELPER
# ─────────────────────────────────────────────────────────────────────────────
def clear_treeview(tv):
    for item in tv.get_children():
        tv.delete(item)


def style_treeview(tv, columns, headings, col_widths=None):
    tv["columns"] = columns
    tv["show"] = "headings"
    for i, col in enumerate(columns):
        tv.heading(col, text=headings[i], anchor="w")
        w = col_widths[i] if col_widths else 100
        tv.column(col, width=w, minwidth=40, anchor="w")


# ─────────────────────────────────────────────────────────────────────────────
# PARSE TREE CANVAS DRAWING
# ─────────────────────────────────────────────────────────────────────────────
NODE_R  = 18
H_GAP   = 40
V_GAP   = 54

def tree_layout(node, depth=0, counter=[0]):
    """Assign (x_index, depth) to every node via in-order traversal."""
    if not node.get("children"):
        node["_x"] = counter[0]
        node["_y"] = depth
        counter[0] += 1
    else:
        for child in node["children"]:
            tree_layout(child, depth + 1, counter)
        xs = [c["_x"] for c in node["children"]]
        node["_x"] = (xs[0] + xs[-1]) / 2.0
        node["_y"] = depth


def draw_tree(canvas, node, x_scale, x_off, y_off):
    cx = int(node["_x"] * x_scale + x_off)
    cy = int(node["_y"] * V_GAP + y_off)

    # draw edges first
    for child in node.get("children", []):
        ccx = int(child["_x"] * x_scale + x_off)
        ccy = int(child["_y"] * V_GAP + y_off)
        canvas.create_line(cx, cy + NODE_R, ccx, ccy - NODE_R,
                           fill=BORDER, width=1)
        draw_tree(canvas, child, x_scale, x_off, y_off)

    # terminal vs non-terminal
    sym = node["symbol"]
    is_term = not node.get("children")
    fill  = PANEL if not is_term else "#e8f4e8"
    outline = ACCENT if not is_term else GREEN
    lw = 2 if not is_term else 1

    canvas.create_oval(cx - NODE_R, cy - NODE_R, cx + NODE_R, cy + NODE_R,
                       fill=fill, outline=outline, width=lw)
    canvas.create_text(cx, cy, text=sym, font=MONO_SM, fill=TEXT)


def render_tree_canvas(canvas, tree_data):
    canvas.delete("all")
    if not tree_data:
        canvas.create_text(200, 80, text="No parse tree data.", font=SANS, fill=TEXT_DIM)
        return

    root = tree_data.get("parseTree") or tree_data
    counter = [0]
    tree_layout(root, 0, counter)
    leaves = counter[0]

    x_scale = max(H_GAP, H_GAP)
    x_off   = 30
    y_off   = 30

    total_w = leaves * x_scale + 2 * x_off
    total_h = (root["_y"] + 2) * V_GAP + y_off   # root _y is 0 but children go deeper

    # recalculate real depth
    def max_depth(n):
        if not n.get("children"):
            return n["_y"]
        return max(max_depth(c) for c in n["children"])

    depth = max_depth(root)
    total_h = (depth + 1) * V_GAP + 2 * y_off

    canvas.config(scrollregion=(0, 0, max(total_w, 400), max(total_h, 300)))
    draw_tree(canvas, root, x_scale, x_off, y_off)


# ─────────────────────────────────────────────────────────────────────────────
# FIRST SETS TAB
# ─────────────────────────────────────────────────────────────────────────────
def build_first_tab(parent):
    frame = tk.Frame(parent, bg=BG)
    frame.pack(fill="both", expand=True)
    
    tk.Label(frame, text="FIRST Sets", bg=BG,
             font=TITLE_F, fg=ACCENT2, anchor="w").pack(fill="x", padx=10, pady=(8, 2))
    
    container = tk.Frame(frame, bg=PANEL, relief="solid", bd=1,
                         highlightthickness=1, highlightbackground=BORDER)
    container.pack(fill="both", expand=True, padx=8, pady=6)
    
    canvas = tk.Canvas(container, bg=PANEL, highlightthickness=0)
    vsb = ttk.Scrollbar(container, orient="vertical", command=canvas.yview)
    canvas.configure(yscrollcommand=vsb.set)
    
    vsb.pack(side="right", fill="y")
    canvas.pack(side="left", fill="both", expand=True)
    
    inner_frame = tk.Frame(canvas, bg=PANEL)
    # Importante: usar window=... y anclarlo
    canvas_window = canvas.create_window((0, 0), window=inner_frame, anchor="nw")
    
    def configure_scroll_region(event):
        canvas.configure(scrollregion=canvas.bbox("all"))
        # Esto hace que el frame interno crezca con el canvas
        canvas.itemconfig(canvas_window, width=event.width)
    
    canvas.bind("<Configure>", configure_scroll_region)
    
    def refresh():
        for widget in inner_frame.winfo_children():
            widget.destroy()
        
        data = load_json("first_sets.json")
        if not data or "firstSets" not in data:
            tk.Label(inner_frame, text="No FIRST sets data available. Run parser first.",
                    bg=PANEL, fg=RED, font=SANS, pady=20).pack()
            return
        
        for nt, symbols in data["firstSets"].items():
            nt_frame = tk.Frame(inner_frame, bg=PANEL, relief="solid", bd=1, pady=2)
            nt_frame.pack(fill="x", padx=10, pady=5)
            
            title_bg = ACCENT if nt.startswith("P'") else ACCENT2
            tk.Label(nt_frame, text=f"FIRST({nt})", bg=title_bg, fg="white", 
                     font=SANS_B, anchor="w", padx=10).pack(fill="x")
            
            content = tk.Frame(nt_frame, bg=PANEL)
            content.pack(fill="x", padx=10, pady=5)
            
            for sym in symbols:
                lbl = tk.Label(content, text=sym, bg="#e8f4e8", fg=GREEN,
                               font=MONO_SM, relief="solid", bd=1, padx=5)
                lbl.pack(side="left", padx=2)

    frame._refresh = refresh
    return frame


# ─────────────────────────────────────────────────────────────────────────────
# CANONICAL COLLECTION TAB
# ─────────────────────────────────────────────────────────────────────────────
def build_canonical_tab(parent):
    frame = tk.Frame(parent, bg=BG)
    frame.pack(fill="both", expand=True)

    label = tk.Label(frame, text="Canonical LR(1) Collection", bg=BG,
                     font=TITLE_F, fg=ACCENT2, anchor="w")
    label.pack(fill="x", padx=10, pady=(8, 2))

    # left: state list
    pane = tk.PanedWindow(frame, orient="horizontal", bg=BG, sashwidth=5,
                          sashrelief="flat")
    pane.pack(fill="both", expand=True, padx=8, pady=6)

    left = tk.Frame(pane, bg=PANEL, relief="solid", bd=1,
                    highlightthickness=1, highlightbackground=BORDER)
    pane.add(left, minsize=130)

    tk.Label(left, text="States", bg=PANEL, font=SANS_B, fg=TEXT,
             anchor="w", padx=6, pady=4).pack(fill="x")
    sep = tk.Frame(left, bg=BORDER, height=1)
    sep.pack(fill="x")

    state_lb_var = tk.Variable(value=[])
    state_lb = tk.Listbox(left, listvariable=state_lb_var,
                          bg=PANEL, fg=TEXT, selectbackground=ACCENT,
                          selectforeground="white", font=MONO,
                          relief="flat", bd=0, highlightthickness=0,
                          activestyle="none")
    state_lb.pack(fill="both", expand=True, padx=2, pady=2)

    right = tk.Frame(pane, bg=PANEL, relief="solid", bd=1,
                     highlightthickness=1, highlightbackground=BORDER)
    pane.add(right, minsize=400)

    tk.Label(right, text="Items", bg=PANEL, font=SANS_B, fg=TEXT,
             anchor="w", padx=6, pady=4).pack(fill="x")
    sep2 = tk.Frame(right, bg=BORDER, height=1)
    sep2.pack(fill="x")

    cols = ("head", "body", "dot", "lookahead")
    heads = ("Head", "Body", "Dot", "Lookahead")
    widths = (60, 200, 40, 140)

    item_frame = tk.Frame(right, bg=PANEL)
    item_frame.pack(fill="both", expand=True)

    tv = ttk.Treeview(item_frame, columns=cols, show="headings", selectmode="browse")
    style_treeview(tv, cols, heads, widths)
    vsb = ttk.Scrollbar(item_frame, orient="vertical", command=tv.yview)
    tv.configure(yscrollcommand=vsb.set)
    tv.pack(side="left", fill="both", expand=True)
    vsb.pack(side="right", fill="y")

    # wire
    states_data = [None]

    def refresh():
        data = load_json("canonical_collection.json")
        if not data:
            return
        states_data[0] = data.get("states", [])
        names = [f"  State {s['id']}" for s in states_data[0]]
        state_lb_var.set(names)
        if states_data[0]:
            state_lb.selection_set(0)
            show_state(0)

    def show_state(idx):
        clear_treeview(tv)
        if not states_data[0]:
            return
        state = states_data[0][idx]
        for item in state.get("items", []):
            body = item["body"]
            dot  = item["dot"]
            body_str = " ".join(body[:dot]) + " . " + " ".join(body[dot:])
            la = "{ " + ", ".join(item["lookahead"]) + " }"
            tv.insert("", "end", values=(item["head"], body_str.strip(), dot, la))

    def on_select(event):
        sel = state_lb.curselection()
        if sel:
            show_state(sel[0])

    state_lb.bind("<<ListboxSelect>>", on_select)

    frame._refresh = refresh
    return frame


# ─────────────────────────────────────────────────────────────────────────────
# LR TABLE TAB
# ─────────────────────────────────────────────────────────────────────────────
def build_lr_table_tab(parent):
    frame = tk.Frame(parent, bg=BG)
    frame.pack(fill="both", expand=True)

    tk.Label(frame, text="LR(1) Parse Table", bg=BG,
             font=TITLE_F, fg=ACCENT2, anchor="w").pack(fill="x", padx=10, pady=(8, 2))

    container = tk.Frame(frame, bg=PANEL, relief="solid", bd=1,
                         highlightthickness=1, highlightbackground=BORDER)
    container.pack(fill="both", expand=True, padx=8, pady=6)

    tv_frame = tk.Frame(container, bg=PANEL)
    tv_frame.pack(fill="both", expand=True, padx=4, pady=4)

    # will be rebuilt on refresh
    tv_holder = [None]
    hsb_holder = [None]
    vsb_holder = [None]

    def refresh():
        data = load_json("lr1_table.json")
        if not data:
            return
        states = data.get("states", [])

        # collect all symbols
        action_syms = set()
        goto_syms   = set()
        for s in states:
            action_syms.update(s.get("action", {}).keys())
            goto_syms.update(s.get("goto", {}).keys())

        # sort: terminals first ($last), then non-terminals
        def sort_key(sym):
            if sym == "$": return (1, sym)
            return (0, sym)
        action_syms = sorted(action_syms, key=sort_key)
        goto_syms   = sorted(goto_syms)

        all_cols = ["state"] + action_syms + goto_syms
        all_heads = ["State"] + action_syms + goto_syms

        # destroy old
        for w in tv_frame.winfo_children():
            w.destroy()

        tv = ttk.Treeview(tv_frame, columns=all_cols, show="headings",
                          selectmode="browse")
        vsb = ttk.Scrollbar(tv_frame, orient="vertical", command=tv.yview)
        hsb = ttk.Scrollbar(tv_frame, orient="horizontal", command=tv.xview)
        tv.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)

        tv.heading("state", text="State", anchor="center")
        tv.column("state", width=55, anchor="center")
        for sym in action_syms:
            tv.heading(sym, text=sym, anchor="center")
            tv.column(sym, width=58, anchor="center")
        for sym in goto_syms:
            tv.heading(sym, text=sym, anchor="center")
            tv.column(sym, width=58, anchor="center")

        # section header rows via tags
        tv.tag_configure("action_hdr", background="#dce8f5", foreground=ACCENT2,
                         font=SANS_B)
        tv.tag_configure("acc_row",  background="#d4edda", foreground=GREEN)
        tv.tag_configure("err_row",  background="#fdecea", foreground=RED)
        tv.tag_configure("even",     background="#fafaf8")
        tv.tag_configure("odd",      background=PANEL)

        for idx, s in enumerate(states):
            row = [str(s["id"])]
            has_acc = False
            for sym in action_syms:
                val = s.get("action", {}).get(sym, "")
                if val == "acc":
                    has_acc = True
                row.append(val)
            for sym in goto_syms:
                row.append(str(s.get("goto", {}).get(sym, "")))
            tag = "acc_row" if has_acc else ("even" if idx % 2 == 0 else "odd")
            tv.insert("", "end", values=row, tags=(tag,))

        vsb.pack(side="right", fill="y")
        hsb.pack(side="bottom", fill="x")
        tv.pack(fill="both", expand=True)

    frame._refresh = refresh
    return frame


# ─────────────────────────────────────────────────────────────────────────────
# TRACE TABLE TAB
# ─────────────────────────────────────────────────────────────────────────────
def build_trace_tab(parent):
    frame = tk.Frame(parent, bg=BG)
    frame.pack(fill="both", expand=True)

    tk.Label(frame, text="Parse Trace", bg=BG,
             font=TITLE_F, fg=ACCENT2, anchor="w").pack(fill="x", padx=10, pady=(8, 2))

    container = tk.Frame(frame, bg=PANEL, relief="solid", bd=1,
                         highlightthickness=1, highlightbackground=BORDER)
    container.pack(fill="both", expand=True, padx=8, pady=6)

    cols   = ("step", "stack", "input", "action")
    heads  = ("Step", "Stack", "Input", "Action")
    widths = (45, 220, 200, 80)

    tv_frame = tk.Frame(container, bg=PANEL)
    tv_frame.pack(fill="both", expand=True, padx=4, pady=4)

    tv  = ttk.Treeview(tv_frame, columns=cols, show="headings", selectmode="browse")
    vsb = ttk.Scrollbar(tv_frame, orient="vertical", command=tv.yview)
    tv.configure(yscrollcommand=vsb.set)
    style_treeview(tv, cols, heads, widths)

    tv.tag_configure("acc",  background="#d4edda", foreground=GREEN, font=SANS_B)
    tv.tag_configure("err",  background="#fdecea", foreground=RED,   font=SANS_B)
    tv.tag_configure("even", background="#fafaf8")
    tv.tag_configure("odd",  background=PANEL)

    vsb.pack(side="right", fill="y")
    tv.pack(fill="both", expand=True)

    def refresh():
        clear_treeview(tv)
        data = load_json("trace_table.json")
        if not data:
            return
        for i, row in enumerate(data.get("trace", [])):
            act = row.get("action", "")
            if act == "acc":
                tag = "acc"
            elif act == "ERROR":
                tag = "err"
            else:
                tag = "even" if i % 2 == 0 else "odd"
            tv.insert("", "end", values=(
                row.get("step", ""),
                row.get("stack", ""),
                row.get("input", ""),
                act
            ), tags=(tag,))

    frame._refresh = refresh
    return frame


# ─────────────────────────────────────────────────────────────────────────────
# PARSE TREE TAB
# ─────────────────────────────────────────────────────────────────────────────
def build_tree_tab(parent):
    frame = tk.Frame(parent, bg=BG)
    frame.pack(fill="both", expand=True)

    tk.Label(frame, text="Parse Tree", bg=BG,
             font=TITLE_F, fg=ACCENT2, anchor="w").pack(fill="x", padx=10, pady=(8, 2))

    container = tk.Frame(frame, bg=PANEL, relief="solid", bd=1,
                         highlightthickness=1, highlightbackground=BORDER)
    container.pack(fill="both", expand=True, padx=8, pady=6)

    canvas = tk.Canvas(container, bg=PANEL, highlightthickness=0)
    hsb = ttk.Scrollbar(container, orient="horizontal", command=canvas.xview)
    vsb = ttk.Scrollbar(container, orient="vertical",   command=canvas.yview)
    canvas.configure(xscrollcommand=hsb.set, yscrollcommand=vsb.set)

    vsb.pack(side="right", fill="y")
    hsb.pack(side="bottom", fill="x")
    canvas.pack(fill="both", expand=True)

    legend = tk.Frame(container, bg=PANEL)
    legend.place(relx=0.0, rely=0.0, x=6, y=4)
    tk.Label(legend, text="  Non-terminal ", bg=PANEL, fg=ACCENT,
             font=MONO_SM, relief="solid", bd=1, padx=2).pack(side="left")
    tk.Label(legend, text="  ", bg=PANEL, width=1).pack(side="left")
    tk.Label(legend, text="  Terminal ", bg="#e8f4e8", fg=GREEN,
             font=MONO_SM, relief="solid", bd=1, padx=2).pack(side="left")

    def refresh():
        data = load_json("parse_tree.json")
        render_tree_canvas(canvas, data)

    frame._refresh = refresh
    return frame


# ─────────────────────────────────────────────────────────────────────────────
# MAIN WINDOW
# ─────────────────────────────────────────────────────────────────────────────
class LR1App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("LR(1) Parser Visualizer")
        self.configure(bg=BG)
        self.geometry("1100x700")
        self.minsize(800, 500)

        self._build_ui()
        self._apply_styles()
        self._load_existing_data()

    def _apply_styles(self):
        style = ttk.Style(self)
        style.theme_use("clam")

        style.configure("Treeview",
                        background=PANEL, foreground=TEXT,
                        rowheight=22, fieldbackground=PANEL,
                        font=MONO_SM, borderwidth=0)
        style.configure("Treeview.Heading",
                        background="#e8e8e0", foreground=ACCENT2,
                        font=SANS_B, relief="flat", padding=(4, 3))
        style.map("Treeview",
                  background=[("selected", ACCENT)],
                  foreground=[("selected", "white")])
        style.configure("TScrollbar", troughcolor=BG, background=BORDER,
                        borderwidth=0, arrowsize=12)
        style.configure("TNotebook", background=BG, borderwidth=0)
        style.configure("TNotebook.Tab",
                        background="#e0e0d8", foreground=TEXT_DIM,
                        padding=(12, 5), font=SANS_SM)
        style.map("TNotebook.Tab",
                  background=[("selected", PANEL)],
                  foreground=[("selected", ACCENT2)],
                  expand=[("selected", [1, 1, 1, 0])])

    def _build_ui(self):
        # ── Top bar ──────────────────────────────────────────────────────────
        top = tk.Frame(self, bg=ACCENT2, height=36)
        top.pack(fill="x", side="top")
        top.pack_propagate(False)
        tk.Label(top, text="LR(1) Parser Visualizer", bg=ACCENT2,
                 fg="white", font=("Helvetica", 12, "bold"),
                 anchor="w", padx=12).pack(side="left", fill="y")

        # ── Main horizontal split ─────────────────────────────────────────────
        main = tk.Frame(self, bg=BG)
        main.pack(fill="both", expand=True)

        # LEFT panel: grammar + input + run
        left_panel = tk.Frame(main, bg=PANEL, width=260,
                              relief="solid", bd=0,
                              highlightthickness=1, highlightbackground=BORDER)
        left_panel.pack(side="left", fill="y", padx=(8, 0), pady=8)
        left_panel.pack_propagate(False)

        self._build_left(left_panel)

        # RIGHT: notebook tabs
        right_panel = tk.Frame(main, bg=BG)
        right_panel.pack(side="left", fill="both", expand=True, padx=8, pady=8)

        self.nb = ttk.Notebook(right_panel)
        self.nb.pack(fill="both", expand=True)

        self.tab_first     = build_first_tab(self.nb)
        self.tab_canonical = build_canonical_tab(self.nb)
        self.tab_lrtable   = build_lr_table_tab(self.nb)
        self.tab_trace     = build_trace_tab(self.nb)
        self.tab_tree      = build_tree_tab(self.nb)

        self.nb.add(self.tab_first,     text="  FIRST Sets  ")
        self.nb.add(self.tab_canonical, text="  Canonical Collection  ")
        self.nb.add(self.tab_lrtable,   text="  LR Table  ")
        self.nb.add(self.tab_trace,     text="  Trace  ")
        self.nb.add(self.tab_tree,      text="  Parse Tree  ")

        self.tabs = [self.tab_first,self.tab_canonical, self.tab_lrtable,
                     self.tab_trace, self.tab_tree]

    def _build_left(self, parent):
        pad = dict(padx=10, pady=(0, 4))

        # Grammar section
        tk.Label(parent, text="Grammar", bg=PANEL, font=SANS_B,
                 fg=ACCENT2, anchor="w").pack(fill="x", padx=10, pady=(10, 2))
        sep = tk.Frame(parent, bg=BORDER, height=1)
        sep.pack(fill="x", padx=8, pady=(0, 4))

        tk.Label(parent, text="Productions (one per line,  ->  separator):",
                 bg=PANEL, font=SANS_SM, fg=TEXT_DIM, anchor="w",
                 wraplength=230, justify="left").pack(fill="x", padx=10)

        self.grammar_text = scrolledtext.ScrolledText(
            parent, height=7, font=MONO, bg="#f9f9f6",
            fg=TEXT, relief="solid", bd=1, insertbackground=ACCENT,
            wrap="none")
        self.grammar_text.pack(fill="x", padx=10, pady=(4, 8))
        self.grammar_text.insert("1.0", GRAMMAR_DEFAULT)

        # Input section
        tk.Label(parent, text="Input", bg=PANEL, font=SANS_B,
                 fg=ACCENT2, anchor="w").pack(fill="x", padx=10, pady=(2, 2))
        sep2 = tk.Frame(parent, bg=BORDER, height=1)
        sep2.pack(fill="x", padx=8, pady=(0, 4))

        tk.Label(parent, text="Tokens (space-separated):",
                 bg=PANEL, font=SANS_SM, fg=TEXT_DIM, anchor="w").pack(
                 fill="x", padx=10)

        self.input_var = tk.StringVar(value="( a )")
        input_entry = tk.Entry(parent, textvariable=self.input_var,
                               font=MONO, bg="#f9f9f6", fg=TEXT,
                               relief="solid", bd=1, insertbackground=ACCENT)
        input_entry.pack(fill="x", padx=10, pady=(4, 10))
        input_entry.bind("<Return>", lambda e: self._run())

        # Run button
        run_btn = tk.Button(parent, text="PARSE", command=self._run,
                            bg=ACCENT, fg="white", font=SANS_B,
                            relief="flat", cursor="hand2",
                            activebackground=ACCENT2, activeforeground="white",
                            padx=8, pady=6)
        run_btn.pack(fill="x", padx=10, pady=(0, 8))

        # Status / log
        tk.Label(parent, text="Log", bg=PANEL, font=SANS_B,
                 fg=ACCENT2, anchor="w").pack(fill="x", padx=10, pady=(6, 2))
        sep3 = tk.Frame(parent, bg=BORDER, height=1)
        sep3.pack(fill="x", padx=8, pady=(0, 4))

        self.log_text = scrolledtext.ScrolledText(
            parent, height=10, font=MONO_SM, bg="#1a1a1a",
            fg="#a8d8a8", relief="flat", bd=0, state="disabled",
            insertbackground="white", wrap="word")
        self.log_text.pack(fill="both", expand=True, padx=8, pady=(0, 10))

        # result label
        self.result_var = tk.StringVar(value="")
        self.result_lbl = tk.Label(parent, textvariable=self.result_var,
                                   bg=PANEL, font=SANS_B, anchor="center",
                                   padx=6, pady=4)
        self.result_lbl.pack(fill="x", padx=10, pady=(0, 8))

    def _log(self, msg, clear=False):
        self.log_text.config(state="normal")
        if clear:
            self.log_text.delete("1.0", "end")
        self.log_text.insert("end", msg + "\n")
        self.log_text.see("end")
        self.log_text.config(state="disabled")

    def _run(self):
        grammar_raw = self.grammar_text.get("1.0", "end").strip()
        if not grammar_raw:
            messagebox.showerror("Error", "Grammar is empty.")
            return

        tokens = self.input_var.get().strip()
        if not tokens:
            messagebox.showerror("Error", "Input is empty.")
            return

        # write grammar
        write_grammar(grammar_raw)
        self._log(f"Grammar written to grammar.txt", clear=True)
        self._log(f"Running: {EXE_NAME}")
        self._log(f"Input: {tokens}")

        self.result_var.set("")
        self.result_lbl.config(bg=PANEL, fg=TEXT)
        self.update_idletasks()

        stdout, stderr, code = run_parser(tokens)

        if stderr:
            self._log(f"[stderr] {stderr.strip()}")
        if stdout:
            # show last few lines
            lines = stdout.strip().splitlines()
            for l in lines[-12:]:
                self._log(l)

        if code != 0 or "not found" in stderr.lower():
            self._log("[ERROR] Execution failed.")
            self.result_var.set("REJECTED")
            self.result_lbl.config(bg="#fdecea", fg=RED)
        else:
            accepted = "accepted" in stdout.lower()
            if accepted:
                self.result_var.set("ACCEPTED")
                self.result_lbl.config(bg="#d4edda", fg=GREEN)
                self._log("[OK] Input accepted.")
            else:
                self.result_var.set("REJECTED")
                self.result_lbl.config(bg="#fdecea", fg=RED)
                self._log("[FAIL] Input rejected.")

        # refresh all tabs
        self._refresh_all()

    def _refresh_all(self):
        for tab in self.tabs:
            if hasattr(tab, "_refresh"):
                try:
                    tab._refresh()
                except Exception as e:
                    self._log(f"[warn] refresh error: {e}")

    def _load_existing_data(self):
        """Load JSON files that may already exist from a previous run."""
        self._refresh_all()


# ─────────────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    app = LR1App()
    app.mainloop()