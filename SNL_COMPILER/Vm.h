#ifndef VM_H_INCLUDED
#define VM_H_INCLUDED
// intermediate-code virtual machine: executes the quadruples produced by Gen.cpp directly, so the program needs no other compiler.
#include "Ir.h"

struct VmIO                                   // the program's I/O, implemented separately by the command-line and GUI versions
{
    virtual bool readInt(int& v) = 0;         // returns false when there is no more input
    virtual bool readChar(char& c) = 0;
    virtual void write(const string& s) = 0;
    virtual ~VmIO() {}
};

struct Vm
{
    const IRProgram& ir;
    VmIO& io;
    vector<int> mem;                          // the whole memory, in words
    vector<int> display;                      // display[level] = start address of that level's current activation record
    vector<int> args;                         // actual arguments pushed before a call
    struct Frame { int level, savedDisplay, base, retPc; };
    vector<Frame> frames;                     // control stack
    vector<int> labelPos;
    int sp, pc;
    long steps, maxSteps;
    string error;                             // runtime error message

    Vm(const IRProgram& p, VmIO& i, int memWords = 1 << 20);
    bool run();                               // returns true on normal termination; on a runtime error returns false and fills in error
    int addr(const Operand& o);               // address of a variable operand
    int val(const Operand& o);                // value of an operand
    int jump(const Operand& label);
};
#endif // VM_H_INCLUDED
