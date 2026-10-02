#include "Vm.h"
#include <sstream>
using namespace std;

Vm::Vm(const IRProgram& p, VmIO& i, int memWords)
    : ir(p), io(i), mem(memWords, 0), display(64, 0), sp(0), pc(0), steps(0), maxSteps(50000000)
{
    for (size_t k = 0; k < ir.code.size(); k++)
        if (ir.code[k].op == "LABEL")
        {
            int l = ir.code[k].a.value;
            if ((int)labelPos.size() <= l) labelPos.resize(l + 1, -1);
            labelPos[l] = (int)k;
        }
}

int Vm::addr(const Operand& o)
{
    int a;
    if (o.kind == Operand::K_VAR) a = display[o.level] + o.value;
    else if (o.kind == Operand::K_REF)
    {
        int slot = display[o.level] + o.value;
        if (slot < 0 || slot >= (int)mem.size()) throw string("memory address out of range");
        a = mem[slot];                            // the slot holds the address of the actual argument
    }
    else throw string("operand is not a variable");
    if (a < 0 || a >= (int)mem.size()) throw string("memory address out of range");
    return a;
}
int Vm::val(const Operand& o)
{
    if (o.kind == Operand::K_CONST) return o.value;
    return mem[addr(o)];
}
int Vm::jump(const Operand& label)
{
    int l = label.value;
    if (l < 0 || l >= (int)labelPos.size() || labelPos[l] < 0) throw string("jump to a nonexistent label");
    return labelPos[l];
}

bool Vm::run()
{
    try
    {
        sp = ir.globalWords; pc = ir.entry; display[0] = 0;
        if (sp >= (int)mem.size()) throw string("out of memory");
        for (;;)
        {
            if (++steps > maxSteps) throw string("step limit exceeded, possibly an infinite loop");
            if (pc < 0 || pc >= (int)ir.code.size()) throw string("program counter out of range");
            const Quad& q = ir.code[pc];
            const string& op = q.op;
            pc++;
            if (op == "ADD") mem[addr(q.r)] = val(q.a) + val(q.b);
            else if (op == "SUB") mem[addr(q.r)] = val(q.a) - val(q.b);
            else if (op == "MUL") mem[addr(q.r)] = val(q.a) * val(q.b);
            else if (op == "DIV") { int d = val(q.b); if (d == 0) throw string("division by zero"); mem[addr(q.r)] = val(q.a) / d; }
            else if (op == "LT") mem[addr(q.r)] = val(q.a) < val(q.b);
            else if (op == "EQ") mem[addr(q.r)] = val(q.a) == val(q.b);
            else if (op == "MOV") mem[addr(q.r)] = val(q.a);
            else if (op == "COPY")
            {
                int s = addr(q.a), d = addr(q.r), n = q.a.size;
                if (s + n > (int)mem.size() || d + n > (int)mem.size()) throw string("memory address out of range");
                for (int k = 0; k < n; k++) mem[d + k] = mem[s + k];
            }
            else if (op == "LD")
            {
                int a = addr(q.a) + val(q.b);
                if (a < 0 || a >= (int)mem.size()) throw string("memory address out of range");
                mem[addr(q.r)] = mem[a];
            }
            else if (op == "ST")
            {
                int a = addr(q.b) + val(q.r);
                if (a < 0 || a >= (int)mem.size()) throw string("memory address out of range");
                mem[a] = val(q.a);
            }
            else if (op == "ADDR") mem[addr(q.r)] = addr(q.a) + val(q.b);
            else if (op == "JMP") pc = jump(q.a);
            else if (op == "JF") { if (val(q.a) == 0) pc = jump(q.r); }
            else if (op == "LABEL" || op == "PROC" || op == "ENDP" || op == "ENTRY") {}
            else if (op == "READ") { int v; if (!io.readInt(v)) throw string("read: no more input"); mem[addr(q.r)] = v; }
            else if (op == "READC") { char ch; if (!io.readChar(ch)) throw string("read: no more input"); mem[addr(q.r)] = (unsigned char)ch; }
            else if (op == "WRITE") { stringstream ss; ss << val(q.a) << "\n"; io.write(ss.str()); }
            else if (op == "WRITEC") { string s(1, (char)val(q.a)); io.write(s + "\n"); }
            else if (op == "WRITES") io.write(ir.strings[q.a.value] + "\n");
            else if (op == "ARG")
            {
                if (q.a.kind == Operand::K_CONST) args.push_back(q.a.value);
                else { int s = addr(q.a); for (int k = 0; k < q.a.size; k++) args.push_back(mem[s + k]); }
            }
            else if (op == "ARGREF") args.push_back(addr(q.a) + val(q.b));
            else if (op == "CALL")
            {
                const ProcInfo& p = ir.procs[q.a.value];
                int base = sp;
                if (base + p.frameWords >= (int)mem.size()) throw string("stack overflow (recursion too deep?)");
                if ((int)args.size() != p.paramWords) throw string("argument/parameter word count mismatch");
                for (int k = 0; k < p.frameWords; k++) mem[base + k] = k < (int)args.size() ? args[k] : 0;
                Frame f; f.level = p.level; f.savedDisplay = display[p.level]; f.base = base; f.retPc = pc;
                frames.push_back(f);
                display[p.level] = base;
                sp = base + p.frameWords;
                args.clear();
                pc = p.entry;
            }
            else if (op == "RET")
            {
                if (frames.empty()) throw string("RET executed outside a procedure");
                Frame f = frames.back(); frames.pop_back();
                display[f.level] = f.savedDisplay;
                sp = f.base;
                pc = f.retPc;
            }
            else if (op == "HALT") break;
            else throw string("unknown quadruple " + op);
        }
    }
    catch (string& e) { error = e; return false; }
    return true;
}
