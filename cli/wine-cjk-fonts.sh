#!/bin/sh
# 让 Wine 里的中文正常显示。
# Wine 自带的字体没有中文字形，而中文区域设置下默认 GUI 字体映射到 SimSun（宋体），它又不存在，
# 结果菜单、对话框和 TextOut 画出来的中文全是 □□。这里把这些字体名替换成一个已有的 CJK 字体：
# macOS 上 Wine 会自动登记系统字体，"Arial Unicode MS" 一定存在；Linux 上可用 FONT 环境变量指定，
# 例如 FONT="WenQuanYi Micro Hei" 或 "Noto Sans CJK SC"。
# 用法: sh cli/wine-cjk-fonts.sh   （对当前 ~/.wine 前缀生效一次即可）
set -e
WINE=${WINE:-wine}
FONT=${FONT:-"Arial Unicode MS"}
export LANG=zh_CN.UTF-8 WINEDEBUG=-all MVK_CONFIG_LOG_LEVEL=0   # 后者关掉 MoltenVK 的启动日志
for n in SimSun NSimSun SimHei "Microsoft YaHei" "Microsoft YaHei UI" 宋体 新宋体 黑体 微软雅黑; do
    "$WINE" reg add 'HKCU\Software\Wine\Fonts\Replacements' /v "$n" /t REG_SZ /d "$FONT" /f > /dev/null
done
echo "Wine font replacements set: SimSun / SimHei / Microsoft YaHei ... -> $FONT"
