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

.PHONY: all test clean
