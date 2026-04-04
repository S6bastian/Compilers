class Grammar:
    def __init__(self, text):
        self.productions = {}                   # map<string, vector<vector<char>>>
        self.start_symbol = None                # string
        self.non_terminals = set()              # unordered_set<string>
        self.terminals = set()                  # unordered_set<string>
        self.production_list = []               # list

        self._parse(text)
        self._compute_terminals()
        self._augment_grammar()
        self._build_production_list()

    def _parse(self, text):
        lines = text.strip().split("\n")

        for i, line in enumerate(lines):
            head, body = line.split("->")
            head = head.strip()
            body = body.strip()

            if i == 0:
                self.start_symbol = head

            if head not in self.productions:
                self.productions[head] = []

            alternatives = body.split("|")

            for alt in alternatives:
                symbols = alt.strip().split()
                self.productions[head].append(symbols)

        self.non_terminals = set(self.productions.keys())

    def _augment_grammar(self):
        original_start = self.start_symbol
        new_start = original_start + "'"

        while new_start in self.productions:
            new_start += "'"

        self.productions[new_start] = [[original_start]]

        self.start_symbol = new_start

    def _compute_terminals(self):
        for head, bodies in self.productions.items():
            for body in bodies:
                for symbol in body:
                    if symbol not in self.productions:
                        self.terminals.add(symbol)

    def _build_production_list(self):
        start = self.start_symbol
        original = self.productions[start][0]  

        self.production_list.append((start, original))

        for head, bodies in self.productions.items():
            if head == start:
                continue
            for body in bodies:
                self.production_list.append((head, body))

    def __str__(self):
        result = ""
        for i, (head, body) in enumerate(self.production_list):
            result += f"{i}: {head} -> {' '.join(body)}\n"
        return result