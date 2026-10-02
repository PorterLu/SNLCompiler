// Command-line entry point. On macOS / Linux it drives the original lexer (wordScanner),
// parser (GrammarAnalyzer), intermediate-code generator (CodeGenerator) and virtual machine (Vm),
// printing to stdout instead of the Windows dialogs.
//
// Usage:  snl_cli <source.txt> [--run] [--quiet] [--orig-tree]
//   * like the Windows build, it writes a sibling <source>.token file that the parser reads back
//   * after a successful parse it runs semantic analysis and writes the quadruples to <source>.ir
//   * --run       execute the intermediate code on the virtual machine (read from stdin, write to stdout)
//   * --quiet     suppress the token list, analysis steps, syntax tree and intermediate code
//   * --orig-tree print the syntax tree with Grammar.cpp's own printTree
//   * exit code: 0 success; 1 lexical / syntax / semantic / runtime error; 2 bad argument or file
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

struct StdIO : VmIO                             // VM I/O wired to standard input/output
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
    ostringstream sink;                         // with --quiet, swallow the analysis output
    streambuf* realOut = cout.rdbuf();
    if (quiet) cout.rdbuf(sink.rdbuf());

    // ---- 1. lexical analysis (GUI menu: Lexical analysis) ----
    wordScanner* scanner = new wordScanner(argv[1], NULL);
    if (!scanner->file.is_open()) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }
    scanner->start();
    if (wordErrorState) {
        cout.rdbuf(realOut);
        cout << endl << "==== Lexical errors (" << scanner->error.size() << ") ====" << endl;
        for (size_t i = 0; i < scanner->error.size(); i++) cout << scanner->error[i] << endl;
        return 1;
    }

    // ---- 2. syntax analysis (GUI menu: Syntax analysis) ----
    GrammarAnalyzer* analyzer = new GrammarAnalyzer(fileName, NULL);
    analyzer->start();
    cout << endl << "==== Analysis steps (" << analyzer->itemList.size() << ") ====" << endl;
    for (size_t i = 0; i < analyzer->itemList.size(); i++) {
        const Item& it = analyzer->itemList[i];
        cout << it.left << "\t" << it.oper << "\t" << it.right << endl;
    }
    if (grammarErrorState) {
        cout.rdbuf(realOut);
        cout << endl << "==== Syntax errors ====" << endl;
        for (size_t i = 0; i < analyzer->itemList.size(); i++)
            if (analyzer->itemList[i].oper == "error")
                cout << "line " << analyzer->itemList[i].right << ": syntax error near token "
                     << analyzer->itemList[i].left << endl;
        return 1;
    }

    // ---- 3. syntax tree (GUI menu: Parse tree) ----
    cout << endl << "==== Syntax tree ====" << endl;
    if (origTree) analyzer->printTree(analyzer->root);
    else dumpTree(analyzer->root, 0);

    // ---- 4. semantic analysis + intermediate code (GUI menu: Compile and run) ----
    CodeGenerator gen;
    if (!gen.generate(analyzer->root)) {
        cout.rdbuf(realOut);
        cout << endl << "==== Semantic errors (" << gen.errors.size() << ") ====" << endl;
        for (size_t i = 0; i < gen.errors.size(); i++) cout << gen.errors[i] << endl;
        return 1;
    }
    string irPath = stripExt(fileName) + ".ir";
    string listing = gen.ir.listing();
    ofstream irf(irPath.c_str());
    irf << listing;
    irf.close();
    cout << endl << "==== Intermediate code (quadruples): " << irPath << " ====" << endl << listing;

    // ---- 5. run on the virtual machine ----
    if (run) {
        cout << endl << "==== Run ====" << endl;
        cout.rdbuf(realOut);
        StdIO io;
        Vm vm(gen.ir, io);
        if (!vm.run()) { cout << "Runtime error: " << vm.error << endl; return 1; }
    }
    cout.rdbuf(realOut);
    return 0;
}
