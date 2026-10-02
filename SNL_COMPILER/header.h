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

//data structure for the token sequence
struct TokenList
{
	vector<Token> List;  //stores the tokens
	int num;			//number of tokens
	int pos;			//which token the cursor points at

	void push(Token& token) //push a token onto the sequence
	{
		List.push_back(token);
		num++;
	}

	Token get()   //fetch a token and advance the cursor by one
	{
		Token temp = List[pos];
		pos++;
		return temp;
	}

	void unget(){pos=pos>0?pos-1:0;} //move the cursor back by one

	TokenList(){num=0; pos=0;}
};

struct Node
{
    string name;
    vector<Node*> son;
    Node* father;
    int curSon;
    string value;   //the lexeme of a terminal node (identifier name, number, etc.); code generation needs it
    int line;       //the line the lexeme is on

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
