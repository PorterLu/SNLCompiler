// 命令行入口：在 macOS / Linux 上直接驱动原有的词法分析器 (wordScanner)、语法分析器 (GrammarAnalyzer)、
// 中间代码生成器 (CodeGenerator) 和虚拟机 (Vm)，用 stdout 代替 Windows 对话框来输出结果。
//
// 用法:  snl_cli <源文件.txt> [--run] [--quiet] [--orig-tree]
//   * 和 Windows 版一样，会在源文件旁边生成同名的 .token 文件，再由语法分析器读回去
//   * 语法分析成功后做语义分析，生成四元式中间代码并写到同名的 .ir 文件
//   * --run     在虚拟机上执行中间代码，程序的 read 从标准输入读，write 写到标准输出
//   * --quiet   不打印单词表、分析步骤、语法树和中间代码（只剩错误信息和程序输出）
//   * --orig-tree 用 Grammar.cpp 自带的 printTree 打印语法树
//   * 退出码: 0 成功；1 词法 / 语法 / 语义 / 运行错误；2 参数或文件错误
#include "header.h"
#include "Word.h"
#include "Grammar.h"
#include "Gen.h"
#include "Vm.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

static void dumpTree(const Node* node, int depth)
{
    for (int i = 0; i < depth; i++) cout << "  ";
    cout << node->name;
    if (!node->value.empty() && node->value != node->name) cout << " [" << node->value << "]";
    cout << endl;
    for (size_t i = 0; i < node->son.size(); i++)
        dumpTree(node->son[i], depth + 1);
}

static string stripExt(const string& path)
{
    size_t dot = path.rfind('.'), slash = path.find_last_of("/\\");
    if (dot == string::npos || (slash != string::npos && dot < slash)) return path;
    return path.substr(0, dot);
}

struct StdIO : VmIO                             // 虚拟机的输入输出接到标准输入输出
{
    bool readInt(int& v) { return (bool)(cin >> v); }
    bool readChar(char& c) { return (bool)(cin >> c); }
    void write(const string& s) { cout << s; cout.flush(); }
};

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <source.txt> [--run] [--quiet] [--orig-tree]\n", argv[0]);
        return 2;
    }
    bool origTree = false, run = false, quiet = false;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--orig-tree") == 0) origTree = true;
        else if (strcmp(argv[i], "--run") == 0) run = true;
        else if (strcmp(argv[i], "--quiet") == 0) quiet = true;
    }
    string fileName = argv[1];
    ostringstream sink;                         // --quiet 时把分析过程的输出吞掉
    streambuf* realOut = cout.rdbuf();
    if (quiet) cout.rdbuf(sink.rdbuf());

    // ---- 1. 词法分析（对应 GUI 菜单「词法分析」）----
    wordScanner* scanner = new wordScanner(argv[1], NULL);
    if (!scanner->file.is_open()) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }
    scanner->start();
    if (wordErrorState) {
        cout.rdbuf(realOut);
        cout << endl << "==== 词法错误 (" << scanner->error.size() << ") ====" << endl;
        for (size_t i = 0; i < scanner->error.size(); i++) cout << scanner->error[i] << endl;
        return 1;
    }

    // ---- 2. 语法分析（对应 GUI 菜单「语法分析」）----
    GrammarAnalyzer* analyzer = new GrammarAnalyzer(fileName, NULL);
    analyzer->start();
    cout << endl << "==== 分析步骤 (" << analyzer->itemList.size() << ") ====" << endl;
    for (size_t i = 0; i < analyzer->itemList.size(); i++) {
        const Item& it = analyzer->itemList[i];
        cout << it.left << "\t" << it.oper << "\t" << it.right << endl;
    }
    if (grammarErrorState) {
        cout.rdbuf(realOut);
        cout << endl << "==== 语法错误 ====" << endl;
        for (size_t i = 0; i < analyzer->itemList.size(); i++)
            if (analyzer->itemList[i].oper == "error")
                cout << "第" << analyzer->itemList[i].right << "行：单词 " << analyzer->itemList[i].left << " 附近有语法错误" << endl;
        return 1;
    }

    // ---- 3. 语法树（对应 GUI 菜单「语法树」）----
    cout << endl << "==== 语法树 ====" << endl;
    if (origTree) analyzer->printTree(analyzer->root);
    else dumpTree(analyzer->root, 0);

    // ---- 4. 语义分析 + 中间代码（对应 GUI 菜单「一键编译」）----
    CodeGenerator gen;
    if (!gen.generate(analyzer->root)) {
        cout.rdbuf(realOut);
        cout << endl << "==== 语义错误 (" << gen.errors.size() << ") ====" << endl;
        for (size_t i = 0; i < gen.errors.size(); i++) cout << gen.errors[i] << endl;
        return 1;
    }
    string irPath = stripExt(fileName) + ".ir";
    string listing = gen.ir.listing();
    ofstream irf(irPath.c_str());
    irf << listing;
    irf.close();
    cout << endl << "==== 中间代码（四元式）: " << irPath << " ====" << endl << listing;

    // ---- 5. 在虚拟机上运行 ----
    if (run) {
        cout << endl << "==== 运行 ====" << endl;
        cout.rdbuf(realOut);
        StdIO io;
        Vm vm(gen.ir, io);
        if (!vm.run()) { cout << "运行错误：" << vm.error << endl; return 1; }
    }
    cout.rdbuf(realOut);
    return 0;
}
