#include "header.h"    //全局头文件
#include "Grammar.h"    //语法分析头文件
#include "Word.h"       //词法分析头文件
#include "resource.h"   //资源
#include "Gen.h"        //语义分析与代码生成头文件
#include "Vm.h"         //中间代码虚拟机头文件
using namespace std;

#define ID_EDIT     1
#define BLOCK_HIGH 20
#define BLOCK_WIDTH 100
#define LINE 30
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);  //主窗体回调函数
wordScanner* wordProject;                               //词法分析项目
GrammarAnalyzer* grammarProject;                        //语法分析项目
static TCHAR szAppName[] = TEXT ("snl") ;               //程序名
int maxWidth=0;
HINSTANCE hInst;                                        //程序句柄
OPENFILENAME ofn ;                                      //文件信息结构
HWND hwndEdit ;                                         //子窗口句柄
HWND hDlgChild = NULL ;                                  //子窗口句柄
BOOL CALLBACK WordDlg (HWND, UINT, WPARAM, LPARAM);     //词法分析子窗口回调函数
BOOL CALLBACK GrammarDlg(HWND, UINT, WPARAM, LPARAM);   //语法分析子窗口回调函数
BOOL CALLBACK TreeDlg(HWND, UINT, WPARAM, LPARAM);   //语法树子窗口回调函数
BOOL CALLBACK BuildDlg(HWND, UINT, WPARAM, LPARAM);  //一键编译结果窗口回调函数
static string buildText;                                //一键编译结果窗口显示的文本

//把 \n 换成 \r\n（编辑框和多行文本框只认 \r\n）
static string CrLf (const string& s)
{
     string r ;
     for (size_t i = 0 ; i < s.size () ; i++) { if (s[i] == '\n') r += "\r\n" ; else r += s[i] ; }
     return r ;
}
//程序输入对话框：虚拟机执行到 read 时弹出
static string inputText ;
BOOL CALLBACK InputDlg (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
     char buf[256] ;
     switch (message)
     {
     case WM_INITDIALOG:
          SetDlgItemText (hwnd, IDC_INPUT_PROMPT, (LPCSTR) lParam) ;
          SetFocus (GetDlgItem (hwnd, IDC_INPUT_EDIT)) ;
          return FALSE ;
     case WM_COMMAND:
          if (LOWORD (wParam) == IDOK)
          {
               GetDlgItemText (hwnd, IDC_INPUT_EDIT, buf, sizeof buf) ;
               inputText = buf ;
               EndDialog (hwnd, IDOK) ;
               return TRUE ;
          }
          if (LOWORD (wParam) == IDCANCEL) { EndDialog (hwnd, IDCANCEL) ; return TRUE ; }
          break ;
     case WM_CLOSE:
          EndDialog (hwnd, IDCANCEL) ;
          return TRUE ;
     }
     return FALSE ;
}
//虚拟机的输入输出：read 弹对话框要输入，write 的内容累积到字符串里
struct DialogIO : VmIO
{
     HWND owner ;
     string out ;
     DialogIO (HWND h) : owner (h) {}
     bool ask (const char* prompt)
     {
          return DialogBoxParam (hInst, MAKEINTRESOURCE (IDC_INPUT_DIALOG), owner, (DLGPROC) InputDlg, (LPARAM) prompt) == IDOK ;
     }
     bool readInt (int& v) { if (!ask ("程序执行到 read，请输入一个整数：")) return false ; v = atoi (inputText.c_str ()) ; return true ; }
     bool readChar (char& c) { if (!ask ("程序执行到 read，请输入一个字符：")) return false ; c = inputText.empty () ? ' ' : inputText[0] ; return true ; }
     void write (const string& s) { out += s ; }
} ;
int vScroll=0;
int constMaxWidth;
INT hScroll=0;
void dfs(HWND hwnd,Node* node,int depth,int width);
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
{
     HACCEL   hAccel ;      //加速键资源句柄
     HWND     hwnd ;        //主窗口句柄
     MSG      msg ;         //消息
     WNDCLASS wndclass ;    //窗口类
	 HMENU    hMenu;        //菜单句柄
	 hInst=hInstance;       //程序句柄赋值
     wndclass.style         = CS_HREDRAW | CS_VREDRAW ; //设定窗口风格
     wndclass.lpfnWndProc   = WndProc ;  //设定回调函数
     wndclass.cbClsExtra    = 0 ;
     wndclass.cbWndExtra    = 0 ;
     wndclass.hInstance     = hInstance ;  //程序句柄
     wndclass.hIcon         = (HICON)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)) ; //加载图标
     wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW) ; //加载光标
     wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH) ; //获取刷子
     wndclass.lpszMenuName  = szAppName ;  //菜单名
     wndclass.lpszClassName = szAppName ;  //窗口类名

     if (!RegisterClass (&wndclass))  //窗口类是否注册成功
     {
          MessageBox (NULL, TEXT ("This program requires Windows NT!"),
                      szAppName, MB_ICONERROR) ;
          return 0 ;
     }
     hMenu = LoadMenu(hInstance,MAKEINTRESOURCE(IDR_MENU1)) ; //加载菜单
     hwnd = CreateWindow (szAppName, szAppName,
                          WS_OVERLAPPEDWINDOW,
                          GetSystemMetrics (SM_CXSCREEN) / 4,
                          GetSystemMetrics (SM_CYSCREEN) / 4,
                          GetSystemMetrics (SM_CXSCREEN) / 2,
                          GetSystemMetrics (SM_CYSCREEN) / 2,
                          NULL, hMenu, hInstance, NULL) ;  //创建窗口

     ShowWindow (hwnd, iCmdShow) ; //设置显示状态
     UpdateWindow (hwnd) ;         //绘制窗口

     hAccel = LoadAccelerators (hInstance, szAppName) ;  //加速键资源加载

     while (GetMessage (&msg, NULL, 0, 0))  //获取消息
     {
         /*第一个判断,如果词法分析的窗口未打开，语法分析和语法树的窗口也为打开
           第二个判断,检验是否有一个子窗口打开，并且是它的消息*/

          if (hDlgChild == 0||!IsDialogMessage(hDlgChild, &msg))
		 {
             if (!TranslateAccelerator (hwnd, hAccel, &msg)) //翻译加速键位为可识别的消息
			 {
                TranslateMessage (&msg) ;
                DispatchMessage (&msg) ;
			 }
		 }
     }
     return msg.wParam ;
}

int toNum(string str)  //string类型转int型
{
    int s=str.size();  //获取string的长度
    int i,num=0;
    for(i=0;i<s;i++)
    {
        num=num*10+(str[s-i-1]-'0'); //先得到低的一位，后乘10,形成新的低位
    }
    return num;
}

bool isNum(string str)  //判断一个string是否可以转为数字
{
    int i,s;
    s=str.size();
    for(i=0;i<s;i++)
    {
        if(i>'9'|i<'0')
            return false;
    }
    return true;
}
//生成询问窗口
AskConfirmation (HWND hwnd)
{
     return MessageBox (hwnd, TEXT ("是否想要推出编译器?"),
                        szAppName, MB_YESNO | MB_ICONQUESTION) ;
}

//更改程序的标题
void DoCaption (HWND hwnd, TCHAR * szTitleName)
{
     TCHAR szCaption[64 + MAX_PATH] ;

     wsprintf (szCaption, TEXT ("%s - %s"), szAppName,
               szTitleName[0] ? szTitleName : UNTITLED) ;

     SetWindowText (hwnd, szCaption) ;
}

//文件信息结构的初始化
void PopFileInitialize (HWND hwnd)
{
     static TCHAR szFilter[] = TEXT ("Text Files (*.TXT)\0*.txt\0")  \
                               TEXT ("ASCII Files (*.ASC)\0*.asc\0") \
                               TEXT ("All Files (*.*)\0*.*\0\0") ;

     ofn.lStructSize       = sizeof (OPENFILENAME) ;
     ofn.hwndOwner         = hwnd ;
     ofn.hInstance         = NULL ;
     ofn.lpstrFilter       = szFilter ;
     ofn.lpstrCustomFilter = NULL ;
     ofn.nMaxCustFilter    = 0 ;
     ofn.nFilterIndex      = 0 ;
     ofn.lpstrFile         = NULL ;
     ofn.nMaxFile          = MAX_PATH ;
     ofn.lpstrFileTitle    = NULL ;
     ofn.nMaxFileTitle     = MAX_PATH ;
     ofn.lpstrInitialDir   = NULL ;
     ofn.lpstrTitle        = NULL ;
     ofn.Flags             = 0 ;
     ofn.nFileOffset       = 0 ;
     ofn.nFileExtension    = 0 ;
     ofn.lpstrDefExt       = TEXT ("txt") ;
     ofn.lCustData         = 0L ;
     ofn.lpfnHook          = NULL ;
     ofn.lpTemplateName    = NULL ;
}

//打开一个文件
BOOL PopFileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName)
{
     ofn.hwndOwner         = hwnd ;
     ofn.lpstrFile         = pstrFileName ;
     ofn.lpstrFileTitle    = pstrTitleName ;
     ofn.Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT ;

     return GetOpenFileName(&ofn) ;
}

//ok窗口的函数
void OkMessage (HWND hwnd, TCHAR * szMessage, TCHAR * szTitleName)
{
     TCHAR szBuffer[64 + MAX_PATH] ;

     wsprintf (szBuffer, szMessage, szTitleName[0] ? szTitleName : UNTITLED) ;

     MessageBox (hwnd, szBuffer, szAppName, MB_OK | MB_ICONEXCLAMATION) ;
}

//把单独的 \n 补成 \r\n，否则 Mac / Linux 上写的文件在编辑框里不分行
static unsigned char* ExpandNewlines (unsigned char* text)
{
     int n = 0, i, j ;
     for (i = 0 ; text[i] ; i++) if (text[i] == '\n' && (i == 0 || text[i-1] != '\r')) n++ ;
     if (n == 0) return text ;
     unsigned char* out = (unsigned char*) malloc (i + n + 1) ;
     for (i = 0, j = 0 ; text[i] ; i++)
     {
          if (text[i] == '\n' && (i == 0 || text[i-1] != '\r')) out[j++] = '\r' ;
          out[j++] = text[i] ;
     }
     out[j] = '\0' ;
     free (text) ;
     return out ;
}
//读取一个文件并显示
BOOL PopFileRead (HWND hwndEdit, PTSTR pstrFileName)
{
     BYTE   bySwap ;
     DWORD  dwBytesRead ;
     HANDLE hFile ;

     int    i, iFileLength, iUniTest ;
     PBYTE  pBuffer, pText, pConv ;


     if (INVALID_HANDLE_VALUE ==
               (hFile = CreateFile (pstrFileName, GENERIC_READ, FILE_SHARE_READ,
                                    NULL, OPEN_EXISTING, 0, NULL)))
          return FALSE ;

     iFileLength = GetFileSize (hFile, NULL) ;
     pBuffer =(unsigned char*) malloc (iFileLength + 2) ;

     ReadFile (hFile, pBuffer, iFileLength, &dwBytesRead, NULL) ;
     CloseHandle (hFile) ;
     pBuffer[iFileLength] = '\0' ;
     pBuffer[iFileLength + 1] = '\0' ;


     iUniTest = IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_REVERSE_SIGNATURE ;

     if (IsTextUnicode (pBuffer, iFileLength, &iUniTest))
     {
          pText = pBuffer + 2 ;
          iFileLength -= 2 ;

          if (iUniTest & IS_TEXT_UNICODE_REVERSE_SIGNATURE)
          {
               for (i = 0 ; i < iFileLength / 2 ; i++)
               {
                    bySwap = ((BYTE *) pText) [2 * i] ;
                    ((BYTE *) pText) [2 * i] = ((BYTE *) pText) [2 * i + 1] ;
                    ((BYTE *) pText) [2 * i + 1] = bySwap ;
               }
          }

          pConv =(unsigned char*) malloc (iFileLength + 2) ;

#ifndef UNICODE
          WideCharToMultiByte (CP_ACP, 0, (PWSTR) pText, -1,(char*) pConv,
                               iFileLength + 2, NULL, NULL) ;

#else
          lstrcpy ((PTSTR) pConv, (PTSTR) pText) ;
#endif

     }
     else
     {
          pText = pBuffer ;
          //跳过 UTF-8 BOM
          if (iFileLength >= 3 && pText[0] == 0xEF && pText[1] == 0xBB && pText[2] == 0xBF)
          {
               pText += 3 ;
               iFileLength -= 3 ;
          }

          pConv =(unsigned char*) malloc (2 * iFileLength + 2) ;

#ifdef UNICODE
          MultiByteToWideChar (CP_ACP, 0, pText, -1, (PTSTR) pConv,
                               iFileLength + 1) ;

#else
          //源文件若是 UTF-8 编码（例如在 Mac / Linux 上写的），先转成当前代码页，编辑框里的中文才不会乱码
          BOOL bHigh = FALSE ;
          for (i = 0 ; i < iFileLength ; i++) if (pText[i] >= 0x80) { bHigh = TRUE ; break ; }
          int wlen = bHigh ? MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR) pText, iFileLength, NULL, 0) : 0 ;
          if (wlen > 0)
          {
               PWSTR pWide = (PWSTR) malloc ((wlen + 1) * sizeof (WCHAR)) ;
               MultiByteToWideChar (CP_UTF8, 0, (LPCSTR) pText, iFileLength, pWide, wlen) ;
               int alen = WideCharToMultiByte (CP_ACP, 0, pWide, wlen, NULL, 0, NULL, NULL) ;
               free (pConv) ;
               pConv = (unsigned char*) malloc (alen + 1) ;
               WideCharToMultiByte (CP_ACP, 0, pWide, wlen, (LPSTR) pConv, alen, NULL, NULL) ;
               pConv[alen] = '\0' ;
               free (pWide) ;
          }
          else
          {
               memcpy (pConv, pText, iFileLength) ;
               pConv[iFileLength] = '\0' ;
          }
          pConv = ExpandNewlines (pConv) ;
#endif
     }

     SetWindowText (hwndEdit, (PTSTR) pConv) ;
     free (pBuffer) ;
     free (pConv) ;

     return TRUE ;
}

BOOL PopFileWrite (HWND hwndEdit, PTSTR pstrFileName)
{
     DWORD  dwBytesWritten ;
     HANDLE hFile ;
     int    iLength ;
     PTSTR  pstrBuffer ;
     WORD   wByteOrderMark = 0xFEFF ;

     if (INVALID_HANDLE_VALUE ==
               (hFile = CreateFile (pstrFileName, GENERIC_WRITE, 0,
                                    NULL, CREATE_ALWAYS, 0, NULL)))
          return FALSE ;

     iLength = GetWindowTextLength (hwndEdit) ;
     pstrBuffer = (PTSTR) malloc ((iLength + 1) * sizeof (TCHAR)) ;

     if (!pstrBuffer)
     {
          CloseHandle (hFile) ;
          return FALSE ;
     }


#ifdef UNICODE
     WriteFile (hFile, &wByteOrderMark, 2, &dwBytesWritten, NULL) ;
#endif

     GetWindowText (hwndEdit, pstrBuffer, iLength + 1) ;
     WriteFile (hFile, pstrBuffer, iLength * sizeof (TCHAR),
                &dwBytesWritten, NULL) ;

     if ((iLength * sizeof (TCHAR)) != (int) dwBytesWritten)
     {
          CloseHandle (hFile) ;
          free (pstrBuffer) ;
          return FALSE ;
     }

     CloseHandle (hFile) ;
     free (pstrBuffer) ;

     return TRUE ;
}

void DrawBorder(HDC hdc, int x, int y, int Width, int Height, int BorderWidth, int WTop)
{
	//WTop为TRUE时Top选白笔，bottom为黑笔

	int i;
	HPEN  hpen;
	hpen=CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	if (WTop)
		SelectObject(hdc,GetStockObject(WHITE_PEN));
	else
		SelectObject(hdc,hpen);
	//画top,left边
	for (i=0; i<BorderWidth; i++)
	{
		MoveToEx(hdc,x+i, y+i,NULL);
		LineTo(hdc,x+Width-i, y+i);
		MoveToEx(hdc,x+i, y+i,NULL);
		LineTo(hdc,x+i, y+Height-i);
	}

	if (WTop)
		SelectObject(hdc,hpen);
	else
		SelectObject(hdc,GetStockObject(WHITE_PEN));

	//画bottom,right边
	for (i=0; i<BorderWidth; i++)
	{
		MoveToEx(hdc,x+Width-i, y+Height-i,NULL);
		LineTo(hdc,x+Width-i, y+i);
		MoveToEx(hdc,x+Width-i, y+Height-i,NULL);
		LineTo(hdc,x+i, y+Height-i);
	}

	DeleteObject(hpen);
}
//主窗口回调函数
LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

     int         iSelect, iEnable ;
     static TCHAR     szFileName[MAX_PATH], szTitleName[MAX_PATH] ;
     static string fileName=""; //文件名记录
     switch (message)
     {
     case WM_CREATE:
          hwndEdit = CreateWindow (TEXT ("edit"), NULL,
                              WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
                              WS_BORDER | ES_LEFT | ES_MULTILINE |
                              ES_AUTOHSCROLL | ES_AUTOVSCROLL,
                              0, 0, 0, 0, hwnd, (HMENU) ID_EDIT,
                              ((LPCREATESTRUCT) lParam)->hInstance, NULL) ; //创建输入用的子窗口
		  PopFileInitialize (hwnd) ;  //文件信息结构的初始化
          return 0 ;

     case WM_SETFOCUS:   //当光标关注这个窗口
          SetFocus (hwndEdit) ;
          return 0 ;

     case WM_SIZE:  //当改变这个窗口的大小
          MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE) ;
          return 0 ;

     case WM_INITMENUPOPUP:  //菜单栏的状态转换
          if (lParam == 1)
          {
			   if (!hDlgChild)
               {
				   EnableMenuItem ((HMENU) wParam,IDC_WORD,   MF_ENABLED) ;
				   EnableMenuItem ((HMENU) wParam,IDC_GRAMMAR,   MF_ENABLED) ;
				   EnableMenuItem ((HMENU) wParam,IDC_TREE,   MF_ENABLED) ;
				   EnableMenuItem ((HMENU) wParam,IDC_BUILD,  MF_ENABLED) ;
               }
               else
               {
                    EnableMenuItem ((HMENU) wParam,IDC_WORD,  MF_GRAYED) ;
                    EnableMenuItem ((HMENU) wParam,IDC_GRAMMAR,  MF_GRAYED) ;
                    EnableMenuItem ((HMENU) wParam,IDC_TREE,  MF_GRAYED) ;
                    EnableMenuItem ((HMENU) wParam,IDC_BUILD, MF_GRAYED) ;
               }
               return 0 ;
          }
          break ;

	 case WM_COMMAND: //收到的菜单的消息
		 switch(LOWORD(wParam))
          {
			 case IDC_OPEN:

                //尝试打开文件
                if (PopFileOpenDlg (hwnd, szFileName, szTitleName))
                {
                    if (!PopFileRead (hwndEdit, szFileName))
                    {
                         OkMessage (hwnd, TEXT ("无法打开文件%s!"),
                                    szTitleName) ;
                         szFileName[0]  = '\0' ;
                         szTitleName[0] = '\0' ;
                    }
                }
                //设置标题
                DoCaption (hwnd, szTitleName) ;
                return 0 ;

             case IDC_WORD:
                //选择词法分析
                if(szFileName[0]=='\0')
                {
                    MessageBox(hwnd,"请打开文件","提醒",MB_OK);
                    return 0;
                }
                fileName=szFileName;
                //词法分析开始i
                wordProject = new wordScanner(szFileName,hwnd);
                //wordProject = new wordScanner(szFileName,hwnd);
                wordProject->start();
                //生成词法分析窗口
                hDlgChild = CreateDialog(hInst,MAKEINTRESOURCE(IDC_WORD_DIALOG),hwnd, WordDlg);
                ShowWindow(hDlgChild,SW_SHOW);

                return 0;

             case IDC_GRAMMAR:
                 //词法分析是否实现错误
                if(wordErrorState==true)
                {
                    MessageBox(hwnd,"词法错误","提醒",MB_OK);
                    return 0;
                }
                //是否已经进行词法分析
                if(fileName=="")
                {
                    MessageBox(hwnd,"先进行词法分析","提醒",MB_OK);
                    return 0;
                }
                //开始语法分析
                grammarProject = new GrammarAnalyzer(fileName.c_str(),hwnd);
                //grammarProject = new GrammarAnalyzer(fileName.c_str(),hwnd);
                grammarProject->start();

                //创建语法分析窗口


                hDlgChild = CreateDialog(hInst,MAKEINTRESOURCE(IDC_GRAMMAR_DIALOG),hwnd,GrammarDlg);
                ShowWindow(hDlgChild,SW_SHOW);
                return 0;
             case IDC_AUTHOR:
                 //显示开发人员信息
                MessageBox(hwnd," 21172602 卢琨\n 21172603 刘璎慧\n 21172617 张智超","开发人员",MB_OK);
                    return 0;
             case IDC_SAVE:
               if (szFileName[0])
               {
                    if (PopFileWrite (hwndEdit, szFileName)&&MessageBox(hwnd,"确定要保存吗",szFileName,MB_YESNO)==IDYES)
                         return 1 ;
                    else
                    {
                         OkMessage (hwnd, TEXT ("不可写文件 %s"),
                                    szTitleName) ;
                         return 0 ;
                    }
               }
               else
               {
                   MessageBox(hwnd,"还未打开文件",NULL,MB_OK);
               }
               return 0;

             case IDC_TREE:
                 if(wordErrorState==true)
                {
                    MessageBox(hwnd,"词法错误","提醒",MB_OK);
                    return 0;
                }
                //是否已经进行词法分析
                if(fileName=="")
                {
                    MessageBox(hwnd,"先进行词法分析","提醒",MB_OK);
                    return 0;
                }
                //开始语法分析
                grammarProject = new GrammarAnalyzer(fileName.c_str(),hwnd);
                //grammarProject = new GrammarAnalyzer(fileName.c_str(),hwnd);
                grammarProject->start();
                if(grammarErrorState==true)
                {
                    MessageBox(hwnd,"语法分析有错","提醒",MB_OK);
                    return 0;
                }
                hDlgChild = CreateDialog(hInst,MAKEINTRESOURCE(IDC_TREE_DIALOG),hwnd,TreeDlg);
                ShowWindow(hDlgChild,SW_SHOW);
                return 0;
             case IDC_BUILD:
                //一键编译：词法分析 -> 语法分析 -> 语义分析并生成中间代码 -> 在虚拟机上直接运行
                if(szFileName[0]=='\0')
                {
                    MessageBox(hwnd,"请打开文件","提醒",MB_OK);
                    return 0;
                }
                {
                    string report;
                    wordScanner* ws=new wordScanner(szFileName,hwnd);
                    ws->start();
                    if(wordErrorState)
                    {
                        report="词法错误：\r\n";
                        for(unsigned i=0;i<ws->error.size();i++) report+=ws->error[i]+"\r\n";
                        delete ws;
                    }
                    else
                    {
                        delete ws;
                        fileName=szFileName;
                        GrammarAnalyzer* ga=new GrammarAnalyzer(fileName,hwnd);
                        ga->start();
                        if(grammarErrorState)
                        {
                            report="语法错误：\r\n";
                            for(unsigned i=0;i<ga->itemList.size();i++)
                                if(ga->itemList[i].oper=="error")
                                    report+="第"+ga->itemList[i].right+"行：单词 "+ga->itemList[i].left+" 附近有语法错误\r\n";
                        }
                        else
                        {
                            CodeGenerator gen;
                            if(!gen.generate(ga->root))
                            {
                                report="语义错误：\r\n";
                                for(unsigned i=0;i<gen.errors.size();i++) report+=gen.errors[i]+"\r\n";
                            }
                            else
                            {
                                string base=fileName;
                                size_t dot=base.rfind('.'), slash=base.find_last_of("\\/");
                                if(dot!=string::npos&&(slash==string::npos||dot>slash)) base=base.substr(0,dot);
                                string irPath=base+".ir";
                                string listing=gen.ir.listing();
                                FILE* f=fopen(irPath.c_str(),"w");
                                if(f){ fputs(listing.c_str(),f); fclose(f); }
                                DialogIO io(hwnd);
                                Vm vm(gen.ir,io);
                                bool ok=vm.run();
                                report="已生成中间代码："+irPath+"\r\n\r\n==== 程序输出 ====\r\n"+CrLf(io.out);
                                if(!ok) report+="运行错误："+vm.error+"\r\n";
                                report+="\r\n==== 中间代码（四元式） ====\r\n"+CrLf(listing);
                            }
                        }
                        delete ga;
                    }
                    buildText=report;
                    hDlgChild=CreateDialogParam(hInst,MAKEINTRESOURCE(IDC_BUILD_DIALOG),hwnd,BuildDlg,(LPARAM)buildText.c_str());
                    ShowWindow(hDlgChild,SW_SHOW);
                }
                return 0;
		 }
		 break;
     case WM_CLOSE:
         //关闭窗口
          if (IDYES == AskConfirmation (hwnd))
               DestroyWindow (hwnd) ;
          return 0 ;

     case WM_QUERYENDSESSION:
         //系统关闭
          if (IDYES == AskConfirmation (hwnd))
               return 1 ;
          else
               return 0 ;

     case WM_DESTROY:
         //注销程序
          PostQuitMessage (0) ;
          return 0 ;
     }
     return DefWindowProc (hwnd, message, wParam, lParam) ;
}

BOOL CALLBACK WordDlg (HWND hwnd, UINT message,
                           WPARAM wParam, LPARAM lParam)
{

	 RECT      rect;
   	 PAINTSTRUCT ps; //绘图信息结构
	 HDC hdc;       //硬件显示句柄
	 string str;
	 TEXTMETRIC  tm ;
	 TCHAR       szBuffer[10] ;
	 int i;
     static int  cxChar, cxCaps, cyChar, cyClient, pos=0 ,iVscrollPos=0;
     switch (message)
     {
     case WM_PAINT:

          hdc = GetDC (hwnd) ;
          //设置字符格式
          GetTextMetrics(hdc,&tm) ;
          cxChar = tm.tmAveCharWidth ;
          cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2 ;
          cyChar = tm.tmHeight + tm.tmExternalLeading ;

          ReleaseDC (hwnd, hdc) ;
          //设置滚动条
          SetScrollRange (hwnd, SB_VERT, 0, wordProject->tokenList.num - 1, FALSE) ;
          SetScrollPos   (hwnd, SB_VERT, iVscrollPos, TRUE) ;

          //是否语法分析错误
          if(!wordErrorState)
          {
              hdc = BeginPaint (hwnd, &ps) ;
              int i,y;
              for (i = 0 ; i < wordProject->tokenList.List.size() ; i++)
              {
                   y = cyChar * (i- iVscrollPos) ;
                   SetBkMode(hdc, TRANSPARENT);
                   TextOut (hdc, 0, y,
                            wordProject->tokenList.List[i].name.c_str(),
                            lstrlen (wordProject->tokenList.List[i].name.c_str())) ;

                   TextOut (hdc, 15 * cxCaps, y,
                            wordProject->tokenList.List[i].type.c_str(),
                            lstrlen (wordProject->tokenList.List[i].type.c_str())) ;

                   SetTextAlign (hdc, TA_RIGHT | TA_TOP) ;

                   TextOut (hdc, 15 * cxCaps + 25 * cxChar, y, szBuffer,
                            wsprintf (szBuffer, TEXT ("%5d"),
                                 wordProject->tokenList.List[i].line)) ;

                   SetTextAlign (hdc, TA_LEFT | TA_TOP) ;
              }
              EndPaint (hwnd, &ps) ;
          }
          else
          {
              //错误则从错误序列里打印错误信息
              hdc = BeginPaint (hwnd, &ps) ;
              int i,y;
              for (i = 0 ; i < wordProject->error.size() ; i++)
              {
                   y = cyChar * (i- iVscrollPos) ;
                   SetBkMode(hdc, TRANSPARENT);
                   TextOut (hdc, 0, y,
                            wordProject->error[i].c_str(),
                            lstrlen (wordProject->error[i].c_str())) ;
              }
              EndPaint (hwnd, &ps) ;
          }
          return TRUE ;

     case WM_VSCROLL:
        //滚动条的操作
          switch (LOWORD(wParam))
          {
          case SB_LINEUP:
               iVscrollPos -= 1 ;
               break ;

          case SB_LINEDOWN:
               iVscrollPos += 1 ;
               break ;

          case SB_PAGEUP:
               iVscrollPos -= cyClient / cyChar ;
               break ;

          case SB_PAGEDOWN:
               iVscrollPos += cyClient / cyChar ;
               break ;

          case SB_THUMBPOSITION:
               iVscrollPos = HIWORD(wParam) ;
               break ;

          default :
               break ;
          }

          iVscrollPos = max (0, min (iVscrollPos, wordProject->tokenList.num - 1)) ;

          if (iVscrollPos != GetScrollPos (hwnd, SB_VERT))
          {
               SetScrollPos (hwnd, SB_VERT, iVscrollPos, TRUE) ;
               InvalidateRect (hwnd, NULL, TRUE) ;
          }
          return 0 ;

    case WM_SIZE:
          cyClient = HIWORD (lParam) ;
          return 0 ;


    case WM_CLOSE :

          DestroyWindow (hwnd) ;
          cout<<"word over1"<<endl;
          delete(wordProject);
          cout<<"word over2"<<endl;
          hDlgChild = NULL ;
          cout<<"word over3"<<endl;
          return TRUE ;
     }
     return FALSE ;
}

//语法分析子窗口回调函数
BOOL CALLBACK GrammarDlg (HWND hwnd, UINT message,
                           WPARAM wParam, LPARAM lParam)
{

	 RECT      rect;
   	 PAINTSTRUCT ps;
	 HDC hdc;
	 string str;
	 TEXTMETRIC  tm ;
	 TCHAR       szBuffer[10] ;
	 int i;
     static int  cxChar, cxCaps, cyChar, cyClient, pos=0 ,iVscrollPos=0;
     switch (message)
     {
     case WM_PAINT:

          hdc = GetDC (hwnd) ;
          GetTextMetrics(hdc,&tm) ;
          cxChar = tm.tmAveCharWidth ;
          cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2 ;
          cyChar = tm.tmHeight + tm.tmExternalLeading ;

          ReleaseDC (hwnd, hdc) ;

          SetScrollRange (hwnd, SB_VERT, 0, grammarProject->itemList.size() - 1, FALSE) ;
          SetScrollPos   (hwnd, SB_VERT, iVscrollPos, TRUE) ;

          hdc = BeginPaint (hwnd, &ps) ;
          int i,y;

          for (i = 0 ; i < grammarProject->itemList.size() ; i++)
          {
               y = cyChar * (i- iVscrollPos) ;
               SetBkMode(hdc, TRANSPARENT);
               TextOut (hdc, 0, y,
                        grammarProject->itemList[i].left.c_str(),
                        lstrlen (grammarProject->itemList[i].left.c_str())) ;

               TextOut (hdc, 15 * cxCaps, y,
                        grammarProject->itemList[i].oper.c_str(),
                        lstrlen (grammarProject->itemList[i].oper.c_str())) ;

               if(!isNum(grammarProject->itemList[i].right))
               {
                   TextOut (hdc, 15 * cxCaps + 25 * cxChar, y, grammarProject->itemList[i].right.c_str(),
                                 lstrlen(grammarProject->itemList[i].right.c_str())) ;
               }
               else if(grammarProject->itemList[i].right!="")
               {
                   TextOut (hdc, 15 * cxCaps + 55 * cxChar, y, szBuffer,
                            wsprintf (szBuffer, TEXT ("%5d"),
                                 toNum(grammarProject->itemList[i].right))) ;
               }

               SetTextAlign (hdc, TA_LEFT | TA_TOP) ;
          }
          EndPaint (hwnd, &ps) ;
          return TRUE ;

     case WM_VSCROLL:
          switch (LOWORD(wParam))
          {
          case SB_LINEUP:
               iVscrollPos -= 1 ;
               break ;

          case SB_LINEDOWN:
               iVscrollPos += 1 ;
               break ;

          case SB_PAGEUP:
               iVscrollPos -= cyClient / cyChar ;
               break ;

          case SB_PAGEDOWN:
               iVscrollPos += cyClient / cyChar ;
               break ;

          case SB_THUMBPOSITION:
               iVscrollPos = HIWORD(wParam) ;
               break ;

          default :
               break ;
          }

          iVscrollPos = max (0, min (iVscrollPos, (int)grammarProject->itemList.size() - 1)) ;

          if (iVscrollPos != GetScrollPos (hwnd, SB_VERT))
          {
               SetScrollPos (hwnd, SB_VERT, iVscrollPos, TRUE) ;
               InvalidateRect (hwnd, NULL, TRUE) ;
          }
          return 0 ;

    case WM_SIZE:
          cyClient = HIWORD (lParam) ;
          return 0 ;


    case WM_CLOSE :
          DestroyWindow (hwnd) ;
          cout<<"grammar over"<<endl;
          delete(grammarProject);
          cout<<"over2"<<endl;
          hDlgChild = NULL ;
          cout<<"over3"<<endl;
          return TRUE ;

     }
     return FALSE ;
}
void dfs(HWND hwnd,Node* node,int depth,int width)
{
    if(node==NULL)
        return ;
    Node* tempNode = node;
    RECT Grid;
    Node* fatherNode = node->father;
    Node* tempSonNode;
    HPEN  hpen;
    HDC hdc;
    int high=fatherNode->son.size()-1;
	hpen=CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	hdc=GetDC(hwnd);
	//cout<<node->name<<endl;
    if(fatherNode->son[0]==node)    //向下走
    {
        SetWindowOrgEx(hdc,(BLOCK_WIDTH+LINE)*hScroll,(BLOCK_HIGH+LINE)*(vScroll),NULL);
        DrawBorder(hdc,(BLOCK_WIDTH+LINE)*depth,(BLOCK_HIGH+LINE)*width,BLOCK_WIDTH,BLOCK_HIGH,1,1);
        Grid.left=(BLOCK_WIDTH+LINE)*depth;
        Grid.right=(BLOCK_WIDTH+LINE)*depth+BLOCK_WIDTH;
        Grid.top=(BLOCK_HIGH+LINE)*width;
        Grid.bottom=(BLOCK_HIGH+LINE)*width+BLOCK_HIGH;
        SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc,(node->name).c_str(),-1,&Grid,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
        SelectObject(hdc,hpen);
        MoveToEx(hdc,(BLOCK_WIDTH+LINE)*(depth-1)+BLOCK_WIDTH,(BLOCK_HIGH+LINE)*width+LINE/2,NULL);
        LineTo(hdc,(BLOCK_WIDTH+LINE)*depth,(BLOCK_HIGH+LINE)*width+LINE/2);
        ReleaseDC (hwnd, hdc);
        DeleteObject(hpen);
        tempSonNode=node->getSon();
        dfs(hwnd,tempSonNode,depth+1,width);
    }

    while(fatherNode->curSon <= high)  //取下一个兄弟节点
    {
        //cout<<"error"<<endl;
        hdc=GetDC(hwnd);
        maxWidth++;
        tempSonNode=fatherNode->getSon();
        SetWindowOrgEx(hdc,(BLOCK_WIDTH+LINE)*hScroll,(BLOCK_HIGH+LINE)*(vScroll),NULL);
        DrawBorder(hdc,(BLOCK_WIDTH+LINE)*depth,(BLOCK_HIGH+LINE)*maxWidth,BLOCK_WIDTH,BLOCK_HIGH,1,1);
        Grid.left=(BLOCK_WIDTH+LINE)*depth;
        Grid.right=(BLOCK_WIDTH+LINE)*depth+BLOCK_WIDTH;
        Grid.top=(BLOCK_HIGH+LINE)*maxWidth;
        Grid.bottom=(BLOCK_HIGH+LINE)*maxWidth+BLOCK_HIGH;
        SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc,(tempSonNode->name).c_str(),-1,&Grid,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
        SelectObject(hdc,hpen);
        MoveToEx(hdc,(BLOCK_WIDTH+LINE)*depth+BLOCK_WIDTH/2,(BLOCK_HIGH+LINE)*width+BLOCK_HIGH,NULL);
        LineTo(hdc,(BLOCK_WIDTH+LINE)*depth+BLOCK_WIDTH/2,(BLOCK_HIGH+LINE)*maxWidth);
        ReleaseDC (hwnd, hdc);
        DeleteObject(hpen);

        width=maxWidth;
        dfs(hwnd,tempSonNode->getSon(),depth+1,maxWidth);
    }
}
BOOL CALLBACK TreeDlg (HWND hwnd, UINT message,
                           WPARAM wParam, LPARAM lParam)
{

	 RECT      rect;
   	 PAINTSTRUCT ps;
   	 Node* a ;
	 HDC hdc;
	 string str;
	 TEXTMETRIC  tm ;
	 TCHAR       szBuffer[10] ;
	 Node* root = grammarProject->root;
	 RECT Grid;
	 int i,depth=grammarProject->maxDepth;
     static int  cxChar, cxCaps, cyChar, cyClient, pos=0 ,iVscrollPos=0,hVscrollPos=0;
     switch (message)
     {
     case WM_PAINT:
          hdc=GetDC(hwnd);
          SetWindowOrgEx(hdc,(BLOCK_WIDTH+LINE)*hVscrollPos,(BLOCK_HIGH+LINE)*(iVscrollPos),NULL);   //设置坐标
          vScroll=iVscrollPos;
          hScroll=hVscrollPos;
          a=root->getSon();
          //cout<<depth<<endl;
          //cout<<"draw tree start"<<endl;
          DrawBorder(hdc,0,0,BLOCK_WIDTH,BLOCK_HIGH,1,1);  //绘制初始节点
          Grid.left=0;
          Grid.right=BLOCK_WIDTH;
          Grid.top=0;
          Grid.bottom=BLOCK_HIGH;
          SetBkMode(hdc, TRANSPARENT);
          DrawText(hdc,(a->name).c_str(),-1,&Grid,DT_SINGLELINE|DT_CENTER|DT_VCENTER);

          dfs(hwnd,a,1,0);  //递归开始
          for(i=0;i<grammarProject->nodeList.size();i++)
              grammarProject->nodeList[i]->curSon=0;
          root=grammarProject->root;   //重新初始化
          constMaxWidth=maxWidth;
          SetScrollRange (hwnd, SB_VERT, 0, maxWidth - 1, FALSE) ;
          SetScrollPos   (hwnd, SB_VERT, iVscrollPos, TRUE) ;
          SetScrollRange (hwnd, SB_HORZ, 0, grammarProject->maxDepth - 1, FALSE) ;
          SetScrollPos   (hwnd, SB_HORZ, hVscrollPos, TRUE) ;
          maxWidth=0;


          return TRUE ;


    case WM_SIZE:
          cyClient = HIWORD (lParam) ;
          return 0 ;

    case WM_VSCROLL:
      switch (LOWORD(wParam))
      {
          case SB_LINEUP:
               iVscrollPos -= 1 ;
               break ;

          case SB_LINEDOWN:
               iVscrollPos += 1 ;
               break ;

          case SB_THUMBPOSITION:
               iVscrollPos = HIWORD(wParam) ;
               break ;

          default :
               break ;
      }

      iVscrollPos = max (0, min (iVscrollPos, constMaxWidth - 1)) ;


      if (iVscrollPos != GetScrollPos (hwnd, SB_VERT))
      {
           SetScrollPos (hwnd, SB_VERT, iVscrollPos, TRUE) ;
           InvalidateRect (hwnd, NULL, TRUE) ;
      }
      return 0 ;
    case WM_HSCROLL:
      switch (LOWORD(wParam))
      {
          case SB_LINELEFT:
               hVscrollPos -= 1 ;
               break ;

          case SB_LINERIGHT:
               hVscrollPos += 1 ;
               break ;

          case SB_THUMBPOSITION:
               hVscrollPos = HIWORD(wParam) ;
               break ;

          default :
               break ;
      }

      hVscrollPos = max (0, min (hVscrollPos, grammarProject->maxDepth - 1)) ;


      if (hVscrollPos != GetScrollPos (hwnd, SB_HORZ))
      {
           SetScrollPos (hwnd, SB_HORZ, hVscrollPos, TRUE) ;
           InvalidateRect (hwnd, NULL, TRUE) ;
      }
      return 0 ;
    case WM_CLOSE :
          DestroyWindow (hwnd) ;
          cout<<"over"<<endl;
          delete(grammarProject);
          cout<<"over2"<<endl;
          hDlgChild = NULL ;
          cout<<"over3"<<endl;
          return TRUE ;

     }
     return FALSE ;
}

//一键编译结果窗口：一个只读的多行编辑框，显示生成的 C 程序或错误列表
BOOL CALLBACK BuildDlg (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
     switch (message)
     {
     case WM_INITDIALOG:
          SetDlgItemText (hwnd, IDC_BUILD_EDIT, (LPCSTR) lParam) ;
          return TRUE ;
     case WM_SIZE:
          MoveWindow (GetDlgItem (hwnd, IDC_BUILD_EDIT), 0, 0, LOWORD (lParam), HIWORD (lParam), TRUE) ;
          return TRUE ;
     case WM_CLOSE:
          DestroyWindow (hwnd) ;
          hDlgChild = NULL ;
          return TRUE ;
     }
     return FALSE ;
}
