#include "header.h"    //global header
#include "Grammar.h"    //parser header
#include "Word.h"       //lexer header
#include "resource.h"   //resources
#include "Gen.h"        //semantic analysis and code generation header
#include "Vm.h"         //intermediate-code virtual machine header
using namespace std;

#define ID_EDIT     1
#define BLOCK_HIGH 20
#define BLOCK_WIDTH 100
#define LINE 30
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);  //main window callback
wordScanner* wordProject;                               //lexical analysis instance
GrammarAnalyzer* grammarProject;                        //syntax analysis instance
static TCHAR szAppName[] = TEXT ("snl") ;               //application name
int maxWidth=0;
HINSTANCE hInst;                                        //application instance handle
OPENFILENAME ofn ;                                      //file information structure
HWND hwndEdit ;                                         //child window handle
HWND hDlgChild = NULL ;                                  //child window handle
BOOL CALLBACK WordDlg (HWND, UINT, WPARAM, LPARAM);     //lexical analysis child-window callback
BOOL CALLBACK GrammarDlg(HWND, UINT, WPARAM, LPARAM);   //syntax analysis child-window callback
BOOL CALLBACK TreeDlg(HWND, UINT, WPARAM, LPARAM);   //syntax tree child-window callback
BOOL CALLBACK BuildDlg(HWND, UINT, WPARAM, LPARAM);  //one-click compile result-window callback
static string buildText;                                //text shown in the one-click compile result window

//turn \n into \r\n (edit controls and multiline text boxes only accept \r\n)
static string CrLf (const string& s)
{
     string r ;
     for (size_t i = 0 ; i < s.size () ; i++) { if (s[i] == '\n') r += "\r\n" ; else r += s[i] ; }
     return r ;
}
//program input dialog: popped up when the virtual machine reaches a read
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
//the virtual machine's I/O: read pops a dialog for input, write accumulates its output into a string
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
     HACCEL   hAccel ;      //accelerator table handle
     HWND     hwnd ;        //main window handle
     MSG      msg ;         //message
     WNDCLASS wndclass ;    //window class
	 HMENU    hMenu;        //menu handle
	 hInst=hInstance;       //store the instance handle
     wndclass.style         = CS_HREDRAW | CS_VREDRAW ; //set the window style
     wndclass.lpfnWndProc   = WndProc ;  //set the callback
     wndclass.cbClsExtra    = 0 ;
     wndclass.cbWndExtra    = 0 ;
     wndclass.hInstance     = hInstance ;  //application instance handle
     wndclass.hIcon         = (HICON)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)) ; //load the icon
     wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW) ; //load the cursor
     wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH) ; //get the brush
     wndclass.lpszMenuName  = szAppName ;  //menu name
     wndclass.lpszClassName = szAppName ;  //window class name

     if (!RegisterClass (&wndclass))  //did the window class register successfully
     {
          MessageBox (NULL, TEXT ("This program requires Windows NT!"),
                      szAppName, MB_ICONERROR) ;
          return 0 ;
     }
     hMenu = LoadMenu(hInstance,MAKEINTRESOURCE(IDR_MENU1)) ; //load the menu
     hwnd = CreateWindow (szAppName, szAppName,
                          WS_OVERLAPPEDWINDOW,
                          GetSystemMetrics (SM_CXSCREEN) / 4,
                          GetSystemMetrics (SM_CYSCREEN) / 4,
                          GetSystemMetrics (SM_CXSCREEN) / 2,
                          GetSystemMetrics (SM_CYSCREEN) / 2,
                          NULL, hMenu, hInstance, NULL) ;  //create the window

     ShowWindow (hwnd, iCmdShow) ; //set the show state
     UpdateWindow (hwnd) ;         //paint the window

     hAccel = LoadAccelerators (hInstance, szAppName) ;  //load the accelerator table

     while (GetMessage (&msg, NULL, 0, 0))  //get a message
     {
         /*first test: if the lexer window is not open, the parser and syntax-tree windows are not open either
           second test: check whether a child window is open and the message belongs to it*/

          if (hDlgChild == 0||!IsDialogMessage(hDlgChild, &msg))
		 {
             if (!TranslateAccelerator (hwnd, hAccel, &msg)) //translate accelerator keys into recognizable messages
			 {
                TranslateMessage (&msg) ;
                DispatchMessage (&msg) ;
			 }
		 }
     }
     return msg.wParam ;
}

int toNum(string str)  //convert a string to an int
{
    int s=str.size();  //get the length of the string
    int i,num=0;
    for(i=0;i<s;i++)
    {
        num=num*10+(str[s-i-1]-'0'); //take the low digit first, then multiply by 10 to build the next one
    }
    return num;
}

bool isNum(string str)  //test whether a string can be converted to a number
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
//pop up a confirmation box
AskConfirmation (HWND hwnd)
{
     return MessageBox (hwnd, TEXT ("是否想要推出编译器?"),
                        szAppName, MB_YESNO | MB_ICONQUESTION) ;
}

//change the window title
void DoCaption (HWND hwnd, TCHAR * szTitleName)
{
     TCHAR szCaption[64 + MAX_PATH] ;

     wsprintf (szCaption, TEXT ("%s - %s"), szAppName,
               szTitleName[0] ? szTitleName : UNTITLED) ;

     SetWindowText (hwnd, szCaption) ;
}

//initialize the file information structure
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

//open a file
BOOL PopFileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName)
{
     ofn.hwndOwner         = hwnd ;
     ofn.lpstrFile         = pstrFileName ;
     ofn.lpstrFileTitle    = pstrTitleName ;
     ofn.Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT ;

     return GetOpenFileName(&ofn) ;
}

//OK message box helper
void OkMessage (HWND hwnd, TCHAR * szMessage, TCHAR * szTitleName)
{
     TCHAR szBuffer[64 + MAX_PATH] ;

     wsprintf (szBuffer, szMessage, szTitleName[0] ? szTitleName : UNTITLED) ;

     MessageBox (hwnd, szBuffer, szAppName, MB_OK | MB_ICONEXCLAMATION) ;
}

//turn lone \n into \r\n, otherwise files written on Mac / Linux do not break lines in the edit control
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
//read a file and display it
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
          //skip a UTF-8 BOM
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
          //if the source is UTF-8 (e.g. written on Mac / Linux), convert it to the current code page first so Chinese in the edit control is not garbled
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
	//when WTop is TRUE the top uses the white pen and the bottom the black pen

	int i;
	HPEN  hpen;
	hpen=CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	if (WTop)
		SelectObject(hdc,GetStockObject(WHITE_PEN));
	else
		SelectObject(hdc,hpen);
	//draw the top and left edges
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

	//draw the bottom and right edges
	for (i=0; i<BorderWidth; i++)
	{
		MoveToEx(hdc,x+Width-i, y+Height-i,NULL);
		LineTo(hdc,x+Width-i, y+i);
		MoveToEx(hdc,x+Width-i, y+Height-i,NULL);
		LineTo(hdc,x+i, y+Height-i);
	}

	DeleteObject(hpen);
}
//main window callback
LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

     int         iSelect, iEnable ;
     static TCHAR     szFileName[MAX_PATH], szTitleName[MAX_PATH] ;
     static string fileName=""; //remembered file name
     switch (message)
     {
     case WM_CREATE:
          hwndEdit = CreateWindow (TEXT ("edit"), NULL,
                              WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
                              WS_BORDER | ES_LEFT | ES_MULTILINE |
                              ES_AUTOHSCROLL | ES_AUTOVSCROLL,
                              0, 0, 0, 0, hwnd, (HMENU) ID_EDIT,
                              ((LPCREATESTRUCT) lParam)->hInstance, NULL) ; //create the child window used for input
		  PopFileInitialize (hwnd) ;  //initialize the file information structure
          return 0 ;

     case WM_SETFOCUS:   //when this window gains focus
          SetFocus (hwndEdit) ;
          return 0 ;

     case WM_SIZE:  //when this window is resized
          MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE) ;
          return 0 ;

     case WM_INITMENUPOPUP:  //update the menu bar state
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

	 case WM_COMMAND: //a menu message
		 switch(LOWORD(wParam))
          {
			 case IDC_OPEN:

                //try to open the file
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
                //set the title
                DoCaption (hwnd, szTitleName) ;
                return 0 ;

             case IDC_WORD:
                //lexical analysis was chosen
                if(szFileName[0]=='\0')
                {
                    MessageBox(hwnd,"请打开文件","提醒",MB_OK);
                    return 0;
                }
                fileName=szFileName;
                //start lexical analysis
                wordProject = new wordScanner(szFileName,hwnd);
                //wordProject = new wordScanner(szFileName,hwnd);
                wordProject->start();
                //create the lexical analysis window
                hDlgChild = CreateDialog(hInst,MAKEINTRESOURCE(IDC_WORD_DIALOG),hwnd, WordDlg);
                ShowWindow(hDlgChild,SW_SHOW);

                return 0;

             case IDC_GRAMMAR:
                 //did lexical analysis report an error
                if(wordErrorState==true)
                {
                    MessageBox(hwnd,"词法错误","提醒",MB_OK);
                    return 0;
                }
                //has lexical analysis been run yet
                if(fileName=="")
                {
                    MessageBox(hwnd,"先进行词法分析","提醒",MB_OK);
                    return 0;
                }
                //start syntax analysis
                grammarProject = new GrammarAnalyzer(fileName.c_str(),hwnd);
                //grammarProject = new GrammarAnalyzer(fileName.c_str(),hwnd);
                grammarProject->start();

                //create the syntax analysis window


                hDlgChild = CreateDialog(hInst,MAKEINTRESOURCE(IDC_GRAMMAR_DIALOG),hwnd,GrammarDlg);
                ShowWindow(hDlgChild,SW_SHOW);
                return 0;
             case IDC_AUTHOR:
                 //show the author information
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
                //has lexical analysis been run yet
                if(fileName=="")
                {
                    MessageBox(hwnd,"先进行词法分析","提醒",MB_OK);
                    return 0;
                }
                //start syntax analysis
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
                //one-click compile: lexical -> syntax -> semantic analysis and intermediate code generation -> run directly on the virtual machine
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
         //close the window
          if (IDYES == AskConfirmation (hwnd))
               DestroyWindow (hwnd) ;
          return 0 ;

     case WM_QUERYENDSESSION:
         //system shutdown
          if (IDYES == AskConfirmation (hwnd))
               return 1 ;
          else
               return 0 ;

     case WM_DESTROY:
         //tear down the program
          PostQuitMessage (0) ;
          return 0 ;
     }
     return DefWindowProc (hwnd, message, wParam, lParam) ;
}

BOOL CALLBACK WordDlg (HWND hwnd, UINT message,
                           WPARAM wParam, LPARAM lParam)
{

	 RECT      rect;
   	 PAINTSTRUCT ps; //paint information structure
	 HDC hdc;       //device context handle
	 string str;
	 TEXTMETRIC  tm ;
	 TCHAR       szBuffer[10] ;
	 int i;
     static int  cxChar, cxCaps, cyChar, cyClient, pos=0 ,iVscrollPos=0;
     switch (message)
     {
     case WM_PAINT:

          hdc = GetDC (hwnd) ;
          //set the character metrics
          GetTextMetrics(hdc,&tm) ;
          cxChar = tm.tmAveCharWidth ;
          cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2 ;
          cyChar = tm.tmHeight + tm.tmExternalLeading ;

          ReleaseDC (hwnd, hdc) ;
          //set up the scroll bar
          SetScrollRange (hwnd, SB_VERT, 0, wordProject->tokenList.num - 1, FALSE) ;
          SetScrollPos   (hwnd, SB_VERT, iVscrollPos, TRUE) ;

          //whether lexical analysis found an error
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
              //on error, print the messages from the error list
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
        //handle the scroll bar
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

//syntax analysis child-window callback
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
    if(fatherNode->son[0]==node)    //go down
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

    while(fatherNode->curSon <= high)  //take the next sibling node
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
          SetWindowOrgEx(hdc,(BLOCK_WIDTH+LINE)*hVscrollPos,(BLOCK_HIGH+LINE)*(iVscrollPos),NULL);   //set the coordinate origin
          vScroll=iVscrollPos;
          hScroll=hVscrollPos;
          a=root->getSon();
          //cout<<depth<<endl;
          //cout<<"draw tree start"<<endl;
          DrawBorder(hdc,0,0,BLOCK_WIDTH,BLOCK_HIGH,1,1);  //draw the root node
          Grid.left=0;
          Grid.right=BLOCK_WIDTH;
          Grid.top=0;
          Grid.bottom=BLOCK_HIGH;
          SetBkMode(hdc, TRANSPARENT);
          DrawText(hdc,(a->name).c_str(),-1,&Grid,DT_SINGLELINE|DT_CENTER|DT_VCENTER);

          dfs(hwnd,a,1,0);  //start the recursion
          for(i=0;i<grammarProject->nodeList.size();i++)
              grammarProject->nodeList[i]->curSon=0;
          root=grammarProject->root;   //reinitialize
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

//one-click compile result window: a read-only multiline edit control showing the program output and intermediate code, or the error list
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
