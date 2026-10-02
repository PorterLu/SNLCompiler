# 在 macOS / Linux 上把 SNL 编译器核心（词法 + 语法分析）编译成命令行程序 build/snl_cli。
# 不修改 ../SNL_COMPILER 下的任何源码：
#   * <windows.h> 由 compat/windows.h 桩文件代替（只提供 HWND / TCHAR / TEXT 的定义）
#   * 源码是 GBK 编码 + CRLF 换行，编译前先转成 UTF-8 + LF 放到 build/src/，终端输出才不乱码
#
# 用法:  make          编译
#        make test     跑 tests/ 下的全部测试程序（t* 应成功，e* 应报错）
#        make clean
SRC_DIR  ?= ../SNL_COMPILER
BUILD    ?= build
CXX      ?= c++
CXXFLAGS ?= -std=c++11 -g -O0 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-sign-compare

CORE_SRCS  = Word.cpp Grammar.cpp
CORE_HDRS  = header.h Word.h Grammar.h
UTF8_FILES = $(addprefix $(BUILD)/src/,$(CORE_SRCS) $(CORE_HDRS))
OBJS       = $(addprefix $(BUILD)/,$(CORE_SRCS:.cpp=.o)) $(BUILD)/cli_main.o
TESTS      = $(sort $(wildcard tests/*.txt))

all: $(BUILD)/snl_cli

$(BUILD)/snl_cli: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# 转码: GBK + CRLF  ->  UTF-8 + LF
$(BUILD)/src/%: $(SRC_DIR)/%
	@mkdir -p $(BUILD)/src
	iconv -f GBK -t UTF-8 $< | LC_ALL=C tr -d '\r' > $@

$(BUILD)/%.o: $(BUILD)/src/%.cpp $(UTF8_FILES)
	$(CXX) $(CXXFLAGS) -I$(BUILD)/src -Icompat -c $< -o $@

$(BUILD)/cli_main.o: cli_main.cpp $(UTF8_FILES)
	$(CXX) $(CXXFLAGS) -I$(BUILD)/src -Icompat -c $< -o $@

# 测试程序先拷到 build/test 下再跑，生成的 .token 文件不会弄脏 tests/ 目录。
# 文件名以 e 开头的用例期望退出码 1（词法/语法错误），其余期望 0。perl alarm 防止死循环挂住。
test: $(BUILD)/snl_cli
	@mkdir -p $(BUILD)/test; fail=0; \
	for t in $(TESTS); do \
	    n=$$(basename $$t); cp $$t $(BUILD)/test/$$n; \
	    case $$n in e*) want=1;; *) want=0;; esac; \
	    perl -e 'alarm 20; exec @ARGV' ./$(BUILD)/snl_cli $(BUILD)/test/$$n > $(BUILD)/test/$${n%.txt}.out 2>&1; got=$$?; \
	    if [ $$got -eq $$want ]; then echo "PASS  $$n"; \
	    else echo "FAIL  $$n (exit $$got, want $$want)"; tail -5 $(BUILD)/test/$${n%.txt}.out; fail=1; fi; \
	done; exit $$fail

clean:
	rm -rf $(BUILD)


# ---- Windows GUI 版交叉编译（需要 brew install mingw-w64），产物 build/win/SNL_COMPILER.exe 可直接用 Wine 运行 ----
#   * compat-win/afxres.h：MinGW 没有 MFC 的 afxres.h，SNL.rc 只需要 windows.h 里的宏
#   * -fpermissive：main.cpp 里 AskConfirmation 没写返回类型；-static：不依赖 libstdc++/libgcc 的 dll
#   * 源码按 GBK 编译，字符串保持 GBK，和原来 Code::Blocks 下的 ANSI 构建一致
MINGW   ?= x86_64-w64-mingw32
WINSRCS  = $(SRC_DIR)/main.cpp $(SRC_DIR)/Word.cpp $(SRC_DIR)/Grammar.cpp
WINHDRS  = $(SRC_DIR)/header.h $(SRC_DIR)/Word.h $(SRC_DIR)/Grammar.h $(SRC_DIR)/resource.h

win: $(BUILD)/win/SNL_COMPILER.exe

$(BUILD)/win/SNL.res: $(SRC_DIR)/SNL.rc $(SRC_DIR)/resource.h compat-win/afxres.h
	@mkdir -p $(BUILD)/win
	$(MINGW)-windres -Icompat-win -I$(SRC_DIR) -O coff -o $@ $<

$(BUILD)/win/SNL_COMPILER.exe: $(WINSRCS) $(WINHDRS) $(BUILD)/win/SNL.res
	$(MINGW)-g++ -O2 -fpermissive -w -finput-charset=GBK -fexec-charset=GBK -static -mwindows -o $@ $(WINSRCS) $(BUILD)/win/SNL.res -lgdi32 -luser32 -lkernel32 -lcomctl32 -lcomdlg32

.PHONY: all test win clean
