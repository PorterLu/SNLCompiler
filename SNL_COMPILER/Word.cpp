#include "Word.h"
#include<iostream>
#include<stdlib.h>
#include<stdio.h>
#include<sstream>
using namespace std;

static string lineStr(int line) //line number to string (the original curLine+0x30 could only represent 0-9)
{
	stringstream ss;
	ss<<line;
	return ss.str();
}
bool wordScanner::isChar()
{
	Token* token;
	char ch;
	bool isError=false; //whether this scan failed. The original code branched on the global wordErrorState, so once any error had occurred every later valid char constant was misjudged
	ch=getChar();
    while(ch=='\n'||ch=='\r'||ch=='\t'||ch==' ')
        ch=getChar();
	if(ch!='\'')
    {
        undoChar();
		return false;
    }
	ch=getChar();          //read the first character after the opening quote
    if(isNumber(ch)||isLetter(ch))
	{
        tempString+=ch;
        ch=getChar();             //read the first character after the letter or digit
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
			undoChar();         //  back up twice to handle forms like '12
			undoChar();
			isError=true;
		}
    }

	string str="line "+lineStr(curLine)+": bad token: "+tempString;
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
    undoChar();     //back the file character pointer up by one
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
        tempString+=ch;     //double delimiter
		token=new Token(tempString,"doubleBoundary",curLine);
		tokenList.push(*token);
		//cout<<tempString<<endl;
		tempString="";
        return true;
    }
    else
	{
        undoChar();
        str="line "+lineStr(curLine)+": bad token: "+tempString;
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
	     str="line ";
	     str+=lineStr(curLine);
	     str+=": bad token: ";
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

bool wordScanner::isString()
{
	Token* token;
	char ch=getChar();
    while(ch=='\n'||ch=='\r'||ch=='\t'||ch==' ')
        ch=getChar();
	if(ch!='"')
	{
		undoChar();
		return false;
	}
	//string constant "...": no escapes, cannot span lines
	int line=curLine;
	ch=getChar();
	while(!file.eof()&&ch!='"'&&ch!='\n')
	{
		tempString+=ch;
		ch=getChar();
	}
	if(ch!='"')
	{
		error.push_back("line "+lineStr(line)+": bad token: \""+tempString);
		wordErrorState=true;
		tempString="";
		return false;
	}
	token=new Token(tempString,"string",line);
	tokenList.push(*token);
	tempString="";
	return true;
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
    if(ch=='.'){     //array delimiter
        tempString+=ch;
		token=new Token(tempString,"arrayBound",curLine);
		tokenList.push(*token);
		tempString="";
        return true;
    }
    else{
        undoChar();     //back the file character pointer up by one
        //a lone dot: it ends the program only when nothing but whitespace follows up to end of file; otherwise it is the "." of a record field access like rec.x (the original always treated it as end of program, so records were unusable)
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

/*there are 20 reserved words, defined in header.h; scan the reserved-word array
and compare with the current identifier; the argument is an identifier that might be a reserved word*/
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

/*decides whether this is an identifier: 0 = not an identifier, 1 = identifier, 2 = reserved word (a reserved word is treated as a special identifier)*/
int wordScanner::isID()
{
	char curChar;  //current character
	Token* token;
	bool isOver=false,isID=false; //isOver tells whether to leave the automaton, isID tells whether it is an identifier
	int transTable[2][2]={{-1,1}, {1,1}};//transition table of the automaton; -1 means entering the error state
	int state=0; //initial state of the automaton
	curChar = getChar(); //get a character
	//first skip whitespace: spaces, tabs and newlines
    while(curChar=='\n'||curChar=='\r'||curChar=='\t'||curChar==' ')
        curChar=getChar();
	//enter the identifier automaton: a letter followed by letters and digits
	while(((curChar>=48&&curChar<=57)||(curChar>=65&&curChar<=90)||(curChar>=97&&curChar<=122))&&!isOver)//check whether the character is in the identifier character set
	{
		switch(state)
		{
			case 0:
				if(curChar>=48&&curChar<=57)  //state 0 is the initial state; a digit here means a leading digit, which is not an identifier, so leave the automaton
					isOver=true;
				else
				{
					state=transTable[state][1]; //transition
                    tempString+=curChar; //build the string
					curChar=getChar(); //move to the next character
					isID=true;		//an identifier; state 1 is an accepting state
				}
				break;
			case 1:
				if(curChar>=48&&curChar<=57) //digit
				{
					state=transTable[state][0];
                    tempString+=curChar;
					curChar=getChar();
					isID=true;
				}
				else  //letter
				{
					state=transTable[state][1];
                    tempString+=curChar;
					curChar=getChar();
					isID=true;
				}
		}
	}
	undoChar(); //back up one character, since the last one is not part of the identifier
	if(isID&&isReservedWord(tempString))//is it a reserved word
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
		cout<<token->name<<" "<<token->type<<" line "<<token->line<<""<<endl;
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
	int ans; //result flag
	char ch;
	cout<<"----------------------lexical analysis---------------------"<<endl;
	while(!file.eof())//file not finished
	{
		size_t errBefore=error.size();

		if(ans=isID()){}
		else if(ans=isChar()){}
		else if(ans=isInteger()){}
		else if(ans=isSingleBoundary()){}
		else if(ans=isDoubleBoundary()){}
		else if(ans=isNotes()){}
		else if(ans=isString()){}
		else if(ans=isArray()){if(ans==2) break;}
		else
		{
			 if(error.size()>errBefore) continue; //some sub-scanner already reported an error and backed up; restart scanning from isID
			 ch=getChar();
			 if(file.eof()) break; //reached end of file (the source did not end with '.'), terminate normally
			 string str;
             str="unknown token ";
             str+=ch;
             str+=" line ";
             str+=lineStr(curLine);
             str+="";
             tempString="";
             error.push_back(str);
             wordErrorState=true;
		}
	}

	wordErrorState=!error.empty(); //set the error state whenever any error was recorded (the original cleared existing errors at end of file)

	if(wordErrorState==false)
    {
        createTokenFile();
        printResult();
    }

    cout<<"word finish"<<endl;
	//system("pause");
}
