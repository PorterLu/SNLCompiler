// 命令行入口：在 macOS / Linux 上直接驱动原有的词法分析器 (wordScanner) 和
// 语法分析器 (GrammarAnalyzer)，用 stdout 代替 Windows 对话框来输出结果。
// 原有的 Word.cpp / Grammar.cpp 一行未改，只是把 main.cpp 里的 Win32 GUI 换成了这个文件。
//
// 用法:  snl_cli <源文件.txt> [--orig-tree]
//   * 和 Windows 版一样，会在源文件旁边生成同名的 .token 文件，再由语法分析器读回去
//   * --orig-tree 时用 Grammar.cpp 自带的 printTree 打印语法树，否则用下面的递归版本
//   * 退出码: 0 成功；1 词法或语法错误；2 参数/文件错误
#include "header.h"
#include "Word.h"
#include "Grammar.h"
#include <cstdio>
#include <cstring>
#include <iostream>
using namespace std;

static void dumpTree(const Node* node, int depth)
{
    for (int i = 0; i < depth; i++) cout << "  ";
    cout << node->name << endl;
    for (size_t i = 0; i < node->son.size(); i++)
        dumpTree(node->son[i], depth + 1);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <source.txt> [--orig-tree]\n", argv[0]);
        return 2;
    }
    bool origTree = (argc > 2 && strcmp(argv[2], "--orig-tree") == 0);
    string fileName = argv[1];

    // ---- 1. 词法分析（对应 GUI 菜单「词法分析」）----
    wordScanner* scanner = new wordScanner(argv[1], NULL);
    if (!scanner->file.is_open()) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }
    scanner->start();   // 打印 token 列表，并生成 .token 文件
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
    if (origTree)
        analyzer->printTree(analyzer->root);
    else
        dumpTree(analyzer->root, 0);
    return 0;
}
