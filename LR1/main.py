from parser.grammar import Grammar

with open("grammar.txt", "r") as f:
    text = f.read()

grammar = Grammar(text)

print(grammar)
print("Terminales:", grammar.terminals)
print("No terminales:", grammar.non_terminals)