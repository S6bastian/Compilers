GRAMMAR: Simplified Markdown CFG

START SYMBOL:
Document

NON-TERMINALS:
Document, BlockList, Block, Heading, Paragraph, Text, Element, Bold, Italics, PlainText

TERMINALS:
'#', '**', '*', '\n', characters

PRODUCTION RULES:
Document    -> BlockList
BlockList   -> Block | Block BlockList
Block       -> Heading | Paragraph

Heading     -> '#' Text '\n'
Paragraph   -> Text '\n'

Text        -> Element | Element Text
Element     -> Bold | Italics | PlainText

Bold        -> '**' PlainText '**'
Italics     -> '*' PlainText '*'
PlainText   -> characters | characters PlainText
