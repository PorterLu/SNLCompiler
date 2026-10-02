// 命令行入口：在 macOS / Linux 上直接驱动原有的词法分析器 (wordScanner)、语法分析器 (GrammarAnalyzer)
// 和代码生成器 (CodeGenerator)，用 stdout 代替 Windows 对话框来输出结果。
//
// 用法:  snl_cli <源文件.txt> [--orig-tree] [--build]
//   * 和 Windows 版一样，会在源文件旁边生成同名的 .token 文件，再由语法分析器读回去
//   * 语法分析成功后做语义分析并生成同名的 .c 文件（最终程序）
//   * --build  再用 cc 把生成的 C 程序编译成可执行文件 <源文件>.out
//   * --orig-tree 用 Grammar.cpp 自带的 printTree 打印语法树，否则用下面的递归版本
//   * 退出码: 0 成功；1 词法 / 语法 / 语义错误；2 参数或文件错误
#include "header.h"
#include "Word.h"
#include "Grammar.h"
#include "Gen.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
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

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <source.txt> [--orig-tree] [--build]\n", argv[0]);
        return 2;
    }
    bool origTree = false, build = false;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--orig-tree") == 0) origTree = true;
        else if (strcmp(argv[i], "--build") == 0) build = true;
    }
    string fileName = argv[1];

    // ---- 1. 词法分析（对应 GUI 菜单「词法分析」）----
    wordScanner* scanner = new wordScanner(argv[1], NULL);
    if (!scanner->file.is_open()) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }
    scanner->start();
    if (wordErrorState) {
        cout << endl << "==== 词法错误 (" << scanner->error.size() << ") ====" << endl;
        for (size_t i = 0; i < scanner->error.size(); i++)
            cout << scanner->error[i] << endl;
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
        cout << endl << "语法分析有错" << endl;
        return 1;
    }

    // ---- 3. 语法树（对应 GUI 菜单「语法树」）----
    cout << endl << "==== 语法树 ====" << endl;
    if (origTree) analyzer->printTree(analyzer->root);
    else dumpTree(analyzer->root, 0);

    // ---- 4. 语义分析 + 生成 C 程序（对应 GUI 菜单「一键编译」）----
    CodeGenerator gen;
    string code = gen.generate(analyzer->root);
    if (!gen.errors.empty()) {
        cout << endl << "==== 语义错误 (" << gen.errors.size() << ") ====" << endl;
        for (size_t i = 0; i < gen.errors.size(); i++) cout << gen.errors[i] << endl;
        return 1;
    }
    string cPath = stripExt(fileName) + ".c";
    ofstream cf(cPath.c_str());
    cf << code;
    cf.close();
    cout << endl << "==== 生成的 C 程序: " << cPath << " ====" << endl << code;

    if (build) {
        string exe = stripExt(fileName) + ".out";
        string cmd = "cc -w -o '" + exe + "' '" + cPath + "'";
        cout << endl << "==== 编译: " << cmd << " ====" << endl;
        int rc = system(cmd.c_str());
        if (rc != 0) { cout << "C 编译失败" << endl; return 1; }
        cout << "已生成可执行文件: " << exe << endl;
    }
    return 0;
}
