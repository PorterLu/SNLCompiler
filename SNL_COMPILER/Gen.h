#ifndef GEN_H_INCLUDED
#define GEN_H_INCLUDED
// semantic analysis + intermediate code generation: walk the syntax tree, check semantics, and produce quadruple intermediate code (IRProgram).
// usage: CodeGenerator gen; if (gen.generate(root)) use gen.ir; else inspect gen.errors.
#include "header.h"
#include "Ir.h"
#include <map>
using namespace std;

struct TypeInfo
{
    enum Kind { INT, CHAR, ARRAY, RECORD };
    Kind kind;
    int size;                                      // size in words
    int low, high;                                 // array lower and upper bounds
    TypeInfo* elem;                                // array element type
    vector<string> fieldNames;                     // record fields
    vector<TypeInfo*> fieldTypes;
    vector<int> fieldOffsets;
    TypeInfo(Kind k) : kind(k), size(1), low(0), high(0), elem(NULL) {}
    bool isScalar() const { return kind == INT || kind == CHAR; }
    int findField(const string& n) const { for (size_t i = 0; i < fieldNames.size(); i++) if (fieldNames[i] == n) return (int)i; return -1; }
};

struct ParamInfo { TypeInfo* type; bool isVar; };

struct Symbol
{
    enum Kind { VAR, TYPE, PROC };
    Kind kind;
    string name;
    TypeInfo* type;             // VAR / TYPE
    int level;                  // VAR: the level it lives at; PROC: the level of the procedure body
    int offset;                 // VAR: word offset within the activation record
    bool isVarParam;            // VAR: whether it is a var parameter
    int procIndex;              // PROC: index into ir.procs
    vector<ParamInfo> params;   // PROC: parameter list
    Symbol(Kind k, const string& n) : kind(k), name(n), type(NULL), level(0), offset(0), isVarParam(false), procIndex(-1) {}
};

struct LValue { Operand base; Operand off; bool hasOff; TypeInfo* type; LValue() : hasOff(false), type(NULL) {} };
struct ExpRes { Operand opnd; TypeInfo* type; bool isVar; LValue lv; ExpRes() : type(NULL), isVar(false) {} };

struct CodeGenerator
{
    vector<string> errors;                      // semantic errors, each with a line number
    IRProgram ir;                               // the generated intermediate code
    bool generate(Node* root);                  // returns true on success

    vector< map<string, Symbol*> > scopes;      // scope stack
    int level;                                  // current level
    int nextOffset;                             // next free word offset in the current activation record
    int tempBase, tempTop, tempMax;             // temporary area: start, current position, high-water mark
    vector<int> saved;                          // save the enclosing allocation state when entering a nested procedure
    int labelCount;
    TypeInfo* intType; TypeInfo* charType;

    void pushScope(); void popScope();
    Symbol* lookup(const string& name);
    bool declare(Symbol* s, int line);
    void error(int line, const string& msg);
    int lineOf(Node* n);
    void emit(const string& op, const Operand& a = Operand(), const Operand& b = Operand(), const Operand& r = Operand());
    Operand newTemp();
    int newLabel();
    Operand operandOf(Symbol* s);

    void declarePart(Node* n);
    void typeDecList(Node* n);
    TypeInfo* typeName(Node* n);
    TypeInfo* baseType(Node* n);
    TypeInfo* arrayType(Node* n);
    TypeInfo* recType(Node* n);
    vector<Node*> idList(Node* n);
    void varDecList(Node* n);
    void procDeclaration(Node* n);
    void paramDecList(Node* n, Symbol* p);

    void stmList(Node* n);
    void stm(Node* n);
    void call(Node* idNode, Node* actList);
    vector<Node*> actParams(Node* n);
    Operand relExp(Node* n);
    ExpRes exp(Node* n);
    ExpRes term(Node* n);
    ExpRes factor(Node* n);
    ExpRes binary(const string& op, ExpRes& a, ExpRes& b, int line);
    LValue lvalue(Node* idNode, Node* variMore);
    LValue indexInto(LValue lv, Node* expNode, int line);
    LValue fieldOf(LValue lv, Node* fid);
    Operand rvalue(ExpRes& e);
    void checkScalar(ExpRes& r, int line);
};
#endif // GEN_H_INCLUDED
