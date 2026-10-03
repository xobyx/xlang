#!/usr/bin/env python3
"""
xlang PEG Grammar Validator
Tests the PEG grammar rules against all test files in tests/*.xb.
"""

import glob
import re
import sys

# Increase recursion limit for deep AST nesting test files
sys.setrecursionlimit(10000)

def validate_all_tests():
    try:
        import pyparsing as pp
    except ImportError:
        print("pyparsing is not installed; install via 'pip install pyparsing'")
        return False

    pp.ParserElement.enablePackrat()

    # Comments & Whitespace
    comment = pp.Regex(r'(//|#)[^\r\n]*') | pp.Regex(r'/\*.*?\*/', flags=re.DOTALL)

    # Keywords
    keywords = [
        "if", "else", "eif", "while", "for", "do", "return", "class", "break",
        "continue", "import", "static", "new", "in", "this", "null", "true", "false",
        "int", "long", "float", "double", "string", "char", "bool", "void", "object"
    ]

    ident = ~pp.MatchFirst([pp.Keyword(kw) for kw in keywords]) + pp.Regex(r'[a-zA-Z_][a-zA-Z0-9_]*')

    # Literals
    int_lit = pp.Regex(r'0[xX][0-9a-fA-F]+[lL]?') | pp.Regex(r'0[bB][01]+[lL]?') | pp.Regex(r'[0-9]+[lL]?')
    float_lit = pp.Regex(r'[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?') | pp.Regex(r'[0-9]+[eE][+-]?[0-9]+')
    bool_lit = pp.Keyword("true") | pp.Keyword("false")
    null_lit = pp.Keyword("null")
    char_lit = pp.Regex(r"'(\\.|[^'\\\n\r])'")
    str_lit = pp.Regex(r'"(\\.|[^"\\\n\r])*"')

    expr = pp.Forward()
    list_lit = pp.Group(pp.Suppress('[') + pp.Optional(pp.delimitedList(expr) + pp.Optional(pp.Suppress(','))) + pp.Suppress(']'))
    literal = float_lit | int_lit | bool_lit | null_lit | char_lit | str_lit | list_lit

    type_kw = pp.MatchFirst([pp.Keyword(t) for t in ["int", "long", "float", "double", "string", "char", "bool", "void", "object"]])
    base_type = type_kw | ident
    type_rule = base_type + pp.ZeroOrMore(pp.Suppress('[') + pp.Optional(int_lit) + pp.Suppress(']'))

    # Postfix & Primary
    new_expr = pp.Keyword("new") + ident + pp.Optional(pp.Suppress('(') + pp.Optional(pp.delimitedList(expr)) + pp.Suppress(')'))
    func_call = ident + pp.Suppress('(') + pp.Optional(pp.delimitedList(expr)) + pp.Suppress(')')
    primary = new_expr | func_call | literal | pp.Keyword("this") | ident | (pp.Suppress('(') + expr + pp.Suppress(')'))

    # Postfix chaining: .field, .method(), [idx], ++, --
    postfix_op = (
        (pp.Suppress('.') + ident + pp.Suppress('(') + pp.Optional(pp.delimitedList(expr)) + pp.Suppress(')')) |
        (pp.Suppress('.') + ident) |
        (pp.Suppress('[') + expr + pp.Suppress(']')) |
        pp.Literal('++') |
        pp.Literal('--')
    )
    postfix_expr = primary + pp.ZeroOrMore(postfix_op)

    # Infix expressions
    operators = [
        (pp.oneOf(['-', '!', '~']), 1, pp.opAssoc.RIGHT),
        (pp.oneOf(['*', '/', '%']), 2, pp.opAssoc.LEFT),
        (pp.oneOf(['+', '-']), 2, pp.opAssoc.LEFT),
        (pp.oneOf(['<<', '>>']), 2, pp.opAssoc.LEFT),
        (pp.oneOf(['<=', '>=', '<', '>']), 2, pp.opAssoc.LEFT),
        (pp.oneOf(['==', '!=']), 2, pp.opAssoc.LEFT),
        (pp.Literal('&'), 2, pp.opAssoc.LEFT),
        (pp.Literal('^'), 2, pp.opAssoc.LEFT),
        (pp.Literal('|'), 2, pp.opAssoc.LEFT),
        (pp.Literal('&&'), 2, pp.opAssoc.LEFT),
        (pp.Literal('||'), 2, pp.opAssoc.LEFT),
    ]
    expr <<= pp.infixNotation(postfix_expr, operators)

    # Assignment
    assign_op = pp.oneOf(['=', '+=', '-=', '*=', '/=', '%='])
    assignment = postfix_expr + assign_op + expr
    expr_or_assign = assignment | expr

    # Statements
    stmt = pp.Forward()
    block = pp.Forward()
    block <<= pp.Suppress('{') + pp.ZeroOrMore(stmt) + pp.Suppress('}')
    body = block | stmt

    import_target = str_lit | ident
    import_stmt = pp.Keyword("import") + ( (pp.Suppress('(') + import_target + pp.Suppress(')')) | import_target )

    param = (type_rule + ident) | ident
    param_list = pp.Optional(pp.delimitedList(param))

    func_decl = pp.Optional(pp.Keyword("static")) + (
        (type_rule + ident + pp.Suppress('(') + param_list + pp.Suppress(')') + block) |
        (ident + pp.Suppress('(') + param_list + pp.Suppress(')') + block)
    )

    var_decl = pp.Optional(pp.Keyword("static")) + type_rule + ident + pp.Optional(pp.Suppress('=') + expr)

    if_stmt = pp.Forward()
    else_clause = (pp.Keyword("else") + if_stmt) | (pp.Keyword("else") + body) | if_stmt
    if_stmt <<= (pp.Keyword("if") | pp.Keyword("eif")) + pp.Suppress('(') + expr + pp.Suppress(')') + body + pp.Optional(else_clause)

    while_stmt = pp.Keyword("while") + pp.Suppress('(') + expr + pp.Suppress(')') + body
    do_while_stmt = pp.Keyword("do") + block + pp.Keyword("while") + pp.Suppress('(') + expr + pp.Suppress(')')

    for_sep = pp.oneOf([',', ';'])
    for_stmt = (
        (pp.Keyword("for") + pp.Suppress('(') + ident + pp.Keyword("in") + expr + pp.Suppress(')') + body) |
        (pp.Keyword("for") + ident + pp.Keyword("in") + expr + body) |
        (pp.Keyword("for") + pp.Suppress('(') + (var_decl | expr_or_assign) + pp.Suppress(for_sep) + expr_or_assign + pp.Suppress(for_sep) + expr_or_assign + pp.Suppress(')') + body) |
        (pp.Keyword("for") + ident + pp.Suppress('(') + expr + pp.Suppress(',') + expr + pp.Suppress(',') + expr + pp.Suppress(')') + body)
    )

    return_stmt = pp.Keyword("return") + pp.Optional(expr)
    break_stmt = pp.Keyword("break")
    continue_stmt = pp.Keyword("continue")

    class_member = func_decl | var_decl
    class_decl = pp.Keyword("class") + ident + pp.Optional(pp.Suppress('(') + pp.Optional(ident) + pp.Suppress(')')) + pp.Suppress('{') + pp.ZeroOrMore(class_member + pp.Optional(pp.Suppress(';'))) + pp.Suppress('}')

    stmt <<= (
        import_stmt |
        class_decl |
        func_decl |
        var_decl + pp.Optional(pp.Suppress(';')) |
        if_stmt |
        while_stmt |
        do_while_stmt + pp.Optional(pp.Suppress(';')) |
        for_stmt |
        return_stmt + pp.Optional(pp.Suppress(';')) |
        break_stmt + pp.Optional(pp.Suppress(';')) |
        continue_stmt + pp.Optional(pp.Suppress(';')) |
        block |
        expr_or_assign + pp.Optional(pp.Suppress(';'))
    )
    stmt.ignore(comment)

    program = pp.ZeroOrMore(stmt) + pp.StringEnd()
    program.ignore(comment)

    sample_files = sorted(glob.glob("tests/*.xb"))
    passed = 0
    failed = 0

    print(f"Validating PEG grammar against {len(sample_files)} test files...")
    for sf in sample_files:
        try:
            with open(sf, "r") as f:
                code = f.read()
            program.parseString(code)
            passed += 1
        except Exception as e:
            print(f"FAIL: {sf}: {e}")
            failed += 1

    print(f"Results: {passed} / {len(sample_files)} passed")
    return failed == 0

if __name__ == "__main__":
    if validate_all_tests():
        print("ALL PEG VALIDATIONS PASSED!")
        sys.exit(0)
    else:
        sys.exit(1)
