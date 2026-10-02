#include"Grammar.h"
#include<iostream>
#include<cstdlib>
using namespace std;
bool wordErrorState=false;
bool grammarErrorState=false;
string unUltimateSign[67]={"Program","ProgramHead","ProgramName","DeclarePart","TypeDec","TypeDeclaration","TypeDecList","TypeDecMore","TypeId","TypeName",
                    "BaseType","StructureType","ArrayType","Low","Top","RecType","FieldDecList","FieldDecMore","IdList","IdMore",
                    "VarDec","VarDeclaration","VarDecList","VarDecMore","VarIdList","VarIdMore","ProcDec","ProcDeclaration","ProcDecMore","ProcName",
                    "ParamList","ParamDecList","ParamMore","Param","FormList","FidMore","ProcDecPart","ProcBody","ProgramBody","StmList",
                    "StmMore","Stm","AssCall","AssignmentRest","ConditionalStm","LoopStm","InputStm","Invar","OutputStm","ReturnStm",
                    "CallStmRest","ActParamList","ActParamMore","RelExp","OtherRelE","Exp","OtherTerm","Term","OtherFactor","Factor",
                    "Variable", "VariMore", "FieldVar","FieldVarMore","CmpOp","AddOp","MultOp"};

string ultimateWord[40]={"program","procedure","type","var","if","then","else","fi","while","do",
                        "endwh","begin","end","read","write","array","of","record","return","integer",
                        "char","character","intc","id",",",";","<","=","+","-",
                        "*","(",")","[","]","/",".",":=","@",".."};

string toString(int num)
{
    string str="";
    int times=0,tempNum=num;
    while(tempNum/10>0)
    {
        tempNum/=10;
        times++;
    }
    while(times>0)
    {
        int k=num/pow(10,times);
        str+=k+0x30;
        num=num-k*pow(10,times);
        times--;

    }
    str+=num+0x30;
    return str;
}

int GrammarAnalyzer::position(string str)
{
    int i;
    for(i=0;i<67;i++)
        if(str==unUltimateSign[i])
            return i;
    for(i=0;i<40;i++)
        if(str==ultimateWord[i])
            return i+67;
    return -1;
}


void GrammarAnalyzer::initProduction()
{
    production[0].left=0;
    production[0].right.push_back("ProgramHead");
    production[0].right.push_back("DeclarePart");
    production[0].right.push_back("ProgramBody");

    production[1].left=1;
    production[1].pushRight("program");
    production[1].pushRight("ProgramName");

    production[2].left=2;
    production[2].right.push_back("id");

    production[3].left=3;
    production[3].right.push_back("TypeDec");
    production[3].right.push_back("VarDec");
    production[3].right.push_back("ProcDec");

    production[4].left=4;
    production[4].right.push_back("@");

    production[5].left=4;
    production[5].right.push_back("TypeDeclaration");

    production[6].left=5;
    production[6].right.push_back("type");
    production[6].right.push_back("TypeDecList");

    production[7].left=6;
    production[7].right.push_back("TypeId");
    production[7].right.push_back("=");
    production[7].right.push_back("TypeName");
    production[7].right.push_back(";");
    production[7].right.push_back("TypeDecMore");

    production[8].left=7;
    production[8].right.push_back("@");

    production[9].left=7;
    production[9].right.push_back("TypeDecList");

    production[10].left=8;
    production[10].right.push_back("id");

    production[11].left=9;
    production[11].right.push_back("BaseType");

    production[12].left=9;
    production[12].right.push_back("StructureType");

    production[13].left=9;
    production[13].right.push_back("id");

    production[14].left=10;
    production[14].right.push_back("integer");

    production[15].left=10;
    production[15].right.push_back("char");

    production[16].left=11;
    production[16].right.push_back("ArrayType");

    production[17].left=11;
    production[17].right.push_back("RecType");

    production[18].left=12;
    production[18].right.push_back("array");
    production[18].right.push_back("[");
    production[18].right.push_back("Low");
    production[18].right.push_back("..");
    production[18].right.push_back("Top");
    production[18].right.push_back("]");
    production[18].right.push_back("of");
    production[18].right.push_back("BaseType");

    production[19].left=13;
    production[19].right.push_back("intc");

    production[20].left=14;
    production[20].right.push_back("intc");

    production[21].left=15;
    production[21].right.push_back("record");
    production[21].right.push_back("FieldDecList");
    production[21].right.push_back("end");

    production[22].left=16;
    production[22].right.push_back("BaseType");
    production[22].right.push_back("IdList");
    production[22].right.push_back(";");
    production[22].right.push_back("FieldDecMore");

    production[23].left=16;
    production[23].right.push_back("ArrayType");
    production[23].right.push_back("IdList");
    production[23].right.push_back(";");
    production[23].right.push_back("FieldDecMore");

    production[24].left=17;
    production[24].right.push_back("@");

    production[25].left=17;
    production[25].right.push_back("FieldDecList");

    production[26].left=18;
    production[26].right.push_back("id");
    production[26].right.push_back("IdMore");

    production[27].left=19;
    production[27].right.push_back("@");

    production[28].left=19;
    production[28].right.push_back(",");
    production[28].right.push_back("IdList");

    production[29].left=20;
    production[29].right.push_back("@");

    production[30].left=20;
    production[30].right.push_back("VarDeclaration");

    production[31].left=21;
    production[31].pushRight("var");
    production[31].pushRight("VarDecList");

    production[32].left=22;
    production[32].pushRight("TypeName");
    production[32].pushRight("VarIdList");
    production[32].pushRight(";");
    production[32].pushRight("VarDecMore");

    production[33].left=23;
    production[33].pushRight("@");

    production[34].left=23;
    production[34].right.push_back("VarDecList");

    production[35].left=24;
    production[35].right.push_back("id");
    production[35].right.push_back("VarIdMore");

    production[36].left=25;
    production[36].right.push_back("@");

    production[37].left=25;
    production[37].right.push_back(",");
    production[37].right.push_back("VarIdList");

    production[38].left=26;
    production[38].right.push_back("@");

    production[39].left=26;
    production[39].right.push_back("ProcDeclaration");

    production[40].left=27;
    production[40].pushRight("procedure");
    production[40].pushRight("ProcName");
    production[40].pushRight("(");
    production[40].pushRight("ParamList");
    production[40].pushRight(")");
    production[40].pushRight(";");
    production[40].pushRight("ProcDecPart");
    production[40].pushRight("ProcBody");
    production[40].pushRight("ProcDecMore");


    production[41].left=28;
    production[41].right.push_back("@");

    production[42].left=28;
    production[42].right.push_back("ProcDeclaration");

    production[43].left=29;
    production[43].right.push_back("id");

    production[44].left=30;
    production[44].right.push_back("@");

    production[45].left=30;
    production[45].right.push_back("ParamDecList");

    production[46].left=31;
    production[46].right.push_back("Param");
    production[46].right.push_back("ParamMore");

    production[47].left=32;
    production[47].right.push_back("@");

    production[48].left=32;
    production[48].right.push_back(";");
    production[48].right.push_back("ParamDecList");

    production[49].left=33;
    production[49].right.push_back("TypeName");
    production[49].right.push_back("FormList");

    production[50].left=33;
    production[50].right.push_back("var");
    production[50].right.push_back("TypeName");
    production[50].right.push_back("FormList");

    production[51].left=34;
    production[51].right.push_back("id");
    production[51].right.push_back("FidMore");

    production[52].left=35;
    production[52].right.push_back("@");

    production[53].left=35;
    production[53].right.push_back(",");
    production[53].right.push_back("FormList");

    production[54].left=36;
    production[54].right.push_back("DeclarePart");

    production[55].left=37;
    production[55].right.push_back("ProgramBody");

    production[56].left=38;
    production[56].right.push_back("begin");
    production[56].right.push_back("StmList");
    production[56].right.push_back("end");

    production[57].left=39;
    production[57].right.push_back("Stm");
    production[57].right.push_back("StmMore");

    production[58].left=40;
    production[58].right.push_back("@");

    production[59].left=40;
    production[59].right.push_back(";");
    production[59].right.push_back("StmList");

    production[60].left=41;
    production[60].right.push_back("ConditionalStm");

    production[61].left=41;
    production[61].right.push_back("LoopStm");

    production[62].left=41;
    production[62].right.push_back("InputStm");

    production[63].left=41;
    production[63].right.push_back("OutputStm");

    production[64].left=41;
    production[64].right.push_back("ReturnStm");

    production[65].left=41;
    production[65].right.push_back("id");
    production[65].right.push_back("AssCall");

    production[66].left=42;
    production[66].right.push_back("AssignmentRest");

    production[67].left=42;
    production[67].right.push_back("CallStmRest");

    production[68].left=43;
    production[68].right.push_back("VariMore");
    production[68].right.push_back(":=");
    production[68].right.push_back("Exp");

    production[69].left=44;
    production[69].right.push_back("if");
    production[69].right.push_back("RelExp");
    production[69].right.push_back("then");
    production[69].right.push_back("StmList");
    production[69].right.push_back("else");
    production[69].right.push_back("StmList");
    production[69].right.push_back("fi");

    production[70].left=45;
    production[70].right.push_back("while");
    production[70].right.push_back("RelExp");
    production[70].right.push_back("do");
    production[70].right.push_back("StmList");
    production[70].right.push_back("endwh");

    production[71].left=46;
    production[71].right.push_back("read");
    production[71].right.push_back("(");
    production[71].right.push_back("Invar");
    production[71].right.push_back(")");

    production[72].left=47;
    production[72].right.push_back("id");

    production[73].left=48;
    production[73].right.push_back("write");
    production[73].right.push_back("(");
    production[73].right.push_back("Exp");
    production[73].right.push_back(")");

    production[74].left=49;
    production[74].right.push_back("return");
    production[74].right.push_back("(");
    production[74].right.push_back("Exp");
    production[74].right.push_back(")");

    production[75].left=50;
    production[75].right.push_back("(");
    production[75].right.push_back("ActParamList");
    production[75].right.push_back(")");


    production[76].left=51;
    production[76].right.push_back("@");

    production[77].left=51;
    production[77].right.push_back("Exp");
    production[77].right.push_back("ActParamMore");

    production[78].left=52;
    production[78].right.push_back("@");

    production[79].left=52;
    production[79].right.push_back(",");
    production[79].right.push_back("ActParamList");

    production[80].left=53;
    production[80].right.push_back("Exp");
    production[80].right.push_back("OtherRelE");

    production[81].left=54;
    production[81].right.push_back("CmpOp");
    production[81].right.push_back("Exp");

    production[82].left=55;
    production[82].right.push_back("Term");
    production[82].right.push_back("OtherTerm");

    production[83].left=56;
    production[83].right.push_back("@");

    production[84].left=56;
    production[84].right.push_back("AddOp");
    production[84].right.push_back("Exp");

    production[85].left=57;
    production[85].right.push_back("Factor");
    production[85].right.push_back("OtherFactor");

    production[86].left=58;
    production[86].right.push_back("@");

    production[87].left=58;
    production[87].right.push_back("MultOp");
    production[87].right.push_back("Term");

    production[88].left=59;
    production[88].right.push_back("(");
    production[88].right.push_back("Exp");
    production[88].right.push_back(")");

    production[89].left=59;
    production[89].right.push_back("intc");

    production[90].left=59;
    production[90].right.push_back("Variable");

    production[91].left=60;
    production[91].right.push_back("id");
    production[91].right.push_back("VariMore");

    production[92].left=61;
    production[92].right.push_back("@");

    production[93].left=61;
    production[93].right.push_back("[");
    production[93].right.push_back("Exp");
    production[93].right.push_back("]");

    production[94].left=61;
    production[94].right.push_back(".");
    production[94].right.push_back("FieldVar");


    production[95].left=62;
    production[95].right.push_back("id");
    production[95].right.push_back("FieldVarMore");

    production[96].left=63;
    production[96].right.push_back("@");

    production[97].left=63;
    production[97].right.push_back("[");
    production[97].right.push_back("Exp");
    production[97].right.push_back("]");

    production[98].left=64;
    production[98].right.push_back("<");

    production[99].left=64;
    production[99].right.push_back("=");

    production[100].left=65;
    production[100].right.push_back("+");

    production[101].left=65;
    production[101].right.push_back("-");

    production[102].left=66;
    production[102].right.push_back("*");

    production[103].left=66;
    production[103].right.push_back("/");
}

void GrammarAnalyzer::initTable(){
    llTable[0][0]=0;
    llTable[1][0]=1;
    llTable[2][23]=2;

    llTable[3][2]=3;
    llTable[3][11]=3;
    llTable[3][3]=3;
    llTable[3][1]=3;

    llTable[4][3]=4;
    llTable[4][11]=4;
    llTable[4][1]=4;

    llTable[4][2]=5;
    llTable[5][2]=6;
    llTable[6][23]=7;

    llTable[7][3]=8;
    llTable[7][11]=8;
    llTable[7][1]=8;

    llTable[7][23]=9;
    llTable[8][23]=10;

    llTable[9][19]=11;
    llTable[9][20]=11;

    llTable[9][15]=12;
    llTable[9][17]=12;

    llTable[9][23]=13;
    llTable[10][19]=14;
    llTable[10][20]=15;
    llTable[11][15]=16;
    llTable[11][17]=17;
    llTable[12][15]=18;
    llTable[13][22]=19;
    llTable[14][22]=20;
    llTable[15][17]=21;

    llTable[16][19]=22;
    llTable[16][20]=22;

    llTable[16][15]=23;
    llTable[17][12]=24;

    llTable[17][19]=25;
    llTable[17][20]=25;
    llTable[17][15]=25;

    llTable[18][23]=26;
    llTable[19][25]=27;
    llTable[19][24]=28;

    llTable[20][1]=29;
    llTable[20][11]=29;

    llTable[20][3]=30;
    llTable[21][3]=31;

    llTable[22][19]=32;
    llTable[22][20]=32;
    llTable[22][23]=32;
    llTable[22][15]=32;
    llTable[22][17]=32;

    llTable[23][1]=33;
    llTable[23][11]=33;

    llTable[23][19]=34;
    llTable[23][20]=34;
    llTable[23][23]=34;
    llTable[23][15]=34;
    llTable[23][17]=34;

    llTable[24][23]=35;
    llTable[25][25]=36;
    llTable[25][24]=37;
    llTable[26][11]=38;
    llTable[26][1]=39;
    llTable[27][1]=40;
    llTable[28][11]=41;
    llTable[28][1]=42;
    llTable[29][23]=43;
    llTable[30][32]=44;

    llTable[30][15]=45;
    llTable[30][17]=45;
    llTable[30][19]=45;
    llTable[30][20]=45;
    llTable[30][23]=45;
    llTable[30][3]=45;

    llTable[31][15]=46;
    llTable[31][17]=46;
    llTable[31][19]=46;
    llTable[31][20]=46;
    llTable[31][23]=46;
    llTable[31][3]=46;

    llTable[32][32]=47;
    llTable[32][25]=48;

    llTable[33][15]=49;
    llTable[33][17]=49;
    llTable[33][19]=49;
    llTable[33][20]=49;
    llTable[33][23]=49;

    llTable[33][3]=50;
    llTable[34][23]=51;
    llTable[35][25]=52;
    llTable[35][32]=52;
    llTable[35][24]=53;

    llTable[36][2]=54;
    llTable[36][3]=54;
    llTable[36][1]=54;
    llTable[36][11]=54;

    llTable[37][11]=55;
    llTable[38][11]=56;

    llTable[39][4]=57;
    llTable[39][8]=57;
    llTable[39][13]=57;
    llTable[39][14]=57;
    llTable[39][18]=57;
    llTable[39][23]=57;

    llTable[40][12]=58;
    llTable[40][6]=58;
    llTable[40][7]=58;
    llTable[40][10]=58;

    llTable[40][25]=59;
    llTable[41][4]=60;
    llTable[41][8]=61;
    llTable[41][13]=62;
    llTable[41][14]=63;
    llTable[41][18]=64;
    llTable[41][23]=65;

    llTable[42][33]=66;
    llTable[42][36]=66;
    llTable[42][37]=66;

    llTable[42][31]=67;

    llTable[43][33]=68;
    llTable[43][36]=68;
    llTable[43][37]=68;

    llTable[44][4]=69;
    llTable[45][8]=70;
    llTable[46][13]=71;
    llTable[47][23]=72;
    llTable[48][14]=73;
    llTable[49][18]=74;
    llTable[50][31]=75;
    llTable[51][32]=76;

    llTable[51][31]=77;
    llTable[51][22]=77;
    llTable[51][23]=77;

    llTable[52][32]=78;
    llTable[52][24]=79;

    llTable[53][31]=80;
    llTable[53][22]=80;
    llTable[53][23]=80;

    llTable[54][26]=81;
    llTable[54][27]=81;

    llTable[55][31]=82;
    llTable[55][22]=82;
    llTable[55][23]=82;

    llTable[56][26]=83;
    llTable[56][27]=83;
    llTable[56][24]=83;
    llTable[56][32]=83;
    llTable[56][25]=83;
    llTable[56][34]=83;
    llTable[56][12]=83;
    llTable[56][9]=83;
    llTable[56][5]=83;
    llTable[56][6]=83;
    llTable[56][7]=83;
    llTable[56][10]=83;

    llTable[56][28]=84;
    llTable[56][29]=84;

    llTable[57][31]=85;
    llTable[57][22]=85;
    llTable[57][23]=85;

    llTable[58][28]=86;
    llTable[58][29]=86;
    llTable[58][26]=86;
    llTable[58][27]=86;
    llTable[58][24]=86;
    llTable[58][32]=86;
    llTable[58][25]=86;
    llTable[58][34]=86;
    llTable[58][12]=86;
    llTable[58][9]=86;
    llTable[58][5]=86;
    llTable[58][6]=86;
    llTable[58][7]=86;
    llTable[58][10]=86;

    llTable[58][30]=87;
    llTable[58][35]=87;

    llTable[59][31]=88;
    llTable[59][22]=89;
    llTable[59][23]=90;
    llTable[60][23]=91;

    llTable[61][37]=92;
    llTable[61][30]=92;
    llTable[61][35]=92;
    llTable[61][28]=92;
    llTable[61][29]=92;
    llTable[61][26]=92;
    llTable[61][27]=92;
    llTable[61][24]=92;
    llTable[61][32]=92;
    llTable[61][25]=92;
    llTable[61][34]=92;
    llTable[61][12]=92;
    llTable[61][9]=92;
    llTable[61][5]=92;
    llTable[61][6]=92;
    llTable[61][7]=92;
    llTable[61][10]=92;

    llTable[61][33]=93;
    llTable[61][36]=94;
    llTable[62][23]=95;

    llTable[63][37]=96;
    llTable[63][30]=96;
    llTable[63][35]=96;
    llTable[63][28]=96;
    llTable[63][29]=96;
    llTable[63][26]=96;
    llTable[63][27]=96;
    llTable[63][24]=96;
    llTable[63][32]=96;
    llTable[63][25]=96;
    llTable[63][34]=96;
    llTable[63][12]=96;
    llTable[63][9]=96;
    llTable[63][5]=96;
    llTable[63][6]=96;
    llTable[63][7]=96;
    llTable[63][10]=96;

    llTable[63][33]=97;
    llTable[64][26]=98;
    llTable[64][27]=99;
    llTable[65][28]=100;
    llTable[65][29]=101;
    llTable[66][30]=102;
    llTable[66][35]=103;
}



void GrammarAnalyzer::readToken() //读取token，并做一定的处理 ，方便语法分析
{
    FILE* file;
    string str;
    str=fileName.substr(0, fileName.length() - 3)+"token";
    file=fopen(str.c_str(),"r");

    char tempString1[100],tempString2[100];
    string str1,str2;
    Token* tempToken;

    while(!feof(file))
    {
        int i,line;
        fscanf(file,"%s %s %d\n",tempString1,tempString2,&line);
        str1=tempString1;
        str2=tempString2;
        if(str2=="reservedWord")
            str2=str1;
        else if(str2=="integer")
            str2="intc";
        else if(str2=="char")
            str2="character";
        else if(str2=="singleBoundary"||str2=="doubleBoundary"||str2=="arrayBound")
            str2=str1;
        tempToken = new Token(str1,str2,line);
        tokenList.push(*tempToken);
    }
    fclose(file);
}
void GrammarAnalyzer::initUUsign()
{
    int i;
    for(i=0;i<107;i++)
    {
        if(i<=66)
            UUsignArray[i].left=unUltimateSign[i];
        else
        {
            UUsignArray[i].left=ultimateWord[i-67];
            UUsignArray[i].isUU=false;
        }

    }
}


void GrammarAnalyzer::start()
{
    cout<<endl<<"------------------Grammar Analyze----------------"<<endl<<endl;
    string str;
    int depth=0;
    Item* tempOperation;
    Node* tempRoot=new Node("Program");
    nodeList.push_back(tempRoot);
    Token token=tokenList.get();
    UUsign topItem,tempItem;
    int i;
    Production tempProduction;
    analyzeStack.push(UUsignArray[0]);
    cout<<"grammar start"<<endl;

    while(!analyzeStack.empty()||(tokenList.pos!=tokenList.num-1))
    {
           tempOperation = new Item();
           topItem=analyzeStack.top();
           int tempNum=position(topItem.left);
           if(tempNum<=66)
           {
               analyzeStack.pop();
               if(llTable[tempNum][position(token.type)-67]<0)
               {
                    error();
                    grammarErrorState=true;
                    break;
               }
               tempOperation->left=topItem.left;
               tempOperation->oper="替换";
               tempProduction=production[llTable[tempNum][position(token.type)-67]];
               str="";
               for(i=tempProduction.right.size()-1;i>=0;i--)
               {
                   str+=tempProduction.right[tempProduction.right.size()-i-1];
                   str+=" ";
                   analyzeStack.push(UUsignArray[position(tempProduction.right[i])]);
               }
               tempOperation->right=str;
               if(str=="@ ")
                tempOperation->right="ε";
               itemList.push_back(*tempOperation);
               for(i=0;i<tempProduction.right.size();i++){          //  加入子节点
                    Node* tempSon=new Node(UUsignArray[position(tempProduction.right[i])].left);
                    if(UUsignArray[position(tempProduction.right[i])].left=="@")
                        tempSon->name="ε";
                    tempSon->father=tempRoot;
                    tempRoot->son.push_back(tempSon);
                    nodeList.push_back(tempSon);
               }
               tempRoot=tempRoot->getSon();      //  转向子节点
               //cout<<tempRoot->name<<endl;
               depth++;
               if(depth>maxDepth)
                    maxDepth=depth;
               //cout<<endl;
           }
           else if(tempNum>66&&topItem.left==token.type)
           {
               analyzeStack.pop();
               tempOperation->left=token.name;
               tempOperation->oper="匹配";
               itemList.push_back(*tempOperation);
               if(tokenList.pos<tokenList.num)      //注意取到尽头的时候不允许再取，否则无法执行后续程序
                    token=tokenList.get();
               tempRoot=tempRoot->father;       //回到父节点
               depth--;
               while((tempRoot->curSon>tempRoot->son.size()-1)&&(tempRoot->father!=NULL)){
                    tempRoot->curSon=0;
                    tempRoot=tempRoot->father;
                    depth--;
               }
               if((tempRoot->curSon>tempRoot->son.size()-1)&&(tempRoot->father==NULL)){
                    //cout<<"结束-----------------------"<<endl;
                    tempRoot->curSon=0;
                    this->root=tempRoot;
                    grammarErrorState=false;
                    break;//树结束
               }
               else{
                    tempRoot=tempRoot->getSon(); //找到栈顶节点
                    depth++;
               }
           }
           else if(tempNum==105)
           {
               tempOperation->left="ε";
               tempOperation->oper="匹配";
               itemList.push_back(*tempOperation);
               analyzeStack.pop();
               tempRoot=tempRoot->father;       //回到父节点
               depth--;
               while((tempRoot->curSon>tempRoot->son.size()-1)&&(tempRoot->father!=NULL)){
                    tempRoot->curSon=0;
                    tempRoot=tempRoot->father;
                    depth--;
               }
               if((tempRoot->curSon>tempRoot->son.size()-1)&&(tempRoot->father==NULL)){
                    tempRoot->curSon=0;
                    this->root=tempRoot;
                    grammarErrorState=true;
                    break;//树结束
               }
               else{
                    tempRoot=tempRoot->getSon(); //找到栈顶节点
               }
           }
           else
           {
               error();
               grammarErrorState=true;
               break;
           }

    }
    cout<<"grammar finish"<<endl;
    if(!(analyzeStack.empty()&&tokenList.pos!=tokenList.num-1)&&!grammarErrorState)
            error();
}

void GrammarAnalyzer::error()
{
    Item* tempOperation;
    tokenList.unget();
    Token tempToken=tokenList.get();
    tempOperation= new Item();
    tempOperation->left= tempToken.name;
    tempOperation->oper="error";
    tempOperation->right=toString(tempToken.line);
    itemList.push_back(*tempOperation);
}

void GrammarAnalyzer::printTree(Node* root){
    Node* temp=root;
    int n=0;
    cout<<temp->name<<endl;
    while(temp->curSon<=temp->son.size()-1){
        temp=temp->getSon();
        n++;
        if(temp->son.size()!=0){
            for(int i=0;i<n;i++){
                cout<<"\t";
            }
            cout<<"|";
            cout<<temp->name<<endl;
        }
        else{
            for(int i=0;i<n;i++){
                cout<<"\t";
            }
            cout<<"|";
            cout<<temp->name<<endl;
            while(temp->son.size()==0){
                temp=temp->father;
                n--;
                while(temp->father!=NULL&&temp->curSon>temp->son.size()-1){ //到根节点就停，否则解引用空指针
                    temp=temp->father;
                    n--;
                }
            }
        }
    }
    for(int i=0;i<nodeList.size();i++) //遍历改动了 curSon，复位以便再次遍历
        nodeList[i]->curSon=0;
}
