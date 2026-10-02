#ifndef GRAMMAR_H_INCLUDED
#define GRAMMAR_H_INCLUDED
#include"header.h"
#include<cstdio>
#include<cstring>
#include<sstream>
#include<cstdlib>
#include<iostream>
#include<stack>

using namespace std;

extern string unUltimateSign[67];

extern string ultimateWord[40];


struct UUsign
{
    string left;
    bool isUU;
    UUsign(){isUU=true;}
};


struct Production
{
    int left;
    vector<string> right;
    void pushRight(string str)
    {
        string s;
        stringstream strcin(str);
        while(strcin>>s)
        {
            strcin.clear();
            right.push_back(s);
        }
    }
    int get(int pos)
    {
        int i;
        for(i=0;i<67;i++)
            if(right[pos]==unUltimateSign[i])
                return i;
        for(i=0;i<40;i++)
            if(right[pos]==ultimateWord[i])
                return i+67;
        return -1;
    }
};

struct Item
{
    string left;
    string oper;
    string right;
    Item()
    {
        this->left="";
        this->oper="";
        this->right="";
    }
};

struct GrammarAnalyzer
{
    Production production[104];
    int llTable[67][40]; //终结符有 40 个（".." 是第 40 个），原来 39 列会越界
    UUsign UUsignArray[107];
    TokenList tokenList;
    Node* root;
    int maxDepth;
    vector<Node*> nodeList;
    string fileName;
    stack<UUsign> analyzeStack;
    vector<string> analyzeList;
    vector<Item> itemList;
    HWND hwnd;

    void initProduction();
    void initTable();
    void initUUsign();
    int position(string str);
    void start();
    void readToken();
    void error();
    void printTree(Node* root);
    GrammarAnalyzer(string fileName,HWND hwnd)
    {
        memset(llTable,-1,sizeof(llTable));
        maxDepth=0;
        this->fileName=fileName;
        this->hwnd=hwnd;
        initUUsign();
        initProduction();
        initTable();
        readToken();

    }

    ~GrammarAnalyzer()
    {
        int i;
	    Token* token;
/*
		for(i=0;i<tokenList.List.size();i++) //释放空间
		{
			delete(&(tokenList.List[i]));
		}

        for(i=0;i<itemList.size();i++)
        {
            delete(&(itemList[i]));
        }
        for(i=0;i<nodeList.size();i++)
        {
            delete(nodeList[i]);
        }
        */
    }
};
#endif // GRAMMAR_H_INCLUDED
