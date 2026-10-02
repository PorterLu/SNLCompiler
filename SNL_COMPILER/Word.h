#ifndef WORD_H_INCLUDED
#define WORD_H_INCLUDED

#include<string.h>
#include<fstream>
#include<cstdlib>
#include<iostream>
#include"header.h"
using namespace std;

struct wordScanner
{
    int note;

	//保留字数组
	string reservedWord[21]={"program","procedure","type","var","if","then","else","fi",
		"while","do","endwh","begin","end","read","write","array",
		"of","record","return","integer","char"};

	ifstream file;
	HWND hwnd;
	string fileName;
	string tempString;//取得的单词
	int curLine;//当前行
	int offset;//文件偏移
	TokenList tokenList;
	vector<string> error;

	wordScanner(TCHAR* fileName,HWND hwnd)
	{
	    note=0;
		curLine=1;
		offset=0;
		tempString="";
		this->fileName=fileName;

		file.open(fileName, ios::in|ios::binary); //打开文件，后期如果有UI设计一定不是这么打开的
		{	//跳过 UTF-8 文件开头的 BOM（EF BB BF），否则会被当成三个非法字符
			char bom[3]={0,0,0};
			file.read(bom,3);
			if(file.gcount()==3&&(unsigned char)bom[0]==0xEF&&(unsigned char)bom[1]==0xBB&&(unsigned char)bom[2]==0xBF)
				offset=3;
			file.clear();
			file.seekg(0);
		}
	}

	wordScanner()
	{
	    note=0;
		curLine=1;
		offset=0;
		tempString="";

		//file.open(fileName); //打开文件，后期如果有UI设计一定不是这么打开的
	}

	bool isReservedWord(string tempString);//是否时保留字，作为isID()子程序
	int isID();//是否是标识符
	bool isChar();
	bool isInteger();
	bool isSingleBoundary();
	bool isDoubleBoundary();
	bool isNotes();
	int isArray();
	void start();
	void printResult();
	void createTokenFile();

	char getChar()//读取 offset 处的字符并后移。若前一个字符是 '\n'，说明刚跨过一个换行，行号加一
	{
		char ch;
		if(offset>=1)
		{
			file.clear();
			file.seekg(offset-1);
			if(file.get()=='\n')
				curLine++;
		}
		file.clear();      //读到文件尾后流会置 eof/fail 位，不清掉的话后面的 seekg 全部失效
		file.seekg(offset);
		ch=file.get();
		offset++;
		return ch;
	}

	void undoChar()//回退一个字符。若被回退的字符前面是 '\n'，则撤销刚才的行号加一
	{
		if(offset>=2)
		{
			file.clear();
			file.seekg(offset-2);
			if(file.get()=='\n')
				curLine--;
		}
		offset--;
	}

	bool isNumber(char ch)
	{     //判断一个字符是否为数字
		if((ch>='0')&&(ch<='9'))
			return true;
		else
			return false;
	}

	bool isLetter(char ch)
	{        //判断一个字符是否为字母
		if ((ch>='a')&&(ch<='z'))
			return true;
		if ((ch>='A')&&(ch<='Z'))
			return true;
		return false;
	}

	~wordScanner()
	{
	    int i;
	    Token* token;
/*
		for(i=0;i<tokenList.List.size();i++) //释放空间
		{
			delete(&(tokenList.List[i]));
		}
*/
		file.close();
	}
};




#endif // WORD_H_INCLUDED
