from parser.grammar import Grammar

class LR1Items:
    def __init__(self, grammar: Grammar):
        self.grammar = grammar
        self.states = []   # lista de conjuntos de items
        self.transitions = {}  # (estado_n, symbol) -> estado_n

        self._build()

    # item = (head, body, dot, lookahead)
    # ejemplo: ("C", ("c", "C"), 0, "d")

    def closure(self, items):
        result = set(items)
        queue = list(items)

        while queue:
            head, body, dot, lookahead = queue.pop()

            # punto al final, nada que expandir
            if dot == len(body):
                continue

            symbol_after_dot = body[dot]

            # solo expandimos no terminales
            if symbol_after_dot not in self.grammar.non_terminals:
                continue

            # calculamos los lookaheads: FIRST(lo que sigue + lookahead actual)
            after = list(body[dot+1:]) + [lookahead]
            lookaheads = self.grammar.first_of_sequence(after) - {self.grammar.empty_symbol}

            for production_body in self.grammar.productions[symbol_after_dot]:
                for la in lookaheads:
                    new_item = (symbol_after_dot, tuple(production_body), 0, la)
                    if new_item not in result:
                        result.add(new_item)
                        queue.append(new_item)

        return result

    def _build(self):
        pass  # aqui va goto y la construccion de estados, lo hacemos despues