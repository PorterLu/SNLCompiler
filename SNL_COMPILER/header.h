#ifndef HEADER_H_INCLUDED
#define HEADER_H_INCLUDED
#include <windows.h>
#include<vector>
#include<string>
#include<cmath>
#include<iostream>
using namespace std;
#define UNTITLED TEXT ("(untitled)")
extern bool wordErrorState;
extern bool grammarErrorState;
struct Token
{
    string name;
	string type;
	int line;
	Token(string name,string type,int line)
	{
		this->name=name;
		this->type=type;
		this->line=line;
	}
};

//Token序列的数据结构
struct TokenList
{
	vector<Token> List;  //用于存储token
	int num;			//token长度
	int pos;			//当前的指针指向哪一个token

	void push(Token& token) //向序列压入一个token
	{
		List.push_back(token);
		num++;
	}

	Token get()   //取得一个token，指针后移一位
	{
		Token temp = List[pos];
		pos++;
		return temp;
	}

	void unget(){pos=pos>0?pos-1:0;} //指针回退一位

	TokenList(){num=0; pos=0;}
};

struct Node
{
    string name;
    vector<Node*> son;
    Node* father;
    int curSon;
    string value;   //终结符结点对应的单词（标识符名、数值等），代码生成要用
    int line;       //该单词所在的行

    Node(string n){
        name=n;
        father=NULL;
        curSon=0;
        value="";
        line=0;
        son.clear();
    }

    Node* getSon(){
        int tempNum=son.size()-1;
        if(curSon>tempNum)
            return NULL;
        Node* temp=son[curSon];
        curSon++;
        return temp;
    }

};


#endif // HEADER_H_INCLUDED
