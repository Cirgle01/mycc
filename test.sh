#!/bin/bash

OPT_FLAG=""

assert() {
    expected="$1"
    input="$2"

    echo -e "$input" > tmp.txt
    ./mycc tmp.txt $OPT_FLAG > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo -e "$input --> $actual"
    else
        echo -e "$input --> $expected expected, but got $actual"
        exit 1
    fi
}

assert_error() {
    input="$1"

    echo -e "$input" > tmp.txt
    ./mycc tmp.txt $OPT_FLAG > tmp.s

    if [ $? -ne 0 ]; then
        echo -e "$input find error"
    else
        echo -e "$input have error, but not be found"
        exit 1
    fi
}

if [ "$1" = "1" ]; then
    OPT_FLAG="-O"
    echo "启用四元式优化"
fi

assert 0 '0;'
assert 42 '42;'

assert 41 " 12 + 34 - 5 ;"

assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'

assert 6 '-5+11;'
assert 10 '- - +10;'

assert 0 '0==1;'
assert 1 '42==42;'
assert 1 '0!=1;'
assert 0 '42!=42;'

assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'

assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'

assert 9 "(20 / 4) + (6 - 2);"
assert 2 "(5 == 5) + (3 != 4);"
assert 20 "(10 > 5) * 20 + (3 < 2);"
assert 100 "(15 <= 15) * 100 + (7 >= 8) * 50;"
assert 11 "-(-5) + 3 * 2;"
assert 1 "((2 + 3) * 4) == (5 * 4);"
assert 4 "(100 / (5 + 5)) - (3 * 2);"

assert 6 'foo = 1;\nbar = 2 + 3;\n foo + bar;'
assert 6 'foo_ = 1;bar1 = 2 + 3;foo_ + bar1;'
assert 2 'a=b=2;'
assert 8 'a= 5 + (b=1+2);'
assert 15 '3 + (a = 6*2);'
assert 170 'a = 5;b = -a + 10;c = +b * -3;d = -(a + b) * (+c - 2);'
assert 1 'x = (((1 + 2) * 3) - 4) / 5;'
assert 6 'a=1;a=a+2;a=a+3;'

assert_error '3+4'
assert_error "3 @ 4;"
assert_error "3.14 + 2;"

assert_error "(3 + 4;"
assert_error "3 + 4)*5;"
assert_error "((3 + 4);"
assert_error "(3 + 4)) / 3;"

assert_error "+;"
assert_error "3 +;"
assert_error "3 * * 4;"
assert_error "3 == == 4;"
assert_error "3 < > 4;"

assert_error ""
assert_error ";"
assert_error "   "
assert_error "   ;"
assert_error "()"
assert_error "();"

if [ "$1" = "1" ]; then
    assert_error "5 / 0"
    assert_error "(1+2*4)/(6-2*3)"
fi

echo OK
