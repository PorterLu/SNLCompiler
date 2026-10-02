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

	//reserved-word array
	string reservedWord[21]={"program","procedure","type","var","if","then","else","fi",
		"while","do","endwh","begin","end","read","write","array",
		"of","record","return","integer","char"};

	ifstream file;
	HWND hwnd;
	string fileName;
	string tempString;//the lexeme just read
	int curLine;//current line
	int offset;//offset into the file
	TokenList tokenList;
	vector<string> error;

	wordScanner(TCHAR* fileName,HWND hwnd)
	{
	    note=0;
		curLine=1;
		offset=0;
		tempString="";
		this->fileName=fileName;

		file.open(fileName, ios::in|ios::binary); //open the file; with a real UI this would not be how it is opened
		{	//skip a UTF-8 BOM (EF BB BF) at the start of the file, otherwise it is read as three illegal characters
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

		//file.open(fileName); //open the file; with a real UI this would not be how it is opened
	}

	bool isReservedWord(string tempString);//whether it is a reserved word; a helper of isID()
	int isID();//whether it is an identifier
	bool isChar();
	bool isInteger();
	bool isSingleBoundary();
	bool isDoubleBoundary();
	bool isNotes();
	bool isString();   //string constant "..." (language extension, used by write)
	int isArray();
	void start();
	void printResult();
	void createTokenFile();

	char getChar()//read the character at offset and advance. If the previous character was '\n' a newline was just crossed, so increment the line number
	{
		char ch;
		if(offset>=1)
		{
			file.clear();
			file.seekg(offset-1);
			if(file.get()=='\n')
				curLine++;
		}
		file.clear();      //after end of file the stream sets its eof/fail bits; without clearing them every later seekg would fail
		file.seekg(offset);
		ch=file.get();
		offset++;
		return ch;
	}

	void undoChar()//back up one character. If the character before it is '\n', undo the line-number increment
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
	{     //test whether a character is a digit
		if((ch>='0')&&(ch<='9'))
			return true;
		else
			return false;
	}

	bool isLetter(char ch)
	{        //test whether a character is a letter
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
		for(i=0;i<tokenList.List.size();i++) //free the space
		{
			delete(&(tokenList.List[i]));
		}
*/
		file.close();
	}
};




#endif // WORD_H_INCLUDED
