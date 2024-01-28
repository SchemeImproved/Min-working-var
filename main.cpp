#include <llvm/Support/raw_ostream.h>
#include <fstream>
#include <string>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/ADT/StringRef.h>

enum TokenKind {
    tok_eof = -1,
    tok_identifier = -2,
    tok_integer = -3,
    tok_string = -4,
    tok_symbol = -5,
    tok_open_paren = -6,
    tok_close_paren = -7,
    tok_newline = -8,
    tok_equal = -9,
    tok_arrow = -10,
    tok_comment = -11,
    tok_dot = -12,
    tok_comma = -13,
    tok_add = -15,
    tok_sub = -16,
    tok_div = -17,
    tok_mut = -18,
    tok_assign = -19,
};

struct Token {
    TokenKind kind;
    std::basic_string<char> value;

    Token(TokenKind kind, llvm::StringRef value);

    Token();
};

class Lexer {
    const char *pos;
    const char *start;

public:
    explicit Lexer(const char *input);

    Token getNextToken();

private:
    Token identifier();

    Token integer();

    Token symbol();
};

class Parser {
public:
    explicit Parser(Lexer &lexer);

    void parse();

private:
    Lexer &lexer;

    void parseClass();


    void parseFunction();

    void parseCallHelper(const Token &currentToken);

    void mainHelper();

    void parseFunctionHelper();

    void parsePublic();

    void parsePrivate();

    void parseConstructer();
};

Token::Token(TokenKind kind, llvm::StringRef value) : kind(kind), value(value) {}

Token::Token() : kind(tok_eof), value("") {} // Default constructor

Lexer::Lexer(const char *input) : pos(input), start(input) {}

Token Lexer::getNextToken() {
    while (isspace(*pos) || *pos == '\n') {
        if (*pos == '\n') {
            pos++;
            continue;
        }
        pos++;
    }

    start = pos;
    if (*pos == '\0') {
        return {tok_eof, ""};
    }

    if (isalpha(*pos)) {
        while (isalnum(*pos) || *pos == '_' || *pos == '*' || *pos == '&' || *pos == '.')
            pos++;
        return identifier();
    } else if (isdigit(*pos)) {
        while (isdigit(*pos) || *pos == '.')
            pos++;
        return integer();
    } else if (*pos == '"') {
        pos++;
        while (*pos != '"' && *pos != '\0')
            pos++;
        if (*pos == '"') {
            pos++;
            return {tok_string, llvm::StringRef(start, pos - start)};
        } else {
            llvm::report_fatal_error("Unterminated string");
        }
    } else {
        switch (*pos) {
            default:
                return symbol();
            case '(':
                pos++;
                return {tok_open_paren, "("};
            case ')':
                pos++;
                return {tok_close_paren, ")"};
            case '=':
                pos++;
                if (*pos == '=') {
                    pos++;
                    return {tok_equal, "=="};
                } else {
                    return {tok_assign, "="};
                }
            case '-':
                pos++;
                if (*pos == '>') {
                    pos++;
                    return {tok_arrow, "->"};
                } else {
                    return {tok_sub, "-"};
                }
            case '.':
                pos++;
                return {tok_dot, "."};

            case '+':
                pos++;
                return {tok_add, "+"};
            case '*':
                pos++;
                if (isalpha(*pos)) {
                    while (isalnum(*pos) || *pos == '_') {
                        pos++;
                        return {tok_identifier, llvm::StringRef(start, pos - start)};
                    }
                } else {
                    return {tok_mut, "*"};
                }
            case '/':
                pos++;
                return {tok_div, "/"};
            case '&':
                pos++;
                if (isalpha(*pos)) {
                    while (isalnum(*pos) || *pos == '_') {
                        pos++;
                        return {tok_identifier, llvm::StringRef(start, pos - start)};
                    }
                } else {
                    llvm::report_fatal_error("Invalid character");
                }
            case ';':
                while (*pos != '\n' && *pos != '\0') {
                    pos++;
                }
                return {tok_comment, ""};
            case ',': {
                pos++;
                return {tok_comma, ","};
            }
        }
    }
}

Token Lexer::identifier() {
    return {tok_identifier, llvm::StringRef(start, pos - start)};
}

Token Lexer::integer() {
    return {tok_integer, llvm::StringRef(start, pos - start)};
}

Token Lexer::symbol() {
    return {tok_symbol, llvm::StringRef(start, 1)};
}


std::ofstream out("output.cpp");

Parser::Parser(Lexer &lexer) : lexer(lexer) {}

void write(const std::string &str) {
    out << str;
}

Token token;

void Parser::parse() {
    token = lexer.getNextToken();
    switch (token.kind) {
        case tok_open_paren:
            break;
        case tok_close_paren:
            return;
        default:
            llvm::report_fatal_error("Expected '('");
    }
    while (true) {
        token = lexer.getNextToken();
        switch (token.kind) {
            case tok_eof:
                return;
            case tok_identifier:
                if (token.value == "class") {
                    parseClass();
                } else if (token.value == "fn") {
                    parseFunction();
                } else {
                    llvm::report_fatal_error("Expected 'class' or 'function' @ 40");
                }
        }
    }
}

void Parser::parseClass() {
    write("class ");
    token = lexer.getNextToken();
    if (token.kind != tok_identifier) {
        llvm::report_fatal_error("Expected identifier");
    } else {
        write(token.value + " { \n");
    }
    token = lexer.getNextToken();
    if (token.kind == tok_close_paren) {
        write("}; \n");
        return;
    }
    if (token.kind != tok_open_paren) {
        llvm::report_fatal_error("Expected '('");
    }
    token = lexer.getNextToken();
    if (token.kind != tok_identifier) {
        llvm::report_fatal_error("Expected identifier");
    }
    if (token.value == "public") {
        write("public: \n");
        parsePublic();
        token = lexer.getNextToken();
        if (token.kind == tok_close_paren) {
            write("}; \n");
            return;
        } else if (token.kind == tok_open_paren) {
            token = lexer.getNextToken();
            if (token.kind == tok_identifier) {
                if (token.value == "private") {
                    write("private: \n");
                    parsePrivate();
                    token = lexer.getNextToken();
                    if (token.kind == tok_close_paren) {
                        write("}; \n");
                        return;
                    }
                } else {
                    llvm::report_fatal_error("Expected 'public' or 'private' @ 80");
                }
            }
        }
    } else if (token.value == "private") {
        write("private: \n");
        parsePrivate();
        token = lexer.getNextToken();
        if (token.kind == tok_close_paren) {
            write("}; \n");
            return;
        }
    } else {
        llvm::report_fatal_error("Expected 'public' or 'private'");

    }
}

void Parser::parsePublic() {
    token = lexer.getNextToken();
    while (true) {
        parseCallHelper(token);
        Token next1 = lexer.getNextToken();
        Token token = lexer.getNextToken();
        if (next1.kind == tok_close_paren && token.kind == tok_close_paren) {
            break;
        }

    }

}

void Parser::parsePrivate() {
    token = lexer.getNextToken();
    while (true) {
        parseCallHelper(token);
        Token next1 = lexer.getNextToken();
        Token token = lexer.getNextToken();
        if (next1.kind == tok_close_paren && token.kind == tok_close_paren) {
            break;
        }

    }
}

void Parser::parseCallHelper(const Token &currentToken) {
    //current token is token open paren
    Token a = lexer.getNextToken();
    if (a.kind == tok_identifier && a.value == "cMethod") {
        parseConstructer();
    }
    if (a.kind == tok_identifier && a.value == "init") {
        Token b = lexer.getNextToken();
        Token c = lexer.getNextToken();
        write(b.value + " " + c.value + " ; \n");
    } else {
        Token b = lexer.getNextToken();
        Token c = lexer.getNextToken();
        write(b.value + " " + a.value + "" + c.value + "; \n");
    }

}

void Parser::parseConstructer() {
    llvm::report_fatal_error("Constructer not supported");
}




void Parser::parseFunction() {
    token = lexer.getNextToken();
    if (token.kind == tok_identifier) {
        if (token.value == "main") {
            mainHelper();
        } else { parseFunctionHelper(); }
    }
}

void Parser::mainHelper() {
    write("int main ");
    //consume "main"
    token = lexer.getNextToken();
    if (token.kind != tok_open_paren) {
        llvm::report_fatal_error("Expected '(' @ main");
    }
    token = lexer.getNextToken();
    if (token.kind != tok_close_paren) {
        llvm::report_fatal_error("Expected ')' @ main");
    }
    write("() { \n");
    token = lexer.getNextToken();

    if (token.kind == tok_close_paren) {
        write("return 0;");
        write("} \n");
        return;
    }
    //(function main () (assign a 1) ( assign b 2))
    while (true) {
        parseCallHelper(token);
        Token next1 = lexer.getNextToken();
        Token token = lexer.getNextToken();
        if (next1.kind == tok_close_paren && token.kind == tok_close_paren) {
            break;
        }

    }
    write("return 0;");
    write("} \n");
}


void Parser::parseFunctionHelper() {
    write("void " + token.value);
    token = lexer.getNextToken();
    if (token.kind != tok_open_paren) {
        llvm::report_fatal_error("Expected '(' ");
    }
    write("(");
    token = lexer.getNextToken();
    if (token.kind == tok_close_paren) {
        write("() { \n");
    } else {
        while (token.kind != tok_close_paren) {
            write(token.value + " ");
            token = lexer.getNextToken();
        }
        write(") {");
    }

    token = lexer.getNextToken();

    if (token.kind == tok_close_paren) {
        write("}");
        return;
    }
    //(function main () (assign a 1) ( assign b 2))
    while (true) {
        parseCallHelper(token);
        Token next1 = lexer.getNextToken();
        Token token = lexer.getNextToken();
        if (next1.kind == tok_close_paren && token.kind == tok_close_paren) {
            break;
        }

    }
    write("}");
}


int main() {
    const char *input = R"(
    (class A(public(init int a) (= a 1)))
    (fn main()
        (init double b)
        (= b 2.5)
        )



    )";
    Lexer lexer(input);
    Parser parser(lexer);

    try {
        parser.parse();
        llvm::outs() << "Parsing completed successfully.\n";
    } catch (const std::exception &e) {
        llvm::errs() << "Error during parsing: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
