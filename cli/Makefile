# 在 macOS / Linux 上把 SNL 编译器核心（词法、语法、语义分析、中间代码生成、虚拟机）编译成命令行程序 build/snl_cli。
# 不修改 ../SNL_COMPILER 下的任何源码：<windows.h> 由 compat/windows.h 桩文件代替（只提供 HWND / TCHAR / TEXT 的定义）。
# 源码是 UTF-8 编码、CRLF 换行，clang / gcc 直接编译。
#
# 用法:  make          编译
#        make test     跑 tests/ 下的全部测试程序（见 run-tests.sh）
#        make win      用 mingw-w64 交叉编译 Windows GUI 版（见下）
#        make clean
SRC_DIR  ?= ../SNL_COMPILER
BUILD    ?= build
CXX      ?= c++
CXXFLAGS ?= -std=c++11 -g -O0 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-sign-compare

CORE_SRCS  = Word.cpp Grammar.cpp Gen.cpp Vm.cpp
CORE_HDRS  = $(addprefix $(SRC_DIR)/,header.h Word.h Grammar.h Gen.h Ir.h Vm.h)
OBJS       = $(addprefix $(BUILD)/,$(CORE_SRCS:.cpp=.o)) $(BUILD)/cli_main.o

all: $(BUILD)/snl_cli

$(BUILD)/snl_cli: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

$(BUILD)/%.o: $(SRC_DIR)/%.cpp $(CORE_HDRS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -Icompat -c $< -o $@

$(BUILD)/cli_main.o: cli_main.cpp $(CORE_HDRS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -Icompat -c $< -o $@

# 测试见 run-tests.sh：t* 必须编译成功且在虚拟机上运行的输出正确，e* 必须报错
test: $(BUILD)/snl_cli
	sh run-tests.sh $(BUILD)/snl_cli $(BUILD)/test

clean:
	rm -rf $(BUILD)

# ---- Windows GUI 版交叉编译（需要 brew install mingw-w64），产物 build/win/SNL_COMPILER.exe 可直接用 Wine 运行 ----
#   * compat-win/afxres.h：MinGW 没有 MFC 的 afxres.h，SNL.rc 只需要 windows.h 里的宏
#   * -fpermissive：main.cpp 里 AskConfirmation 没写返回类型；-static：不依赖 libstdc++/libgcc 的 dll
#   * 源码是 UTF-8，但 Windows 版的字符串要编成 GBK，ANSI 版的 MessageBox / 编辑框才能正确显示中文
MINGW    ?= x86_64-w64-mingw32
WINFLAGS ?= -O2 -s
WINSRCS   = $(addprefix $(SRC_DIR)/,main.cpp $(CORE_SRCS))
WINHDRS   = $(CORE_HDRS) $(SRC_DIR)/resource.h

win: $(BUILD)/win/SNL_COMPILER.exe

$(BUILD)/win/SNL.res: $(SRC_DIR)/SNL.rc $(SRC_DIR)/resource.h compat-win/afxres.h
	@mkdir -p $(BUILD)/win
	$(MINGW)-windres -Icompat-win -I$(SRC_DIR) -O coff -o $@ $<

$(BUILD)/win/SNL_COMPILER.exe: $(WINSRCS) $(WINHDRS) $(BUILD)/win/SNL.res
	$(MINGW)-g++ $(WINFLAGS) -fpermissive -w -finput-charset=UTF-8 -fexec-charset=GBK -static -mwindows -o $@ $(WINSRCS) $(BUILD)/win/SNL.res -lgdi32 -luser32 -lkernel32 -lcomctl32 -lcomdlg32

.PHONY: all test win clean
