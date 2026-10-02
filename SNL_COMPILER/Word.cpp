#include "Word.h"
#include<iostream>
#include<stdlib.h>
#include<stdio.h>
#include<sstream>
using namespace std;

static string lineStr(int line) //行号转字符串（原来用 curLine+0x30 拼接，只能表示 0~9）
{
	stringstream ss;
	ss<<line;
	return ss.str();
}
bool wordScanner::isChar()
{
	Token* token;
	char ch;
	bool isError=false; //本次识别是否出错。原来直接拿全局 wordErrorState 做分支，前面一旦出过错，后面所有合法的字符常量都会被误判
	ch=getChar();
    while(ch=='\n'||ch=='\r'||ch=='\t'||ch==' ')
        ch=getChar();
	if(ch!='\'')
    {
        undoChar();
		return false;
    }
	ch=getChar();          //读'后的第一个字符
    if(isNumber(ch)||isLetter(ch))
	{
        tempString+=ch;
        ch=getChar();             //读字母或数字后的第一个字符
    }
    else
	{
        tempString='\'';
        undoChar();
		isError=true;
    }

	if(!isError)
	{
		if(ch=='\'')
		{
			token=new Token(tempString,"char",curLine);
			tokenList.push(*token);
			tempString="";
			return true;
		}
		else
		{
			tempString='\'';
			undoChar();         //  回退两次，解决出现类似 '12 这种形式
			undoChar();
			isError=true;
		}
    }

	string str="程序第"+lineStr(curLine)+"行有错误单词： "+tempString;
	tempString="";
	error.push_back(str);
	wordErrorState=true;
	return false;
}

bool wordScanner::isInteger()
{
	Token* token;
	char ch;
	ch=getChar();
    while(ch=='\n'||ch=='\r'||ch=='\t'||ch==' ')
        ch=getChar();
	if(!isNumber(ch))
    {
        undoChar();
		return false;
    }
	while(isNumber(ch))
	{
        tempString+=ch;
        ch=getChar();
    }
    undoChar();     //回退文件字符指针一位
	token=new Token(tempString,"integer",curLine);
    tokenList.push(*token);
    //cout<<tempString<<endl;
	tempString="";
    return true;
}

bool wordScanner::isSingleBoundary()
{
	Token* token;
	char ch=getChar();
    while(ch=='\n'||ch=='\r'||ch=='\t'||ch==' ')
		ch=getChar();
	if((ch=='+')||(ch=='-')||(ch=='*')||(ch=='/')||(ch=='<')||(ch=='=')||(ch=='(')||(ch==')')||(ch=='[')||(ch==']')||(ch==';')||(ch==','))
	{
		tempString+=ch;
		token=new Token(tempString,"singleBoundary",curLine);
		tokenList.push(*token);
		//cout<<tempString<<endl;
		tempString="";
		return true;
	}
	else
	{
		undoChar();
		return false;
	}
}

bool wordScanner::isDoubleBoundary()
{
	Token* token;
	string str;
	char ch=getChar();
    while(ch=='\n'||ch=='\r'||ch=='\t'||ch==' ')
        ch=getChar();
	if(ch!=':')
	{
		undoChar();
		return false;
	}
	tempString+=ch;
    ch=getChar();
    if(ch=='='){
        tempString+=ch;     //双分界
		token=new Token(tempString,"doubleBoundary",curLine);
		tokenList.push(*token);
		//cout<<tempString<<endl;
		tempString="";
        return true;
    }
    else
	{
        undoChar();
        str="程序第"+lineStr(curLine)+"行有错误单词： "+tempString;
		error.push_back(str);
		wordErrorState=true;
		tempString="";
		return false;
    }
}

bool wordScanner::isNotes()
{
	char ch=getChar();
    while(ch=='\n'||ch=='\r'||ch=='\t'||ch==' ')
        ch=getChar();
	if(ch!='{')
	{
		undoChar();
		return false;
	}
	do
	{
           ch=getChar();
		   tempString+=ch;
	}while((!file.eof()&&ch!='}'));

	if(ch!='}')
	{
		 string str;
	     str="程序第";
	     str+=lineStr(curLine);
	     str+="行有错误单词： ";
	     str+=tempString;
	     tempString="";
	     error.push_back(str);
	     wordErrorState=true;
	}
	else
	{
	    //cout<<tempString<<endl;
		tempString="";
		return true;
	}
	return false;
}

int wordScanner::isArray()
{
	Token* token;
	char ch=getChar();
    while(ch=='\n'||ch=='\r'||ch=='\t'||ch==' ')
        ch=getChar();
	if(ch!='.')
	{
		undoChar();
		return false;
	}
	tempString+=ch;
    ch=getChar();
    if(ch=='.'){     //数组分界
        tempString+=ch;
		token=new Token(tempString,"arrayBound",curLine);
		tokenList.push(*token);
		tempString="";
        return true;
    }
    else{
        undoChar();     //回退文件字符指针一位
        //单独一个点：后面只剩空白直到文件尾时才是程序结束标志；否则是记录域访问 rec.x 里的 "."（原来一律当作程序结束，记录类型无法使用）
        int n=0;
        do { ch=getChar(); n++; } while(!file.eof()&&(ch=='\n'||ch=='\r'||ch=='\t'||ch==' '));
        bool atEnd=file.eof();
        while(n-->0) undoChar();
        if(atEnd)
        {
            tempString="";
            return 2;
        }
		token=new Token(tempString,"singleBoundary",curLine);
		tokenList.push(*token);
		tempString="";
        return true;
    }
}

/*总共有20个保留字，已在header.h中定义，遍历保留字数组，
与当前标识符比较，进行判断，输入的参数为可能是保留字的标识符*/
bool wordScanner::isReservedWord(string tempString)
{
	int i;
	for(i=0;i<=20;i++)
	{
		if(tempString==reservedWord[i])
			return true;
	}
	return false;
}

/*判断是否是标识符的函数，返回值为0不是标识符，1时是标识符，2时是保留字，这里将保留字看作特殊的标识符*/
int wordScanner::isID()
{
	char curChar;  //当前字符
	Token* token;
	bool isOver=false,isID=false; //isOver表示是否可以推出自动机，isID表示是否是标识符
	int transTable[2][2]={{-1,1}, {1,1}};//转换表，自动机的状态转换数组，-1表示进入错误状态
	int state=0; //自动初始状态
	curChar = getChar(); //获取一个字符
	//开始前先除掉空白字符，空格，制表符，回车
    while(curChar=='\n'||curChar=='\r'||curChar=='\t'||curChar==' ')
        curChar=getChar();
	//进入标识符判断的自动机，字母开头，数字和字母组成的字符串
	while(((curChar>=48&&curChar<=57)||(curChar>=65&&curChar<=90)||(curChar>=97&&curChar<=122))&&!isOver)//确定是否是在表示符的字符集合里
	{
		switch(state)
		{
			case 0:
				if(curChar>=48&&curChar<=57)  //状态0是初始状态，输入数字，意味着数字开头，不符合定义，退出自动机
					isOver=true;
				else
				{
					state=transTable[state][1]; //转换
                    tempString+=curChar; //生成字符串
					curChar=getChar(); //去下一个字符
					isID=true;		//字符串，一号状态时终止状态
				}
				break;
			case 1:
				if(curChar>=48&&curChar<=57) //数字
				{
					state=transTable[state][0];
                    tempString+=curChar;
					curChar=getChar();
					isID=true;
				}
				else  //字母
				{
					state=transTable[state][1];
                    tempString+=curChar;
					curChar=getChar();
					isID=true;
				}
		}
	}
	undoChar(); //回退一个字符，因为最新的字符一定不是标识符的部分
	if(isID&&isReservedWord(tempString))//是否是保留字
	{
		token=new Token(tempString,"reservedWord",curLine);
		tokenList.push(*token);
		//cout<<tempString<<endl;
		tempString="";
		return 2;
	}
	else if(isID)
	{
		token=new Token(tempString,"id",curLine);
		tokenList.push(*token);
		//cout<<tempString<<endl;
		tempString="";
		return 1;
	}
	else
		return 0;
}

void wordScanner::printResult()
{
	int i=0;
	Token* token;
	for(i=0;i<tokenList.num;i++)
	{
		token=&tokenList.List[i];
		cout<<token->name<<" "<<token->type<<" 第"<<token->line<<"行"<<endl;
	}
}

void wordScanner::createTokenFile()
{
    int i;
    Token* token;
    string str=fileName.substr(0,fileName.length()-3)+"token";
    FILE* tempFile=fopen(str.c_str(),"w");
	for(i=0;i<tokenList.num;i++)
	{
	    token=&(tokenList.List[i]);
        fprintf(tempFile,"%s %s %d\n",(token->name).c_str(),(token->type).c_str(),token->line);
	}
	fclose(tempFile);
}

void wordScanner::start()
{
    //file.open(fileName);
    cout<<"word start"<<endl;
	int ans; //判断符
	char ch;
	cout<<"----------------------词法分析---------------------"<<endl;
	while(!file.eof())//文件未结束
	{
		size_t errBefore=error.size();

		if(ans=isID()){}
		else if(ans=isChar()){}
		else if(ans=isInteger()){}
		else if(ans=isSingleBoundary()){}
		else if(ans=isDoubleBoundary()){}
		else if(ans=isNotes()){}
		else if(ans=isArray()){if(ans==2) break;}
		else
		{
			 if(error.size()>errBefore) continue; //某个子程序已经报错并回退了位置，重新从 isID 开始识别
			 ch=getChar();
			 if(file.eof()) break; //读到文件尾（源程序没有以 . 结束），正常结束
			 string str;
             str="未知错误 ";
             str+=ch;
             str+=" 第";
             str+=lineStr(curLine);
             str+="行";
             tempString="";
             error.push_back(str);
             wordErrorState=true;
		}
	}

	wordErrorState=!error.empty(); //本次只要记录过错误就置错误状态（原来到文件尾会把已有错误清掉）

	if(wordErrorState==false)
    {
        createTokenFile();
        printResult();
    }

    cout<<"word finish"<<endl;
	//system("pause");
}
