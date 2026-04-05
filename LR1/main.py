from parser.grammar import Grammar

with open("grammar.txt", "r") as f:
    text = f.read()

grammar = Grammar(text)
grammar.print_first()

print(grammar)
print("Terminales:", grammar.terminals)
print("No terminales:", grammar.non_terminals)