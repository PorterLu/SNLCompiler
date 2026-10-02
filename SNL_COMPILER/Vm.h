#ifndef VM_H_INCLUDED
#define VM_H_INCLUDED
// 中间代码虚拟机：直接执行 Gen.cpp 生成的四元式，程序不需要再经过任何别的编译器。
#include "Ir.h"

struct VmIO                                   // 程序的输入输出，由命令行版 / GUI 版各自实现
{
    virtual bool readInt(int& v) = 0;         // 返回 false 表示没有输入了
    virtual bool readChar(char& c) = 0;
    virtual void write(const string& s) = 0;
    virtual ~VmIO() {}
};

struct Vm
{
    const IRProgram& ir;
    VmIO& io;
    vector<int> mem;                          // 整个内存，以字为单位
    vector<int> display;                      // display[层次] = 该层次当前活动记录的起始地址
    vector<int> args;                         // 调用前压入的实参
    struct Frame { int level, savedDisplay, base, retPc; };
    vector<Frame> frames;                     // 控制栈
    vector<int> labelPos;
    int sp, pc;
    long steps, maxSteps;
    string error;                             // 运行错误信息

    Vm(const IRProgram& p, VmIO& i, int memWords = 1 << 20);
    bool run();                               // 正常结束返回 true；运行错误返回 false 并填 error
    int addr(const Operand& o);               // 变量操作数的地址
    int val(const Operand& o);                // 操作数的值
    int jump(const Operand& label);
};
#endif // VM_H_INCLUDED
