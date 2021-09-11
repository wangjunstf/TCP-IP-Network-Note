#!/bin/bash

# 向一个文件填充300M的数据

file=news.txt
((size=300*1024*3))
for (( i=1; i<=size; i++)) 
do
    echo -n 'c' >> $file
done