#!/bin/bash
#每隔2秒就更新一下html,保证gcov的结果(html形式)是最新的，所有html都保存在当前目录下的html目录
while [ 1 ] 
do
for f in `ls | grep gcno`
do
    gcov $f >/dev/null 2>&1
done
lcov -b ./ -d ./ -c -o server.info >/dev/null 2>&1
genhtml server.info -o html >/dev/null 2>&1
sleep 2
done

