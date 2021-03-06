/*
 * File: Tester.cpp
 * Project: Assignment4
 * File Created: Tuesday, 24th November 2020 7:59:20 pm
 * Author: Shane Arcaro (sma237@njit.edu)
 */

#include <iostream>
#include <fstream>
#include <map>
#include "val.h"
#include "parseRun.h"

// Check if value has been seen before
bool g_valCheck = false;
// Check if printing is allowed (no errors detected)
bool g_print = true;
// Check if an if statement expression should be allowed to run
bool g_allow = true;

int main(int argc, char** argv) {
    if (argc == 2) {
        std::fstream file(argv[1]);
        int lineNumber = 1;

        if (file.is_open()) {
            bool errorsFound = Prog(file, lineNumber);
            std::cout << std::endl;
            if (errorsFound) {
                std::cout << "Unsuccessful Execution" << std::endl;
                std::cout << std::endl;
                std::cout << "Number of Syntax Errors: " << error_count << std::endl;
            }
            else
                std::cout << "Successful Execution" << std::endl;
        }
        else {
            std::cout << "CANNOT OPEN FILE " << argv[1] << std::endl;
            std::exit(1);
        }
    }
    else {
        std::cout << (argc > 2 ? "ONLY ONE FILE NAME ALLOWED" : "NO FILE NAME GIVEN") << std::endl;
        std::exit(1);
    }
    return 0;
}

/**
 * Starts Recursive Decent Parser 
 */
bool Prog(istream& in, int& line) {
    LexItem item = Parser::GetNextToken(in, line);
    bool errorsFound = false;

    if (item.GetToken() != BEGIN) {
        ParseError(line, "No Begin Token");
        Parser::PushBackToken(item);
        errorsFound = true;
    }

    if (StmtList(in, line))
        errorsFound = true;

    return errorsFound;   
}

/**
 * Language is one long Statement List broken up into smaller statements
 */
bool StmtList(istream& in, int& line) {
    LexItem item;
    bool errorsFound = false;

    while (true) {
        item = Parser::GetNextToken(in, line);
        if (item.GetToken() == DONE || item.GetToken() == END)
            return errorsFound;
        else
            Parser::PushBackToken(item);
        if (Stmt(in, line)) {
            // ParseError(line, "Invalid Statement Expression");
            errorsFound = true;
        }
    }
    return errorsFound;
}

/**
 * A statement can be either Print, If, Assign or an error
 */
bool Stmt(istream& in, int& line) {
    LexItem item = Parser::GetNextToken(in, line);
    Parser::PushBackToken(item);

    switch (item.GetToken()) {
        case PRINT:
            return PrintStmt(in, line);
        case IF:
            return IfStmt(in, line);
        case IDENT:
            return AssignStmt(in, line);
        default:
            /**
             * If the input cannot be recognized read in tokens until the next
             * line is reached. If the current line is evaluated after an 
             * unrecognized input AssignStmt tends to be called causing
             * a back-to-back chain of errors.
             */
            const int currentLine = item.GetLinenum();
            while (currentLine == line) {
                item = Parser::GetNextToken(in, line);
            }
            Parser::PushBackToken(item);
            ParseError(currentLine, "Unrecognized Input");
            return true;
    }
    return false;
}

/**
 * Print statements
 * Need to check for variable assignment
 */
bool PrintStmt(istream& in, int& line) {
    const int lineNumber = line;
    g_valCheck = true;
    ValQue = new queue<Value>;
    // Increase line count if print statement is incorrect
    if (ExprList(in, line)) {
        ParseError(lineNumber, "Invalid Print Statement Expression");
        line++;

        // Memory has to be deleted
        while(!(*ValQue).empty()){
            ValQue->pop();
        }
        delete ValQue;
        return true;
    }

    // Print values to screen left to right
    if (g_print && g_allow) {
        while (g_print && !(*ValQue).empty()) {
            std::cout << ValQue->front();
            ValQue->pop();
        }
        std::cout << std::endl;
    }
    g_allow = true;
    return false;
}

/**
 * If statements a little more complex
 * Still need to check for variable assignment
 */
bool IfStmt(istream& in, int& line) {
    LexItem item = Parser::GetNextToken(in, line);
    bool errorsFound = false;
    g_valCheck = true;

    if (item.GetToken() != IF) {
        ParseError(line, "Missing If Statement Expression");
        errorsFound = true;
    }

    item = Parser::GetNextToken(in, line);
    if (item.GetToken() != LPAREN) {
        ParseError(line, "Missing Left Parenthesis");
        errorsFound = true;
    }
    Value expressionValue;
    if (Expr(in, line, expressionValue)) {
        // ParseError(line, "Invalid if statement Expression");
        errorsFound = true;
    }

    if (!expressionValue.IsInt()) {
        ParseError(line, "Run-Time: Non Integer If Statement Expression");
        errorsFound = true;
    }
    else {
        if (expressionValue.GetInt() == 0)
            g_allow = false;

    }

    item = Parser::GetNextToken(in, line);
    if (item.GetToken() != RPAREN) {
        ParseError(line, "Missing Right Parenthesis");
        errorsFound = true;
    }

    item = Parser::GetNextToken(in, line);
    if (item.GetToken() != THEN) {
        ParseError(line, "Missing THEN");
        errorsFound = true;
    }

    // Check for errors before if statement's statement is evaluted.
    if (errorsFound)
        g_print = false;

    if (Stmt(in, line)) {
        ParseError(line, "Invalid If Statement Expression");
        errorsFound = true;
    }

    // Also check for errors after if statement's statement just in case
    if (errorsFound)
        g_print = false;

    return errorsFound;
}

/**
 * Assign statement just assigns variable
 * Don't need to check for variable assignment because
 * that's what this is doing
 */
bool AssignStmt(istream& in, int& line) {
    const int currentLine = line;
    bool errorsFound = false;
    LexItem tok;
    std::string identifier = "";
    
    // Check if var is valid, if so store the lexeme as an identifier to be used for assignment
    if (Var(in, line, tok)) {
        ParseError(currentLine, "Invalid Assignment Statement Identifier");
        errorsFound = true;
    }
    else 
        identifier = tok.GetLexeme();
       
    LexItem item = Parser::GetNextToken(in, line);
    if (item.GetToken() != EQ) {
        ParseError(currentLine, "Missing Assignment Operator =");
        errorsFound = true;
    }

    // Check for assignment value type. If true symbol assignment starts assuming no other errors
    Value val;
    if (Expr(in, line, val)) {
        ParseError(currentLine, "Invalid Assignment Statement Expression");
        errorsFound = true;
    }

    if (!errorsFound && identifier != "" && g_allow) {
        // If the var doesn't exist in symbolTable just set its value
        if (symbolTable.find(identifier) == symbolTable.end()) {
            symbolTable.insert(std::make_pair(identifier, val));
        }
        // If the var exists the value has to be checked for runtime assignment errors
        else {
            Value& actualType = symbolTable.find(identifier)->second;

            // Check actual value against new value for reassignment
            // Int and real numbers can actually be reassigned value types
            if (actualType.IsInt() && val.IsInt() || actualType.IsReal() && val.IsReal()
                || actualType.IsStr() && val.IsStr())
                symbolTable[identifier] = val;
            else if (actualType.IsInt() && val.IsReal())
                symbolTable[identifier] = Value((int) val.GetReal());
            else if (actualType.IsReal() && val.IsInt())
                symbolTable[identifier] = Value((float) val.GetInt());
            else {
                ParseError(currentLine, "Run-Time: Illegal Type Reassignment");
                errorsFound = true;
            }
        }
    }
    
    item = Parser::GetNextToken(in, line);
    if (item.GetToken() != SCOMA) {
        Parser::PushBackToken(item);
        ParseError(currentLine, "Missing Semicolon");
        errorsFound = true;
    }

    if (errorsFound)
        g_print = false;
    g_allow = true;
    return errorsFound; 
}

/**
 * Expression list can only be called from print statement
 * Breaks into multiple expression statements
 */
bool ExprList(istream& in, int& line) {
    LexItem item = Parser::GetNextToken(in, line);
    bool errorsFound = false;

    if (item.GetToken() != PRINT) {
        // ParseError(line, "Invalid Print Statement Expression");
        errorsFound = true;
    }

    while (true) {
        Value value;
        if (Expr(in, line, value)) {
            // ParseError(line, "Invalid Print Statement Expression List");
            errorsFound = true;
        }
        else
            ValQue->push(value);
        item = Parser::GetNextToken(in, line);

        if (item.GetToken() != COMA) {
            if (item.GetToken() == SCOMA)
                return errorsFound;
            Parser::PushBackToken(item);
            return true;
        }
    }
    return errorsFound; 
}

/**
 * Basic level of expression with + and - operations
 */
bool Expr(istream& in, int& line, Value & retVal) {
    bool errorsFound = false;
    Value accumulator;
    Value current;
    int operation = -1;

    while (true) {
        if (Term(in, line, current)) {
            // ParseError(line, "Invalid Term Expression");
            return true;
        }
        else {
            // Create a single total value and pass it on using references
            if (accumulator.IsErr())
                accumulator = current;
            else if (operation == 0) {
                // Because String operations aren't supported error is generated
                if (accumulator.IsStr() || current.IsStr()) {
                    ParseError(line, "Run-Time: Invalid String Operation");
                    errorsFound = true;
                }
                else
                    accumulator = accumulator + current;
            }
            else if (operation == 1) {
                if (accumulator.IsStr() || current.IsStr()) {
                    ParseError(line, "Run-Time: Invalid Arithmetic Operation");
                    errorsFound = true;
                }
                else
                    accumulator = accumulator - current;
            }
        }

        LexItem item = Parser::GetNextToken(in, line);
        if (item.GetToken() == ERR)
            errorsFound = true;
        else if (item.GetToken() == PLUS)
            operation = 0;
        else if (item.GetToken() == MINUS)
            operation = 1;
        else {
            Parser::PushBackToken(item);
            retVal = accumulator;
            return errorsFound;
        }
    }
    return errorsFound;  
}

/**
 * Higher Precedence expression with * and / 
 */
bool Term(istream& in, int& line, Value & retVal) {
    bool errorsFound = false;
    Value accumulator;
    Value current;
    int operation = -1;
    while (true) {
        if (Factor(in, line, current)) {
            // ParseError(line, "Invalid Term Expression");
            return true;
        }
        else {
            // Create a single total value and pass it on using references
            if (accumulator.IsErr())
                accumulator = current;
            else if (operation == 0) {
                // Because String operations aren't supported error is generated
                if (accumulator.IsStr() || current.IsStr()) {
                    ParseError(line, "Run-Time: Invalid String Operation");
                    errorsFound = true;
                }
                else
                    accumulator = accumulator * current;
            }
            else if (operation == 1) {
                if (accumulator.IsStr() || current.IsStr()) {
                    ParseError(line, "Run-Time: Invalid Arithmetic Operation");
                    errorsFound = true;
                }
                else
                    accumulator = accumulator / current;
            }
        }

        LexItem item = Parser::GetNextToken(in, line);
        if (item.GetToken() == ERR)
            errorsFound = true;
        else if (item.GetToken() == MULT)
            operation = 0;
        else if (item.GetToken() == DIV)
            operation = 1;
        else {
            Parser::PushBackToken(item);
            retVal = accumulator;
            return errorsFound;
        }
    }
    return errorsFound;  
}

/**
 * Used to check for variable assignment
 * g_valCheck is used with statements to check
 * if value is defined or not 
 */
bool Var(istream& in, int& line, LexItem &tok) {
    LexItem item = Parser::GetNextToken(in, line);
    tok = item;
    if (item.GetToken() == IDENT) {
        if (defVar.find(item.GetLexeme()) != defVar.end()) {
            defVar[item.GetLexeme()] = false;
            g_valCheck = false;
        }
        else {
            if (g_valCheck) {
                ParseError(line, "Undefined variable used in expression");
                g_valCheck = false;
                g_print = false;
                return true;
            }
            defVar[item.GetLexeme()] = true;
        }
    }
    else {
        ParseError(line, "Invalid Identifier Expression");
        g_valCheck = false;
        return true;
    }
    return false;
}

/**
 * Most basic level of expression - breaks into basic components
 * of the grammar 
 */
bool Factor(istream& in, int& line, Value &retVal) {
    LexItem item = Parser::GetNextToken(in, line);
    bool errorsFound = false;
    const int lineNumber = item.GetLinenum();

    switch (item.GetToken()) {
        case ERR:
            item = Parser::GetNextToken(in, line);
            if (item.GetLinenum() == lineNumber)
                line++;
            Parser::PushBackToken(item);
            g_valCheck = false;
            return true;
        case IDENT:
            Parser::PushBackToken(item);
            if (Var(in, line, item))
                return true;
            retVal = symbolTable[item.GetLexeme()];
            return false;
        case ICONST:
            g_valCheck = false;
            retVal = Value(std::stoi(item.GetLexeme()));
            return false;
        case RCONST:
            g_valCheck = false;
            retVal = Value(std::stof(item.GetLexeme()));
            return false;
        case SCONST:
            g_valCheck = false;
            retVal = Value(item.GetLexeme());
            return false;
        case LPAREN:
            g_valCheck = false;
            if (Expr(in, line, retVal)) {
                // ParseError(line, "Invalid Expression Statement");
                errorsFound = true;
            }
            item = Parser::GetNextToken(in, line);
            if (item.GetToken() == RPAREN)
                return errorsFound;
            else {
                Parser::PushBackToken(item);
                ParseError(line, "Missing Right Parenthesis");
                return true;
            }
    }
    return true;
}
