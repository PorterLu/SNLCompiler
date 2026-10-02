#ifndef GEN_H_INCLUDED
#define GEN_H_INCLUDED
// 语义分析 + 代码生成：把语法分析得到的语法树翻译成一个等价的 C 程序（最终程序）。
// 用法：CodeGenerator gen; string c = gen.generate(root); 若 gen.errors 非空则有语义错误。
#include "header.h"
#include <map>
#include <set>
#include <sstream>
using namespace std;

struct TypeInfo
{
    enum Kind { INT, CHAR, ARRAY, RECORD };
    Kind kind;
    string cname;                                  // 在 C 里的类型名
    int low, high;                                 // 数组下界、上界
    TypeInfo* elem;                                // 数组元素类型
    vector< pair<string, TypeInfo*> > fields;      // 记录的域
    TypeInfo(Kind k, const string& c) : kind(k), cname(c), low(0), high(0), elem(NULL) {}
    bool isScalar() const { return kind == INT || kind == CHAR; }
};

struct ParamInfo { TypeInfo* type; bool isVar; };

struct Symbol
{
    enum Kind { VAR, TYPE, PROC };
    Kind kind;
    string name;                // SNL 里的名字
    string cname;               // C 里的名字
    TypeInfo* type;             // VAR / TYPE
    int level;                  // VAR：所在层次（0 为主程序）；PROC：过程体所在层次
    bool isVarParam;            // VAR：是否 var 形参
    vector<ParamInfo> params;   // PROC：形参表
    Symbol(Kind k, const string& n) : kind(k), name(n), type(NULL), level(0), isVarParam(false) {}
};

struct ProcCtx                  // 正在翻译的过程
{
    Symbol* sym;
    string frameFields;         // 活动记录（形参 + 局部变量）的 C 成员
    string sigParams;           // C 函数形参表
    string paramCopies;         // 把形参拷进活动记录的语句
};

struct CodeGenerator
{
    vector<string> errors;                      // 语义错误，每条带行号
    string generate(Node* root);                // 翻译整个程序；有语义错误时返回空串

    struct ExpRes { string code; TypeInfo* type; bool isVar; };

    vector< map<string, Symbol*> > scopes;      // 作用域栈
    vector<ProcCtx> ctxStack;                   // 过程嵌套栈
    set<string> usedNames;
    int level, counter;
    string decls, globals, protos, funcs;       // 输出的各个段
    TypeInfo* intType; TypeInfo* charType;

    void pushScope(); void popScope();
    Symbol* lookup(const string& name);
    bool declare(Symbol* s, int line);
    string uniqueName(const string& base);
    void error(int line, const string& msg);
    int lineOf(Node* n);

    void declarePart(Node* n);
    void typeDecList(Node* n);
    TypeInfo* typeName(Node* n, const string& hint);
    TypeInfo* baseType(Node* n);
    TypeInfo* structureType(Node* n, const string& hint);
    TypeInfo* arrayType(Node* n, const string& hint);
    TypeInfo* recType(Node* n, const string& hint);
    vector<Node*> idList(Node* n);
    void varDecList(Node* n, string& out);
    void procDeclaration(Node* n);
    void paramDecList(Node* n, Symbol* p);

    void stmList(Node* n, string& out, int indent);
    void stm(Node* n, string& out, int indent);
    void call(Node* idNode, Node* actList, string& out, const string& pad);
    vector<Node*> actParams(Node* n);
    string relExp(Node* n);
    ExpRes exp(Node* n);
    ExpRes term(Node* n);
    ExpRes factor(Node* n);
    ExpRes simpleVar(Node* idNode);
    ExpRes variableRest(Node* idNode, Node* variMore);
    ExpRes index(ExpRes base, Node* expNode, int line);
    ExpRes field(ExpRes base, Node* fid);
    void checkScalar(ExpRes& r, int line);
    string varAccess(Symbol* s);
};
#endif // GEN_H_INCLUDED
