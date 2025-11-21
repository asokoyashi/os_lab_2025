#!/bin/bash

# Если аргументов нет — выходим с ошибкой
if [ $# -eq 0 ]; then
    echo "Ошибка: не переданы аргументы"
    echo "Использование: $0 число1 число2 ..."
    exit 1
fi

sum=0
count=0

for num in "$@"; do
    sum=$((sum + num))
    count=$((count + 1))
done

# Делим на количество, используем bc для чисел с плавающей точкой
average=$(echo "scale=3; $sum / $count" | bc)

echo "Количество чисел: $count"
echo "Среднее арифметическое: $average"
