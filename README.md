## SNL Compiler
  This is a SNL language compiler written by our group. It converts a SNL program into concrete syntax tree. We provide a test program. 
  
```snl
program sd
type  i=integer;
var  i x;
procedure sd(integer a);
begin 
   write(a)
end    
begin
   a:=1
end.
```

## Build and test on macOS / Linux

Only the Win32 GUI in `SNL_COMPILER/main.cpp` depends on Windows. The lexer and parser
build as a command-line tool with clang or gcc:

```sh
cd cli
make          # builds build/snl_cli
make test     # runs tests/*.txt  (t* must parse, e* must report an error)
./build/snl_cli tests/t02_expr.txt
```

The tool prints the token list, the LL(1) analysis steps and the syntax tree.
The sources stay GBK + CRLF; the Makefile converts copies to UTF-8 before compiling.
