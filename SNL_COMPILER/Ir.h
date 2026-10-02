#ifndef IR_H_INCLUDED
#define IR_H_INCLUDED
// data structures for the intermediate code (quadruples). The generator (Gen.cpp) produces it and the virtual machine (Vm.cpp) executes it directly.
//
// run model: memory is an array of integer words. The main program's variables start at address 0; each call allocates an activation record on the stack,
// laid out as [params | locals | temporaries]; display[level] points at the current activation record of that level, through which a nested procedure reaches
// an enclosing procedure's variables. A var parameter takes one word holding the address of the actual argument.
#include "header.h"
#include <sstream>
using namespace std;

struct Operand
{
    enum Kind { K_NONE, K_CONST, K_VAR, K_REF, K_LABEL, K_PROC, K_STR };   // the prefix avoids the CONST macro defined in windows.h
    Kind kind;
    int value;      // CONST: constant value; VAR / REF: word offset in the activation record; LABEL: label; PROC: procedure number
    int level;      // VAR / REF: the level, 0 is the main program
    int size;       // VAR / REF: size in words of the referenced object, more than 1 for arrays and records
    string name;    // for printing
    Operand() : kind(K_NONE), value(0), level(0), size(1) {}
    static Operand constant(int v) { Operand o; o.kind = K_CONST; o.value = v; return o; }
    static Operand var(const string& n, int lv, int off, int sz) { Operand o; o.kind = K_VAR; o.name = n; o.level = lv; o.value = off; o.size = sz; return o; }
    static Operand ref(const string& n, int lv, int off, int sz) { Operand o; o.kind = K_REF; o.name = n; o.level = lv; o.value = off; o.size = sz; return o; }
    static Operand label(int n) { Operand o; o.kind = K_LABEL; o.value = n; return o; }
    static Operand proc(int n, const string& nm) { Operand o; o.kind = K_PROC; o.value = n; o.name = nm; return o; }
    static Operand str(int n, const string& text) { Operand o; o.kind = K_STR; o.value = n; o.name = text; return o; }
    string str(int curLevel) const
    {
        stringstream ss;
        switch (kind)
        {
        case K_NONE:  return "-";
        case K_CONST: ss << "#" << value; return ss.str();
        case K_LABEL: ss << "L" << value; return ss.str();
        case K_PROC:  return name;
        case K_STR:   return "\"" + name + "\"";
        default:
            ss << name;
            if (level != 0 && level != curLevel) ss << "@" << level;   // an enclosing procedure's variable
            return ss.str();
        }
    }
};

struct Quad { string op; Operand a, b, r; };

struct ProcInfo
{
    string name;
    int level;          // level of the procedure body (1 for a top-level procedure)
    int entry;          // index of the PROC quadruple; a call starts executing here
    int paramWords;     // size of the parameters in words
    int frameWords;     // size of the whole activation record in words
};

struct IRProgram
{
    vector<Quad> code;
    vector<ProcInfo> procs;
    vector<string> strings;   // string constant table; a WRITES operand is an index into it
    int globalWords;    // size in words of the main program's variables and temporaries
    int entry;          // index of the main program's first quadruple
    IRProgram() : globalWords(0), entry(0) {}

    // quadruple instruction set:
    //   ADD/SUB/MUL/DIV a,b,r   r := a op b          LT/EQ a,b,r   r := (a<b) / (a=b), result 0 or 1
    //   MOV a,-,r               r := a               COPY a,-,r    block-copy a.size words (array/record assignment)
    //   LD base,off,r           r := base[off]       ST v,base,off base[off] := v
    //   ADDR base,off,r         r := address of base + off (r is then used as a REF)
    //   LABEL L / JMP L / JF a,-,L (jump when a is 0)
    //   READ/READC -,-,r        read an integer / char      WRITE/WRITEC a  write an integer / char and a newline
    //   WRITES a                write a string constant and a newline (a is an index into the string table)
    //   ARG a (pass by value, push the whole block) / ARGREF base,off (var parameter, push the address) / CALL p / RET [a]
    //   PROC p ... ENDP p       extent of the procedure body         ENTRY / HALT  start and end of the main program
    string listing() const
    {
        stringstream ss;
        for (size_t i = 0; i < procs.size(); i++)
            ss << "; 过程 " << procs[i].name << ": 层次 " << procs[i].level << ", 入口 " << procs[i].entry
               << ", 形参 " << procs[i].paramWords << " 字, 活动记录 " << procs[i].frameWords << " 字\n";
        ss << "; 主程序: 入口 " << entry << ", 变量区 " << globalWords << " 字\n";
        int lv = 0;
        for (size_t i = 0; i < code.size(); i++)
        {
            const Quad& q = code[i];
            if (q.op == "PROC") lv = procs[q.a.value].level;
            ss.width(4); ss << i << ": (" << q.op << ", " << q.a.str(lv) << ", " << q.b.str(lv) << ", " << q.r.str(lv) << ")\n";
            if (q.op == "ENDP") lv = 0;
        }
        return ss.str();
    }
};
#endif // IR_H_INCLUDED
