#!/bin/sh
# 跑 tests/ 下全部用例（由 make test 调用）。
#   * 文件名以 e 开头的用例期望编译失败（snl_cli 退出码 1），其余期望成功（退出码 0）
#   * 成功的用例还会用 cc 编译生成的 C 程序；若有 tests/<名字>.in 则作为标准输入运行，
#     若有 tests/<名字>.expected 则比对程序输出
#   * 用例先拷到输出目录再跑，生成的 .token / .c 文件不会弄脏 tests/；perl alarm 防止死循环挂住
BIN=${1:-build/snl_cli}; OUT=${2:-build/test}
mkdir -p "$OUT"; fail=0
for t in tests/*.txt; do
    n=$(basename "$t" .txt); cp "$t" "$OUT/$n.txt"
    case $n in e*) want=1;; *) want=0;; esac
    perl -e 'alarm 20; exec @ARGV' "./$BIN" "$OUT/$n.txt" > "$OUT/$n.out" 2>&1; got=$?
    if [ "$got" -ne "$want" ]; then echo "FAIL  $n (exit $got, want $want)"; tail -5 "$OUT/$n.out"; fail=1; continue; fi
    if [ "$want" -eq 0 ]; then
        if ! cc -w -o "$OUT/$n.bin" "$OUT/$n.c" 2> "$OUT/$n.cc.log"; then
            echo "FAIL  $n (generated C does not compile)"; head -5 "$OUT/$n.cc.log"; fail=1; continue; fi
        in=/dev/null; [ -f "tests/$n.in" ] && in="tests/$n.in"
        perl -e 'alarm 20; exec @ARGV' "$OUT/$n.bin" < "$in" > "$OUT/$n.run" 2>&1
        if [ -f "tests/$n.expected" ] && ! diff -q "tests/$n.expected" "$OUT/$n.run" > /dev/null; then
            echo "FAIL  $n (program output differs)"; diff "tests/$n.expected" "$OUT/$n.run" | head -5; fail=1; continue; fi
    fi
    echo "PASS  $n"
done
exit $fail
