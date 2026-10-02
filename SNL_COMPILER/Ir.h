#ifndef IR_H_INCLUDED
#define IR_H_INCLUDED
// 中间代码（四元式）的数据结构。代码生成器 (Gen.cpp) 生成它，虚拟机 (Vm.cpp) 直接执行它。
//
// 运行模型：内存是一个整数字数组。主程序的变量从地址 0 开始；每次调用过程在栈上分配一个活动记录，
// 布局是 [形参 | 局部变量 | 临时变量]；display[层次] 指向该层次当前的活动记录，嵌套过程由此访问
// 外层过程的变量。var 形参占一个字，存的是实参的地址。
#include "header.h"
#include <sstream>
using namespace std;

struct Operand
{
    enum Kind { K_NONE, K_CONST, K_VAR, K_REF, K_LABEL, K_PROC };   // 加前缀是为了避开 windows.h 里的 CONST 宏
    Kind kind;
    int value;      // CONST：常量值；VAR / REF：在活动记录里的字偏移；LABEL：标号；PROC：过程编号
    int level;      // VAR / REF：所在层次，0 是主程序
    int size;       // VAR / REF：所指对象占的字数，数组和记录大于 1
    string name;    // 打印用
    Operand() : kind(K_NONE), value(0), level(0), size(1) {}
    static Operand constant(int v) { Operand o; o.kind = K_CONST; o.value = v; return o; }
    static Operand var(const string& n, int lv, int off, int sz) { Operand o; o.kind = K_VAR; o.name = n; o.level = lv; o.value = off; o.size = sz; return o; }
    static Operand ref(const string& n, int lv, int off, int sz) { Operand o; o.kind = K_REF; o.name = n; o.level = lv; o.value = off; o.size = sz; return o; }
    static Operand label(int n) { Operand o; o.kind = K_LABEL; o.value = n; return o; }
    static Operand proc(int n, const string& nm) { Operand o; o.kind = K_PROC; o.value = n; o.name = nm; return o; }
    string str(int curLevel) const
    {
        stringstream ss;
        switch (kind)
        {
        case K_NONE:  return "-";
        case K_CONST: ss << "#" << value; return ss.str();
        case K_LABEL: ss << "L" << value; return ss.str();
        case K_PROC:  return name;
        default:
            ss << name;
            if (level != 0 && level != curLevel) ss << "@" << level;   // 外层过程的变量
            return ss.str();
        }
    }
};

struct Quad { string op; Operand a, b, r; };

struct ProcInfo
{
    string name;
    int level;          // 过程体的层次（顶层过程为 1）
    int entry;          // PROC 四元式的下标，调用时从这里开始执行
    int paramWords;     // 形参占的字数
    int frameWords;     // 整个活动记录的字数
};

struct IRProgram
{
    vector<Quad> code;
    vector<ProcInfo> procs;
    int globalWords;    // 主程序的变量和临时变量占的字数
    int entry;          // 主程序第一条四元式的下标
    IRProgram() : globalWords(0), entry(0) {}

    // 四元式指令集：
    //   ADD/SUB/MUL/DIV a,b,r   r := a op b          LT/EQ a,b,r   r := (a<b) / (a=b)，结果 0 或 1
    //   MOV a,-,r               r := a               COPY a,-,r    整块复制 a.size 个字（数组、记录赋值）
    //   LD base,off,r           r := base[off]       ST v,base,off base[off] := v
    //   ADDR base,off,r         r := base 的地址 + off（之后 r 作为 REF 使用）
    //   LABEL L / JMP L / JF a,-,L（a 为 0 时跳转）
    //   READ/READC -,-,r        读入整数 / 字符      WRITE/WRITEC a  输出整数 / 字符并换行
    //   ARG a（按值传递，整块压栈） / ARGREF base,off（var 参数，压地址） / CALL p / RET [a]
    //   PROC p ... ENDP p       过程体的范围         ENTRY / HALT  主程序的开始与结束
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
