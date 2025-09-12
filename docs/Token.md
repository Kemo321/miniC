### How It Works
The Token struct represents individual lexer outputs with a TokenType enum for categories like keywords (int, void, str, if, else, while, return), identifiers, literals (int, string), operators (plus, minus, multiply, divide, assign, equal, not, not equal, less, greater, less eq, greater eq), punctuation (lparen, rparen, lbrace, rbrace, colon, comma, semicolon), newline, and EOF. It stores a variant value (int for numbers, string for identifiers/keywords/strings), plus line and column for error reporting. No methods; it's a simple data holder for passing to parsers.

### Example of Use
In lexing "if (x == 1)", tokens include KEYWORD_IF, LPAREN, IDENTIFIER "x", OP_EQUAL, LITERAL_INT 1, RPAREN, allowing the parser to build an if condition expression from these structured elements.
