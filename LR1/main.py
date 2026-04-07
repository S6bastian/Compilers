from parser.grammar import Grammar
from parser.lr1_items import LR1Items

with open("grammar.txt", "r") as f:
    text = f.read()

grammar = Grammar(text)
grammar.print_first()

print(grammar)
print("Terminales:", grammar.terminals)
print("No terminales:", grammar.non_terminals)

# ---- nuevo ----
items = LR1Items(grammar)

start_item = (grammar.start_symbol, tuple(grammar.productions[grammar.start_symbol][0]), 0, "$")
state0 = items.closure({start_item})

print("\nEstado 0:")
for item in sorted(state0):
    head, body, dot, la = item
    body_with_dot = list(body)
    body_with_dot.insert(dot, ".")
    print(f"  [{head} -> {' '.join(body_with_dot)},  {la}]")