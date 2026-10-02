#ifndef GEN_H_INCLUDED
#define GEN_H_INCLUDED
// 语义分析 + 中间代码生成：遍历语法树，检查语义，生成四元式形式的中间代码 (IRProgram)。
// 用法：CodeGenerator gen; if (gen.generate(root)) 用 gen.ir; else 看 gen.errors。
#include "header.h"
#include "Ir.h"
#include <map>
using namespace std;

struct TypeInfo
{
    enum Kind { INT, CHAR, ARRAY, RECORD };
    Kind kind;
    int size;                                      // 占的字数
    int low, high;                                 // 数组下界、上界
    TypeInfo* elem;                                // 数组元素类型
    vector<string> fieldNames;                     // 记录的域
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
    int level;                  // VAR：所在层次；PROC：过程体的层次
    int offset;                 // VAR：在活动记录里的字偏移
    bool isVarParam;            // VAR：是否 var 形参
    int procIndex;              // PROC：在 ir.procs 里的下标
    vector<ParamInfo> params;   // PROC：形参表
    Symbol(Kind k, const string& n) : kind(k), name(n), type(NULL), level(0), offset(0), isVarParam(false), procIndex(-1) {}
};

struct LValue { Operand base; Operand off; bool hasOff; TypeInfo* type; LValue() : hasOff(false), type(NULL) {} };
struct ExpRes { Operand opnd; TypeInfo* type; bool isVar; LValue lv; ExpRes() : type(NULL), isVar(false) {} };

struct CodeGenerator
{
    vector<string> errors;                      // 语义错误，每条带行号
    IRProgram ir;                               // 生成的中间代码
    bool generate(Node* root);                  // 成功返回 true

    vector< map<string, Symbol*> > scopes;      // 作用域栈
    int level;                                  // 当前层次
    int nextOffset;                             // 当前活动记录里下一个空闲字偏移
    int tempBase, tempTop, tempMax;             // 临时变量区：起点、当前位置、高水位
    vector<int> saved;                          // 进入嵌套过程时保存外层的分配状态
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
