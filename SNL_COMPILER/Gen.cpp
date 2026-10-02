#include "Gen.h"
#include <cstdio>
#include <cstdlib>
using namespace std;

static bool isEps(Node* n) { return n->son.empty() && n->value.empty(); }   // epsilon node: no children and no token
static string itos(int v) { stringstream ss; ss << v; return ss.str(); }
static bool sameType(TypeInfo* a, TypeInfo* b) { return (a->isScalar() && b->isScalar()) || a == b; }

// ---------------- helpers ----------------
int CodeGenerator::lineOf(Node* n)          // line number of the first token in the subtree
{
    if (n->son.empty()) return n->line;
    for (size_t i = 0; i < n->son.size(); i++) { int l = lineOf(n->son[i]); if (l) return l; }
    return 0;
}
void CodeGenerator::error(int line, const string& msg) { errors.push_back("line " + itos(line) + ": " + msg); }
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
    if (scopes.back().count(s->name)) { error(line, "identifier " + s->name + " redeclared"); return false; }
    scopes.back()[s->name] = s;
    return true;
}
void CodeGenerator::checkScalar(ExpRes& r, int line)
{
    if (r.type && !r.type->isScalar()) { error(line, "expression here must be integer or char"); r.type = NULL; }
}
void CodeGenerator::emit(const string& op, const Operand& a, const Operand& b, const Operand& r)
{
    Quad q; q.op = op; q.a = a; q.b = b; q.r = r;
    ir.code.push_back(q);
}
Operand CodeGenerator::newTemp()
{
    Operand t = Operand::var("t" + itos(tempTop - tempBase + 1), level, tempTop, 1);
    tempTop++;
    if (tempTop > tempMax) tempMax = tempTop;
    return t;
}
int CodeGenerator::newLabel() { return ++labelCount; }
Operand CodeGenerator::operandOf(Symbol* s)
{
    if (s->isVarParam) return Operand::ref(s->name, s->level, s->offset, s->type->size);
    return Operand::var(s->name, s->level, s->offset, s->type->size);
}

// ---------------- whole program ----------------
bool CodeGenerator::generate(Node* root)
{
    errors.clear(); scopes.clear(); saved.clear(); ir = IRProgram();
    level = 0; nextOffset = 0; tempBase = tempTop = tempMax = 0; labelCount = 0;
    intType = new TypeInfo(TypeInfo::INT);
    charType = new TypeInfo(TypeInfo::CHAR);
    // Program -> ProgramHead DeclarePart ProgramBody
    string progName = root->son[0]->son[1]->son[0]->value;
    pushScope();
    declarePart(root->son[1]);
    tempBase = tempTop = tempMax = nextOffset;        // the main program's temporaries go after the global variables
    ir.entry = (int)ir.code.size();
    emit("ENTRY", Operand::proc(-1, progName));
    stmList(root->son[2]->son[1]);                    // ProgramBody -> begin StmList end
    emit("HALT");
    ir.globalWords = tempMax;
    popScope();
    return errors.empty();
}

// ---------------- declarations ----------------
void CodeGenerator::declarePart(Node* n)          // DeclarePart -> TypeDec VarDec ProcDec
{
    Node* typeDec = n->son[0]; Node* varDec = n->son[1]; Node* procDec = n->son[2];
    if (!isEps(typeDec->son[0])) typeDecList(typeDec->son[0]->son[1]);     // TypeDeclaration -> type TypeDecList
    if (!isEps(varDec->son[0])) varDecList(varDec->son[0]->son[1]);        // VarDeclaration -> var VarDecList
    if (!isEps(procDec->son[0])) procDeclaration(procDec->son[0]);         // ProcDec -> ProcDeclaration
}

void CodeGenerator::typeDecList(Node* n)          // TypeDecList -> TypeId = TypeName ; TypeDecMore
{
    while (n)
    {
        Node* idNode = n->son[0]->son[0];
        TypeInfo* t = typeName(n->son[2]);
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

TypeInfo* CodeGenerator::typeName(Node* n)       // TypeName -> BaseType | StructureType | id
{
    Node* c = n->son[0];
    if (c->name == "BaseType") return baseType(c);
    if (c->name == "StructureType")
        return c->son[0]->name == "ArrayType" ? arrayType(c->son[0]) : recType(c->son[0]);
    Symbol* s = lookup(c->value);
    if (!s) { error(c->line, "type " + c->value + " is not declared"); return NULL; }
    if (s->kind != Symbol::TYPE) { error(c->line, c->value + " is not a type name"); return NULL; }
    return s->type;
}
TypeInfo* CodeGenerator::baseType(Node* n) { return n->son[0]->name == "integer" ? intType : charType; }
TypeInfo* CodeGenerator::arrayType(Node* n)       // ArrayType -> array [ Low .. Top ] of BaseType
{
    int low = atoi(n->son[2]->son[0]->value.c_str());
    int high = atoi(n->son[4]->son[0]->value.c_str());
    if (high < low) { error(lineOf(n), "array upper bound is below lower bound"); return NULL; }
    TypeInfo* t = new TypeInfo(TypeInfo::ARRAY);
    t->low = low; t->high = high; t->elem = baseType(n->son[7]);
    t->size = high - low + 1;
    return t;
}
TypeInfo* CodeGenerator::recType(Node* n)         // RecType -> record FieldDecList end
{
    TypeInfo* t = new TypeInfo(TypeInfo::RECORD);
    t->size = 0;
    Node* f = n->son[1];
    while (f)                                     // FieldDecList -> (BaseType | ArrayType) IdList ; FieldDecMore
    {
        TypeInfo* ft = f->son[0]->name == "BaseType" ? baseType(f->son[0]) : arrayType(f->son[0]);
        vector<Node*> ids = idList(f->son[1]);
        for (size_t i = 0; i < ids.size() && ft; i++)
        {
            if (t->findField(ids[i]->value) >= 0) { error(ids[i]->line, "record field " + ids[i]->value + " is duplicated"); continue; }
            t->fieldNames.push_back(ids[i]->value);
            t->fieldTypes.push_back(ft);
            t->fieldOffsets.push_back(t->size);
            t->size += ft->size;
        }
        Node* more = f->son[3];
        f = isEps(more->son[0]) ? NULL : more->son[0];
    }
    if (t->size == 0) t->size = 1;
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

void CodeGenerator::varDecList(Node* n)           // VarDecList -> TypeName VarIdList ; VarDecMore
{
    while (n)
    {
        TypeInfo* t = typeName(n->son[0]);
        vector<Node*> ids = idList(n->son[1]);
        for (size_t i = 0; i < ids.size() && t; i++)
        {
            Symbol* s = new Symbol(Symbol::VAR, ids[i]->value);
            s->type = t; s->level = level; s->offset = nextOffset;
            if (declare(s, ids[i]->line)) nextOffset += t->size;
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
        TypeInfo* t = typeName(param->son[isVar ? 1 : 0]);
        vector<Node*> ids = idList(param->son[isVar ? 2 : 1]);
        for (size_t i = 0; i < ids.size() && t; i++)
        {
            Symbol* s = new Symbol(Symbol::VAR, ids[i]->value);
            s->type = t; s->level = level; s->isVarParam = isVar; s->offset = nextOffset;
            if (!declare(s, ids[i]->line)) continue;
            nextOffset += isVar ? 1 : t->size;     // a var parameter takes only one word (an address)
            ParamInfo pi; pi.type = t; pi.isVar = isVar; p->params.push_back(pi);
        }
        Node* more = n->son[1];
        n = isEps(more->son[0]) ? NULL : more->son[1];   // ParamMore -> ; ParamDecList
    }
}

// ProcDeclaration -> procedure ProcName ( ParamList ) ; ProcDecPart ProcBody ProcDecMore
void CodeGenerator::procDeclaration(Node* n)
{
    while (n)
    {
        Node* idNode = n->son[1]->son[0];
        Symbol* p = new Symbol(Symbol::PROC, idNode->value);
        p->level = level + 1;
        p->procIndex = (int)ir.procs.size();
        ProcInfo info; info.name = idNode->value; info.level = p->level; info.entry = 0; info.paramWords = 0; info.frameWords = 0;
        ir.procs.push_back(info);
        declare(p, idNode->line);                  // the procedure name belongs to the enclosing scope; declare it first so the body can call it recursively

        saved.push_back(nextOffset); saved.push_back(tempBase); saved.push_back(tempTop); saved.push_back(tempMax);
        level++; pushScope(); nextOffset = 0;
        if (!isEps(n->son[3]->son[0])) paramDecList(n->son[3]->son[0], p);   // ParamList -> ParamDecList
        ir.procs[p->procIndex].paramWords = nextOffset;
        declarePart(n->son[6]->son[0]);           // ProcDecPart -> DeclarePart (may contain nested procedures)
        tempBase = tempTop = tempMax = nextOffset; // temporaries go after the local variables
        ir.procs[p->procIndex].entry = (int)ir.code.size();
        emit("PROC", Operand::proc(p->procIndex, p->name));
        stmList(n->son[7]->son[0]->son[1]);        // ProcBody -> ProgramBody -> begin StmList end
        emit("RET");
        emit("ENDP", Operand::proc(p->procIndex, p->name));
        ir.procs[p->procIndex].frameWords = tempMax;
        popScope(); level--;
        tempMax = saved.back(); saved.pop_back(); tempTop = saved.back(); saved.pop_back();
        tempBase = saved.back(); saved.pop_back(); nextOffset = saved.back(); saved.pop_back();

        Node* more = n->son[8];
        n = isEps(more->son[0]) ? NULL : more->son[0];
    }
}

// ---------------- variables (lvalues) ----------------
LValue CodeGenerator::lvalue(Node* idNode, Node* vm)     // id VariMore; VariMore -> epsilon | [ Exp ] | . FieldVar
{
    LValue lv;
    Symbol* s = lookup(idNode->value);
    if (!s) { error(idNode->line, "variable " + idNode->value + " is not declared"); return lv; }
    if (s->kind != Symbol::VAR) { error(idNode->line, idNode->value + " is not a variable"); return lv; }
    lv.base = operandOf(s); lv.type = s->type;
    if (!vm || isEps(vm->son[0])) return lv;
    if (vm->son[0]->name == "[") return indexInto(lv, vm->son[1], idNode->line);
    Node* fv = vm->son[1];                        // FieldVar -> id FieldVarMore
    lv = fieldOf(lv, fv->son[0]);
    Node* fvm = fv->son[1];                       // FieldVarMore -> epsilon | [ Exp ]
    if (!isEps(fvm->son[0])) lv = indexInto(lv, fvm->son[1], fv->son[0]->line);
    return lv;
}
LValue CodeGenerator::indexInto(LValue lv, Node* expNode, int line)
{
    ExpRes i = exp(expNode); checkScalar(i, line);
    Operand iv = rvalue(i);
    if (!lv.type) return lv;
    if (lv.type->kind != TypeInfo::ARRAY) { error(line, "subscript used on a non-array variable"); lv.type = NULL; return lv; }
    Operand off;                                   // offset = index - lower bound
    if (iv.kind == Operand::K_CONST) off = Operand::constant(iv.value - lv.type->low);
    else { off = newTemp(); emit("SUB", iv, Operand::constant(lv.type->low), off); }
    if (lv.hasOff)                                 // array field inside a record: add the field offset as well
    {
        if (off.kind == Operand::K_CONST && lv.off.kind == Operand::K_CONST) off = Operand::constant(off.value + lv.off.value);
        else { Operand t = newTemp(); emit("ADD", off, lv.off, t); off = t; }
    }
    lv.off = off; lv.hasOff = true; lv.type = lv.type->elem;
    return lv;
}
LValue CodeGenerator::fieldOf(LValue lv, Node* fid)
{
    if (!lv.type) return lv;
    if (lv.type->kind != TypeInfo::RECORD) { error(fid->line, "field access on a non-record variable: " + fid->value); lv.type = NULL; return lv; }
    int k = lv.type->findField(fid->value);
    if (k < 0) { error(fid->line, "record has no field " + fid->value); lv.type = NULL; return lv; }
    lv.off = Operand::constant(lv.type->fieldOffsets[k]); lv.hasOff = true;
    lv.type = lv.type->fieldTypes[k];
    return lv;
}
Operand CodeGenerator::rvalue(ExpRes& e)          // turn an expression result into a directly usable operand (emitting a load if needed)
{
    if (!e.isVar) return e.opnd;                   // a constant or an already computed temporary
    LValue& lv = e.lv;
    if (!lv.type) return Operand::constant(0);
    if (!lv.hasOff) return lv.base;                // the whole variable
    Operand t = newTemp();
    if (lv.type->isScalar()) { emit("LD", lv.base, lv.off, t); return t; }
    emit("ADDR", lv.base, lv.off, t);              // a whole array field inside a record: take its address and use it by reference
    return Operand::ref(t.name, t.level, t.value, lv.type->size);
}

// ---------------- expressions ----------------
ExpRes CodeGenerator::binary(const string& op, ExpRes& a, ExpRes& b, int line)
{
    checkScalar(a, line); checkScalar(b, line);
    Operand x = rvalue(a), y = rvalue(b);
    ExpRes r; r.type = intType; r.isVar = false;
    if (x.kind == Operand::K_CONST && y.kind == Operand::K_CONST && !(op == "DIV" && y.value == 0))   // constant folding
    {
        int v = op == "ADD" ? x.value + y.value : op == "SUB" ? x.value - y.value : op == "MUL" ? x.value * y.value : x.value / y.value;
        r.opnd = Operand::constant(v);
        return r;
    }
    r.opnd = newTemp();
    emit(op, x, y, r.opnd);
    return r;
}
ExpRes CodeGenerator::factor(Node* n)             // Factor -> ( Exp ) | intc | Variable
{
    Node* c = n->son[0];
    if (c->name == "(") return exp(n->son[1]);
    if (c->name == "intc") { ExpRes r; r.opnd = Operand::constant(atoi(c->value.c_str())); r.type = intType; r.isVar = false; return r; }
    ExpRes r; r.isVar = true;
    r.lv = lvalue(c->son[0], c->son[1]);           // Variable -> id VariMore
    r.type = r.lv.type;
    return r;
}
ExpRes CodeGenerator::term(Node* n)               // Term -> Factor OtherFactor; OtherFactor -> epsilon | MultOp Term, flattened to be left-associative
{
    ExpRes r = factor(n->son[0]);
    Node* other = n->son[1];
    while (!isEps(other->son[0]))
    {
        string op = other->son[0]->son[0]->name == "*" ? "MUL" : "DIV";
        Node* rhs = other->son[1];
        ExpRes t = factor(rhs->son[0]);
        r = binary(op, r, t, lineOf(n));
        other = rhs->son[1];
    }
    return r;
}
ExpRes CodeGenerator::exp(Node* n)                // Exp -> Term OtherTerm; OtherTerm -> epsilon | AddOp Exp, flattened to be left-associative
{
    ExpRes r = term(n->son[0]);
    Node* other = n->son[1];
    while (!isEps(other->son[0]))
    {
        string op = other->son[0]->son[0]->name == "+" ? "ADD" : "SUB";
        Node* rhs = other->son[1];
        ExpRes t = term(rhs->son[0]);
        r = binary(op, r, t, lineOf(n));
        other = rhs->son[1];
    }
    return r;
}
Operand CodeGenerator::relExp(Node* n)            // RelExp -> Exp OtherRelE; OtherRelE -> CmpOp Exp
{
    ExpRes a = exp(n->son[0]); checkScalar(a, lineOf(n));
    string op = n->son[1]->son[0]->son[0]->name == "=" ? "EQ" : "LT";
    ExpRes b = exp(n->son[1]->son[1]); checkScalar(b, lineOf(n));
    Operand x = rvalue(a), y = rvalue(b);
    Operand t = newTemp();
    emit(op, x, y, t);
    return t;
}
vector<Node*> CodeGenerator::actParams(Node* n)   // ActParamList -> epsilon | Exp ActParamMore; ActParamMore -> epsilon | , ActParamList
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

// ---------------- statements ----------------
void CodeGenerator::stmList(Node* n)              // StmList -> Stm StmMore; StmMore -> epsilon | ; StmList
{
    while (n)
    {
        stm(n->son[0]);
        Node* more = n->son[1];
        n = isEps(more->son[0]) ? NULL : more->son[1];
    }
}
void CodeGenerator::call(Node* idNode, Node* actList)
{
    Symbol* p = lookup(idNode->value);
    if (!p) { error(idNode->line, "procedure " + idNode->value + " is not declared"); return; }
    if (p->kind != Symbol::PROC) { error(idNode->line, idNode->value + " is not a procedure"); return; }
    vector<Node*> args = actParams(actList);
    if (args.size() != p->params.size())
    {
        error(idNode->line, "procedure " + idNode->value + " needs " + itos((int)p->params.size()) + " arguments but got " + itos((int)args.size()) + "");
        return;
    }
    for (size_t i = 0; i < args.size(); i++)
    {
        ExpRes a = exp(args[i]);
        TypeInfo* want = p->params[i].type;
        if (p->params[i].isVar)
        {
            if (!a.isVar) { error(idNode->line, "argument " + itos((int)i + 1) + " is a var parameter; the actual argument must be a variable"); continue; }
            if (a.type && a.type != want && !(a.type->isScalar() && want->isScalar() && a.type->kind == want->kind))
                error(idNode->line, "argument " + itos((int)i + 1) + " type does not match the var parameter");
            if (!a.lv.type) continue;
            emit("ARGREF", a.lv.base, a.lv.hasOff ? a.lv.off : Operand::constant(0));
        }
        else
        {
            if (a.type && !sameType(a.type, want)) error(idNode->line, "argument " + itos((int)i + 1) + " has a mismatched type");
            emit("ARG", rvalue(a));
        }
    }
    emit("CALL", Operand::proc(p->procIndex, p->name));
}
void CodeGenerator::stm(Node* n)
{
    tempTop = tempBase;                           // temporaries live only within one statement; each statement reuses them from the start
    Node* c = n->son[0];
    if (c->name == "ConditionalStm")              // if RelExp then StmList else StmList fi
    {
        int lElse = newLabel(), lEnd = newLabel();
        Operand cond = relExp(c->son[1]);
        emit("JF", cond, Operand(), Operand::label(lElse));
        stmList(c->son[3]);
        emit("JMP", Operand::label(lEnd));
        emit("LABEL", Operand::label(lElse));
        stmList(c->son[5]);
        emit("LABEL", Operand::label(lEnd));
    }
    else if (c->name == "LoopStm")                // while RelExp do StmList endwh
    {
        int lStart = newLabel(), lEnd = newLabel();
        emit("LABEL", Operand::label(lStart));
        Operand cond = relExp(c->son[1]);
        emit("JF", cond, Operand(), Operand::label(lEnd));
        stmList(c->son[3]);
        emit("JMP", Operand::label(lStart));
        emit("LABEL", Operand::label(lEnd));
    }
    else if (c->name == "InputStm")               // read ( Invar ), Invar -> id
    {
        Node* id = c->son[2]->son[0];
        LValue lv = lvalue(id, NULL);
        if (!lv.type) return;
        if (!lv.type->isScalar()) { error(id->line, "read accepts only an integer or char variable"); return; }
        emit(lv.type->kind == TypeInfo::CHAR ? "READC" : "READ", Operand(), Operand(), lv.base);
    }
    else if (c->name == "OutputStm")              // write ( OutputRest; OutputRest -> Exp ) | string )
    {
        Node* rest = c->son[2];
        if (rest->son[0]->name == "string")       // language extension: output a string constant
        {
            ir.strings.push_back(rest->son[0]->value);
            emit("WRITES", Operand::str((int)ir.strings.size() - 1, rest->son[0]->value));
            return;
        }
        ExpRes e = exp(rest->son[0]);
        if (!e.type) return;
        if (!e.type->isScalar()) { error(lineOf(c), "write accepts only an integer or char"); return; }
        emit(e.type->kind == TypeInfo::CHAR ? "WRITEC" : "WRITE", rvalue(e));
    }
    else if (c->name == "ReturnStm")              // return ( Exp )
    {
        ExpRes e = exp(c->son[2]); checkScalar(e, lineOf(c));
        if (!e.type) return;
        if (level == 0) emit("HALT");
        else emit("RET", rvalue(e));
    }
    else                                          // id AssCall; AssCall -> AssignmentRest | CallStmRest
    {
        Node* ac = n->son[1]->son[0];
        if (ac->name == "AssignmentRest")         // VariMore := Exp
        {
            LValue lv = lvalue(c, ac->son[0]);
            ExpRes rhs = exp(ac->son[2]);
            if (!lv.type || !rhs.type) return;
            if (!sameType(lv.type, rhs.type)) { error(c->line, "assignment type mismatch"); return; }
            Operand v = rvalue(rhs);
            if (lv.type->isScalar())
            {
                if (!lv.hasOff) emit("MOV", v, Operand(), lv.base);
                else emit("ST", v, lv.base, lv.off);
            }
            else                                  // whole-array or whole-record assignment
            {
                if (!lv.hasOff) emit("COPY", v, Operand(), lv.base);
                else
                {
                    Operand t = newTemp();
                    emit("ADDR", lv.base, lv.off, t);
                    emit("COPY", v, Operand(), Operand::ref(t.name, t.level, t.value, lv.type->size));
                }
            }
        }
        else call(c, ac->son[1]);                 // CallStmRest -> ( ActParamList )
    }
}
