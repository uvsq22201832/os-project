class Token:
    def __init__(self, code, text=None):
        self.code = code
        self.text = text

    def __repr__(self):
        return f"Token({self.code}" + (f", {self.text}" if self.text is not None else "") + ")"

def addTk(token_type, text=None):
    return Token(token_type, text)



# Token types
END = "END"
BREAK = "BREAK"
CHAR = "CHAR"
DOUBLE = "DOUBLE"
ELSE = "ELSE"
FOR = "FOR"
IF = "IF"
INT = "INT"
RETURN = "RETURN"
STRUCT = "STRUCT"
VOID = "VOID"
WHILE = "WHILE"
EQUAL = "EQUAL"
ASSIGN = "ASSIGN"
NOTEQ = "NOTEQ"
LESSEQ = "LESSEQ"
LESS = "LESS"
GREATER = "GREATER"
GREATEREQ = "GREATEREQ"
CT_INT = "CT_INT"
CT_REAL = "CT_REAL"
CT_CHAR = "CT_CHAR"
CT_STRING = "CT_STRING"
COMMENT = "COMMENT"
ID = "ID"
COMMA = "COMMA"
SEMICOLON = "SEMICOLON"
LPAR = "LPAR"
RPAR = "RPAR"
LBRACKET = "LBRACKET"
RBRACKET = "RBRACKET"
LACC = "LACC"
RACC = "RACC"
ADD = "ADD"
SUB = "SUB"
MUL = "MUL"
DIV = "DIV"
DOT = "DOT"
AND = "AND"
OR = "OR"
NOT = "NOT"

dic={'break': BREAK, 'char': CHAR, 'double': DOUBLE, 'else': ELSE, 'for': FOR, 'if': IF, 'int': INT, 'return': RETURN, 'struct': STRUCT, 'void': VOID, 'while': WHILE}

class Lexer:
    def __init__(self, text):
        self.text = text
        self.i = 0
        self.line = 1

    def verif(self):
        if self.i + 1 < len(self.text):
            return self.text[self.i + 1]
        return '\0'

    def getNextToken(self):
        while True:
            if self.i >= len(self.text):
                return addTk(END)

            ch = self.text[self.i]

            if ch in (' ', '\t', '\r'):
                self.i += 1
            elif ch == '\n':
                self.line += 1
                self.i+=1
            elif ch == '/' and self.verif() == '/':
                while self.i < len(self.text) and self.text[self.i] not in ('\n', '\0'):
                    self.i += 1
            elif ch == '/' and self.verif() == '*': 
                self.i += 2
                while self.i < len(self.text) - 1:
                    if self.text[self.i] == '*' and self.text[self.i+1] == '/':
                        self.i += 2
                        break
                    if self.text[self.i] == '\n':
                        self.line += 1
                    self.i += 1

            elif ch.isalpha() or ch == '_':
                start = self.i
                self.i += 1
                while self.i < len(self.text) and (self.text[self.i].isalnum() or self.text[self.i] == '_'):
                    self.i += 1
                text = self.text[start:self.i]
                return addTk(dic.get(text, ID),text if text not in ['break','char','double','else','for','if','int','return','struct','void','while'] else None)

            elif ch.isdigit():
                start = self.i
                if ch == '0' and self.verif() in ('x', 'X'):
                    self.i+= 2
                    while self.i < len(self.text) and self.text[self.i] in '0123456789abcdefABCDEF':
                        self.i+= 1
                    value = int(self.text[start:self.i], 16)
                    return addTk(CT_INT, value)
                else:
                    has_dot = False
                    has_exp = False
                    while self.i< len(self.text) and (self.text[self.i].isdigit() or self.text[self.i] == '.' or self.text[self.i] in 'eE'):
                        if self.text[self.i] == '.':
                            has_dot = True
                        if self.text[self.i] in 'eE':
                            has_exp= True
                        self.i += 1
                    text = self.text[start:self.i]
                    return addTk(CT_REAL if has_dot or has_exp else CT_INT, float(text) if has_dot or has_exp else int(text))
            elif ch == '\'':
                start = self.i
                self.i+= 1
                if self.text[self.i] == '\\':
                    self.i+= 2  
                else:
                    self.i += 1
                self.i+= 1  
                text = self.text[start:self.i]
                return addTk(CT_CHAR, text)

            elif ch == '"':
                start = self.i
                self.i+= 1
                while self.i< len(self.text) and self.text[self.i] != '"':
                    if self.text[self.i] == '\\':
                        self.i += 2
                    else:
                        self.i += 1
                self.i += 1
                text = self.text[start:self.i]
                return addTk(CT_STRING, text)

            else:
                two_char_ops = {'==': EQUAL, '!=': NOTEQ, '<=': LESSEQ, '>=': GREATEREQ, '&&': AND, '||': OR}
                one_char_ops = {'=': ASSIGN, '+': ADD, '-': SUB, '*': MUL, '/': DIV, '.': DOT, '!': NOT,
                                '<': LESS, '>': GREATER, ',': COMMA, ';': SEMICOLON, '(': LPAR, ')': RPAR,
                                '[': LBRACKET, ']': RBRACKET, '{': LACC, '}': RACC}
                verif = self.verif()
                pair = ch + verif
                if pair in two_char_ops:
                    self.i += 2
                    return addTk(two_char_ops[pair])
                elif ch in one_char_ops:
                    self.i += 1
                    return addTk(one_char_ops[ch])
                else:
                    raise Exception(f"Invalid character '{ch}' at line {self.line}")
                



with open(r"C:\Users\yanis\OneDrive\Pictures\Bureau\COMPILES_TECHNIQUE\0.c", "r") as f:
    code = f.read()

lexer = Lexer(code)

tokens = []
while True:
    token = lexer.getNextToken()
    tokens.append(token)
    if token.code == END:
        break
l_t=[]

for tk in tokens:
    l_t.append(tk)

print(l_t)



