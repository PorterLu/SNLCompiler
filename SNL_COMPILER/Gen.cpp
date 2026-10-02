#include "Gen.h"
#include <cstdio>
#include <cstdlib>
using namespace std;

static bool isEps(Node* n) { return n->son.empty() && n->value.empty(); }   // ε 结点：既无子结点也无单词
static string itos(int v) { stringstream ss; ss << v; return ss.str(); }
static string ind(int n) { return string(n * 4, ' '); }
static bool sameType(TypeInfo* a, TypeInfo* b) { return (a->isScalar() && b->isScalar()) || a == b; }

// ---------------- 工具 ----------------
int CodeGenerator::lineOf(Node* n)          // 子树里第一个单词的行号
{
    if (n->son.empty()) return n->line;
    for (size_t i = 0; i < n->son.size(); i++) { int l = lineOf(n->son[i]); if (l) return l; }
    return 0;
}
void CodeGenerator::error(int line, const string& msg) { errors.push_back("第" + itos(line) + "行：" + msg); }
void CodeGenerator::pushScope() { scopes.push_back(map<string, Symbol*>()); }
void CodeGenerator::popScope() { scopes.pop_back(); }
Symbol* CodeGenerator::lookup(const string& name)
{
    for (int i = (int)scopes.size() - 1; i >= 0; i--)
    {
        map<string, Symbol*>::iterator it = scopes[i].find(name);
        if (it != scopes[i].end()) return it->second;
    }
    return NULL;
}
bool CodeGenerator::declare(Symbol* s, int line)
{
    if (scopes.back().count(s->name)) { error(line, "标识符 " + s->name + " 重复声明"); return false; }
    scopes.back()[s->name] = s;
    return true;
}
string CodeGenerator::uniqueName(const string& base)
{
    string n = base; int k = 1;
    while (usedNames.count(n)) n = base + "_" + itos(++k);
    usedNames.insert(n);
    return n;
}
void CodeGenerator::checkScalar(ExpRes& r, int line)
{
    if (r.type && !r.type->isScalar()) { error(line, "此处的表达式必须是整数或字符类型"); r.type = NULL; }
}

// ---------------- 整个程序 ----------------
string CodeGenerator::generate(Node* root)
{
    errors.clear(); scopes.clear(); ctxStack.clear(); usedNames.clear();
    decls = globals = protos = funcs = "";
    level = 0; counter = 0;
    intType = new TypeInfo(TypeInfo::INT, "int");
    charType = new TypeInfo(TypeInfo::CHAR, "char");
    // Program -> ProgramHead DeclarePart ProgramBody
    string progName = root->son[0]->son[1]->son[0]->value;
    pushScope();
    declarePart(root->son[1]);
    string mainBody;
    stmList(root->son[2]->son[1], mainBody, 1);       // ProgramBody -> begin StmList end
    popScope();
    if (!errors.empty()) return "";

    string out;
    out += "/* 由 SNL 编译器从程序 " + progName + " 的语法树生成 */\n";
    out += "#include <stdio.h>\n#include <string.h>\n\n";
    out += "static void *snl_display[64];   /* 各层过程当前活动记录的指针，嵌套过程由此访问外层变量 */\n\n";
    if (!decls.empty())   out += "/* ---- 类型与活动记录 ---- */\n" + decls + "\n";
    if (!globals.empty()) out += "/* ---- 全局变量 ---- */\n" + globals + "\n";
    if (!protos.empty())  out += "/* ---- 过程原型 ---- */\n" + protos + "\n";
    if (!funcs.empty())   out += "/* ---- 过程 ---- */\n" + funcs;
    out += "int main(void)\n{\n" + mainBody + "    return 0;\n}\n";
    return out;
}

// ---------------- 声明部分 ----------------
void CodeGenerator::declarePart(Node* n)          // DeclarePart -> TypeDec VarDec ProcDec
{
    Node* typeDec = n->son[0]; Node* varDec = n->son[1]; Node* procDec = n->son[2];
    if (!isEps(typeDec->son[0])) typeDecList(typeDec->son[0]->son[1]);          // TypeDeclaration -> type TypeDecList
    if (!isEps(varDec->son[0]))
    {
        if (level == 0) varDecList(varDec->son[0]->son[1], globals);             // VarDeclaration -> var VarDecList
        else varDecList(varDec->son[0]->son[1], ctxStack.back().frameFields);
    }
    if (!isEps(procDec->son[0])) procDeclaration(procDec->son[0]);              // ProcDec -> ProcDeclaration
}

void CodeGenerator::typeDecList(Node* n)          // TypeDecList -> TypeId = TypeName ; TypeDecMore
{
    while (n)
    {
        Node* idNode = n->son[0]->son[0];
        TypeInfo* t = typeName(n->son[2], idNode->value);
        if (t)
        {
            Symbol* s = new Symbol(Symbol::TYPE, idNode->value);
            s->type = t;
            declare(s, idNode->line);
        }
        Node* more = n->son[4];
        n = isEps(more->son[0]) ? NULL : more->son[0];
    }
}

TypeInfo* CodeGenerator::typeName(Node* n, const string& hint)   // TypeName -> BaseType | StructureType | id
{
    Node* c = n->son[0];
    if (c->name == "BaseType") return baseType(c);
    if (c->name == "StructureType") return structureType(c, hint);
    Symbol* s = lookup(c->value);
    if (!s) { error(c->line, "类型 " + c->value + " 未声明"); return NULL; }
    if (s->kind != Symbol::TYPE) { error(c->line, c->value + " 不是类型名"); return NULL; }
    return s->type;
}
TypeInfo* CodeGenerator::baseType(Node* n) { return n->son[0]->name == "integer" ? intType : charType; }
TypeInfo* CodeGenerator::structureType(Node* n, const string& hint)
{
    Node* c = n->son[0];
    return c->name == "ArrayType" ? arrayType(c, hint) : recType(c, hint);
}
TypeInfo* CodeGenerator::arrayType(Node* n, const string& hint)  // ArrayType -> array [ Low .. Top ] of BaseType
{
    int low = atoi(n->son[2]->son[0]->value.c_str());
    int high = atoi(n->son[4]->son[0]->value.c_str());
    TypeInfo* elem = baseType(n->son[7]);
    if (high < low) { error(lineOf(n), "数组上界小于下界"); return NULL; }
    TypeInfo* t = new TypeInfo(TypeInfo::ARRAY, uniqueName(hint.empty() ? "T_array" : "T_" + hint));
    t->low = low; t->high = high; t->elem = elem;
    decls += "typedef struct { " + elem->cname + " d[" + itos(high - low + 1) + "]; } " + t->cname +
             ";   /* array [" + itos(low) + ".." + itos(high) + "] of " + elem->cname + " */\n";
    return t;
}
TypeInfo* CodeGenerator::recType(Node* n, const string& hint)    // RecType -> record FieldDecList end
{
    TypeInfo* t = new TypeInfo(TypeInfo::RECORD, uniqueName(hint.empty() ? "T_record" : "T_" + hint));
    string body;
    Node* f = n->son[1];
    while (f)                                     // FieldDecList -> (BaseType | ArrayType) IdList ; FieldDecMore
    {
        TypeInfo* ft = f->son[0]->name == "BaseType" ? baseType(f->son[0]) : arrayType(f->son[0], "");
        vector<Node*> ids = idList(f->son[1]);
        for (size_t i = 0; i < ids.size() && ft; i++)
        {
            bool dup = false;
            for (size_t j = 0; j < t->fields.size(); j++) if (t->fields[j].first == ids[i]->value) dup = true;
            if (dup) { error(ids[i]->line, "记录的域 " + ids[i]->value + " 重复"); continue; }
            t->fields.push_back(make_pair(ids[i]->value, ft));
            body += "    " + ft->cname + " f_" + ids[i]->value + ";\n";
        }
        Node* more = f->son[3];
        f = isEps(more->son[0]) ? NULL : more->son[0];
    }
    decls += "typedef struct {\n" + body + "} " + t->cname + ";\n";
    return t;
}

vector<Node*> CodeGenerator::idList(Node* n)      // IdList / VarIdList / FormList -> id (, ...)*
{
    vector<Node*> v;
    while (n)
    {
        v.push_back(n->son[0]);
        Node* more = n->son[1];
        n = isEps(more->son[0]) ? NULL : more->son[1];
    }
    return v;
}

void CodeGenerator::varDecList(Node* n, string& out)   // VarDecList -> TypeName VarIdList ; VarDecMore
{
    while (n)
    {
        TypeInfo* t = typeName(n->son[0], "");
        vector<Node*> ids = idList(n->son[1]);
        for (size_t i = 0; i < ids.size() && t; i++)
        {
            Symbol* s = new Symbol(Symbol::VAR, ids[i]->value);
            s->type = t; s->level = level; s->cname = "v_" + ids[i]->value;
            if (declare(s, ids[i]->line)) out += (level == 0 ? "" : "    ") + t->cname + " " + s->cname + ";\n";
        }
        Node* more = n->son[3];
        n = isEps(more->son[0]) ? NULL : more->son[0];
    }
}

void CodeGenerator::paramDecList(Node* n, Symbol* p)   // ParamDecList -> Param ParamMore
{
    while (n)
    {
        Node* param = n->son[0];                   // Param -> TypeName FormList | var TypeName FormList
        bool isVar = param->son[0]->name == "var";
        TypeInfo* t = typeName(param->son[isVar ? 1 : 0], "");
        vector<Node*> ids = idList(param->son[isVar ? 2 : 1]);
        for (size_t i = 0; i < ids.size() && t; i++)
        {
            Symbol* s = new Symbol(Symbol::VAR, ids[i]->value);
            s->type = t; s->level = level; s->isVarParam = isVar; s->cname = "v_" + ids[i]->value;
            if (!declare(s, ids[i]->line)) continue;
            ParamInfo pi; pi.type = t; pi.isVar = isVar; p->params.push_back(pi);
            ProcCtx& c = ctxStack.back();
            string ctype = t->cname + (isVar ? " *" : " ");
            c.frameFields += "    " + ctype + s->cname + ";\n";
            if (!c.sigParams.empty()) c.sigParams += ", ";
            c.sigParams += ctype + "a_" + ids[i]->value;
            c.paramCopies += "    frame." + s->cname + " = a_" + ids[i]->value + ";\n";
        }
        Node* more = n->son[1];
        n = isEps(more->son[0]) ? NULL : more->son[1];   // ParamMore -> ; ParamDecList
    }
}

// ProcDeclaration -> procedure ProcName ( ParamList ) ; ProcDecPart ProcBody ProcDecMore
// 每个过程翻译成一个 C 函数；形参和局部变量放在活动记录 struct 里，snl_display[层次] 指向当前活动记录，
// 这样嵌套的内层过程可以访问外层过程的变量，递归时各自有独立的活动记录。
void CodeGenerator::procDeclaration(Node* n)
{
    while (n)
    {
        Node* idNode = n->son[1]->son[0];
        Symbol* p = new Symbol(Symbol::PROC, idNode->value);
        p->cname = uniqueName("P_" + idNode->value);
        p->level = level + 1;
        declare(p, idNode->line);                  // 过程名属于外层作用域，先登记，过程体里才能递归调用
        ProcCtx ctx; ctx.sym = p; ctxStack.push_back(ctx);
        level++; pushScope();
        if (!isEps(n->son[3]->son[0])) paramDecList(n->son[3]->son[0], p);   // ParamList -> ParamDecList
        declarePart(n->son[6]->son[0]);           // ProcDecPart -> DeclarePart（含嵌套过程）
        string fields = ctxStack.back().frameFields;
        decls += "struct F_" + p->cname + " {\n" + (fields.empty() ? string("    int snl_unused;\n") : fields) + "};\n";
        string body;
        stmList(n->son[7]->son[0]->son[1], body, 1);   // ProcBody -> ProgramBody -> begin StmList end
        ProcCtx done = ctxStack.back();
        ctxStack.pop_back(); popScope(); level--;

        string lv = itos(p->level);
        string sig = "static int " + p->cname + "(" + (done.sigParams.empty() ? "void" : done.sigParams) + ")";
        protos += sig + ";\n";
        funcs += sig + "   /* procedure " + idNode->value + " */\n{\n";
        funcs += "    struct F_" + p->cname + " frame;\n";
        funcs += "    struct F_" + p->cname + " *snl_prev = (struct F_" + p->cname + " *)snl_display[" + lv + "];\n";
        funcs += "    int snl_ret = 0;\n";
        funcs += "    memset(&frame, 0, sizeof frame);\n";
        funcs += "    snl_display[" + lv + "] = &frame;\n";
        funcs += done.paramCopies;
        funcs += body;
        funcs += "    goto snl_exit;\nsnl_exit:\n    snl_display[" + lv + "] = snl_prev;\n    return snl_ret;\n}\n\n";

        Node* more = n->son[8];
        n = isEps(more->son[0]) ? NULL : more->son[0];
    }
}

// ---------------- 变量访问 ----------------
string CodeGenerator::varAccess(Symbol* s)
{
    string base;
    if (s->level == 0) base = s->cname;                                   // 全局变量
    else if (s->level == level) base = "frame." + s->cname;               // 本过程的活动记录
    else base = "((struct F_" + ctxStack[s->level - 1].sym->cname + " *)snl_display[" + itos(s->level) + "])->" + s->cname;
    if (s->isVarParam) base = "(*" + base + ")";
    return base;
}
CodeGenerator::ExpRes CodeGenerator::simpleVar(Node* idNode)
{
    ExpRes r; r.code = "0"; r.type = NULL; r.isVar = true;
    Symbol* s = lookup(idNode->value);
    if (!s) { error(idNode->line, "变量 " + idNode->value + " 未声明"); return r; }
    if (s->kind != Symbol::VAR) { error(idNode->line, idNode->value + " 不是变量"); return r; }
    r.code = varAccess(s); r.type = s->type;
    return r;
}
CodeGenerator::ExpRes CodeGenerator::index(ExpRes base, Node* expNode, int line)
{
    ExpRes i = exp(expNode); checkScalar(i, line);
    if (!base.type) return base;
    if (base.type->kind != TypeInfo::ARRAY) { error(line, "对非数组变量使用下标"); base.type = NULL; return base; }
    base.code = base.code + ".d[(" + i.code + ") - (" + itos(base.type->low) + ")]";
    base.type = base.type->elem;
    return base;
}
CodeGenerator::ExpRes CodeGenerator::field(ExpRes base, Node* fid)
{
    if (!base.type) return base;
    if (base.type->kind != TypeInfo::RECORD) { error(fid->line, "对非记录变量访问域 " + fid->value); base.type = NULL; return base; }
    for (size_t i = 0; i < base.type->fields.size(); i++)
        if (base.type->fields[i].first == fid->value)
        {
            base.code += ".f_" + fid->value;
            base.type = base.type->fields[i].second;
            return base;
        }
    error(fid->line, "记录中没有域 " + fid->value); base.type = NULL;
    return base;
}
CodeGenerator::ExpRes CodeGenerator::variableRest(Node* idNode, Node* vm)   // id VariMore；VariMore -> ε | [ Exp ] | . FieldVar
{
    ExpRes r = simpleVar(idNode);
    if (isEps(vm->son[0])) return r;
    if (vm->son[0]->name == "[") return index(r, vm->son[1], idNode->line);
    Node* fv = vm->son[1];                        // FieldVar -> id FieldVarMore
    r = field(r, fv->son[0]);
    Node* fvm = fv->son[1];                       // FieldVarMore -> ε | [ Exp ]
    if (!isEps(fvm->son[0])) r = index(r, fvm->son[1], fv->son[0]->line);
    return r;
}

// ---------------- 表达式 ----------------
CodeGenerator::ExpRes CodeGenerator::factor(Node* n)   // Factor -> ( Exp ) | intc | Variable
{
    Node* c = n->son[0];
    if (c->name == "(") { ExpRes r = exp(n->son[1]); r.code = "(" + r.code + ")"; r.isVar = false; return r; }
    if (c->name == "intc") { ExpRes r; r.code = c->value; r.type = intType; r.isVar = false; return r; }
    return variableRest(c->son[0], c->son[1]);    // Variable -> id VariMore
}
CodeGenerator::ExpRes CodeGenerator::term(Node* n)     // Term -> Factor OtherFactor；OtherFactor -> ε | MultOp Term，展平成左结合
{
    ExpRes r = factor(n->son[0]);
    Node* other = n->son[1];
    while (!isEps(other->son[0]))
    {
        string op = other->son[0]->son[0]->name;
        Node* rhs = other->son[1];
        ExpRes t = factor(rhs->son[0]);
        checkScalar(r, lineOf(n)); checkScalar(t, lineOf(n));
        r.code = "(" + r.code + " " + op + " " + t.code + ")"; r.type = intType; r.isVar = false;
        other = rhs->son[1];
    }
    return r;
}
CodeGenerator::ExpRes CodeGenerator::exp(Node* n)      // Exp -> Term OtherTerm；OtherTerm -> ε | AddOp Exp，展平成左结合
{
    ExpRes r = term(n->son[0]);
    Node* other = n->son[1];
    while (!isEps(other->son[0]))
    {
        string op = other->son[0]->son[0]->name;
        Node* rhs = other->son[1];
        ExpRes t = term(rhs->son[0]);
        checkScalar(r, lineOf(n)); checkScalar(t, lineOf(n));
        r.code = "(" + r.code + " " + op + " " + t.code + ")"; r.type = intType; r.isVar = false;
        other = rhs->son[1];
    }
    return r;
}
string CodeGenerator::relExp(Node* n)                  // RelExp -> Exp OtherRelE；OtherRelE -> CmpOp Exp
{
    ExpRes a = exp(n->son[0]); checkScalar(a, lineOf(n));
    string op = n->son[1]->son[0]->son[0]->name;
    ExpRes b = exp(n->son[1]->son[1]); checkScalar(b, lineOf(n));
    return "(" + a.code + (op == "=" ? " == " : " < ") + b.code + ")";
}
vector<Node*> CodeGenerator::actParams(Node* n)        // ActParamList -> ε | Exp ActParamMore；ActParamMore -> ε | , ActParamList
{
    vector<Node*> v;
    while (n && !isEps(n->son[0]))
    {
        v.push_back(n->son[0]);
        Node* more = n->son[1];
        n = isEps(more->son[0]) ? NULL : more->son[1];
    }
    return v;
}

// ---------------- 语句 ----------------
void CodeGenerator::stmList(Node* n, string& out, int indent)   // StmList -> Stm StmMore；StmMore -> ε | ; StmList
{
    while (n)
    {
        stm(n->son[0], out, indent);
        Node* more = n->son[1];
        n = isEps(more->son[0]) ? NULL : more->son[1];
    }
}
void CodeGenerator::call(Node* idNode, Node* actList, string& out, const string& pad)
{
    Symbol* p = lookup(idNode->value);
    if (!p) { error(idNode->line, "过程 " + idNode->value + " 未声明"); return; }
    if (p->kind != Symbol::PROC) { error(idNode->line, idNode->value + " 不是过程"); return; }
    vector<Node*> args = actParams(actList);
    if (args.size() != p->params.size())
    {
        error(idNode->line, "过程 " + idNode->value + " 需要 " + itos((int)p->params.size()) + " 个参数，实际给了 " + itos((int)args.size()) + " 个");
        return;
    }
    string code = p->cname + "(";
    for (size_t i = 0; i < args.size(); i++)
    {
        ExpRes a = exp(args[i]);
        TypeInfo* want = p->params[i].type;
        if (i) code += ", ";
        if (p->params[i].isVar)
        {
            if (!a.isVar) { error(idNode->line, "第 " + itos((int)i + 1) + " 个参数是 var 参数，实参必须是变量"); continue; }
            if (a.type && (a.type != want && !(a.type->isScalar() && want->isScalar() && a.type->kind == want->kind)))
                error(idNode->line, "第 " + itos((int)i + 1) + " 个实参的类型与 var 形参不一致");
            code += "&" + a.code;
        }
        else
        {
            if (a.type && !sameType(a.type, want)) error(idNode->line, "第 " + itos((int)i + 1) + " 个实参的类型不匹配");
            code += a.code;
        }
    }
    out += pad + code + ");\n";
}
void CodeGenerator::stm(Node* n, string& out, int indent)
{
    Node* c = n->son[0]; string pad = ind(indent);
    if (c->name == "ConditionalStm")              // if RelExp then StmList else StmList fi
    {
        out += pad + "if " + relExp(c->son[1]) + " {\n";
        stmList(c->son[3], out, indent + 1);
        out += pad + "} else {\n";
        stmList(c->son[5], out, indent + 1);
        out += pad + "}\n";
    }
    else if (c->name == "LoopStm")                // while RelExp do StmList endwh
    {
        out += pad + "while " + relExp(c->son[1]) + " {\n";
        stmList(c->son[3], out, indent + 1);
        out += pad + "}\n";
    }
    else if (c->name == "InputStm")               // read ( Invar )，Invar -> id
    {
        Node* id = c->son[2]->son[0];
        ExpRes v = simpleVar(id);
        if (!v.type) return;
        if (!v.type->isScalar()) { error(id->line, "read 只能读入整数或字符变量"); return; }
        out += pad + "scanf(\"" + string(v.type->kind == TypeInfo::CHAR ? " %c" : "%d") + "\", &" + v.code + ");\n";
    }
    else if (c->name == "OutputStm")              // write ( Exp )
    {
        ExpRes e = exp(c->son[2]);
        if (!e.type) return;
        if (!e.type->isScalar()) { error(lineOf(c), "write 只能输出整数或字符"); return; }
        out += pad + "printf(\"" + string(e.type->kind == TypeInfo::CHAR ? "%c" : "%d") + "\\n\", " + e.code + ");\n";
    }
    else if (c->name == "ReturnStm")              // return ( Exp )
    {
        ExpRes e = exp(c->son[2]); checkScalar(e, lineOf(c));
        if (!e.type) return;
        if (level == 0) out += pad + "return 0;   /* return(" + e.code + ") */\n";
        else out += pad + "{ snl_ret = " + e.code + "; goto snl_exit; }\n";
    }
    else                                          // id AssCall；AssCall -> AssignmentRest | CallStmRest
    {
        Node* ac = n->son[1]->son[0];
        if (ac->name == "AssignmentRest")         // VariMore := Exp
        {
            ExpRes lhs = variableRest(c, ac->son[0]);
            ExpRes rhs = exp(ac->son[2]);
            if (!lhs.type || !rhs.type) return;
            if (!sameType(lhs.type, rhs.type)) { error(c->line, "赋值两边的类型不匹配"); return; }
            out += pad + lhs.code + " = " + rhs.code + ";\n";
        }
        else call(c, ac->son[1], out, pad);       // CallStmRest -> ( ActParamList )
    }
}
