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

The tool prints the token list, the LL(1) analysis steps, the syntax tree and the intermediate
code (quadruples) generated from the tree, which is also written to `<source>.ir`. With `--run` the
intermediate code is executed on the built-in virtual machine (`read` from stdin, `write` to stdout).
`make test` checks that every `tests/t*.txt` compiles and that running it with `tests/<name>.in`
produces `tests/<name>.expected`; `tests/e*.txt` must be rejected.
The sources are UTF-8 (CRLF). The Windows build passes `-finput-charset=UTF-8 -fexec-charset=GBK`
so that the ANSI GUI still shows Chinese correctly; the Code::Blocks project has these options set.

### Hello world

One extension to standard SNL is `write("text")`, which prints a string constant, so a hello
world program is just:

```snl
program hello
begin
   write("hello world")
end.
```

Open it in the GUI and choose 编译 → 一键编译 (one-click compile). The window below shows the
result: the program output `hello world` and the quadruples generated from the syntax tree.
You can also pass the file on the command line and it is compiled and run on launch.

![hello world in the GUI](docs/hello_world.png)

The same program on the command line:

```sh
cd cli && make
./build/snl_cli tests/t11_hello.txt --run
```

An addition program that reads two numbers and writes their sum is in `cli/tests/t12_add.txt`.

In the Windows GUI the same pipeline is behind the menu item 编译 → 一键编译: lexical, syntax and
semantic analysis in one click, then the program runs on the virtual machine (a dialog asks for
input on `read`); the window shows the program output and the intermediate code, which is also
written next to the source as `<source>.ir`.

## Running the Windows GUI on macOS with Wine

```sh
# Wine: the Homebrew wine casks are disabled; install the WineHQ build from
# https://github.com/Gcenx/macOS_Wine_builds/releases  (unpack, move "Wine Stable.app" to /Applications)
export PATH="/Applications/Wine Stable.app/Contents/Resources/wine/bin:$PATH"
sh cli/wine-cjk-fonts.sh                     # once per Wine prefix, otherwise Chinese text shows as boxes
brew install mingw-w64 && make -C cli win    # rebuild the GUI exe from the current sources
LANG=zh_CN.UTF-8 wine cli/build/win/SNL_COMPILER.exe
```
