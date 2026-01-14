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

assert 0 'return 0;'
assert 42 'return 42;'

assert 41 "return 12 + 34 - 5 ;"

assert 47 'return 5+6*7;'
assert 15 'return 5*(9-6);'
assert 4 'return (3+5)/2;'

assert 6 'return -5+11;'
assert 10 'return - - +10;'

assert 0 'return 0==1;'
assert 1 'return 42==42;'
assert 1 'return 0!=1;'
assert 0 'return 42!=42;'

assert 1 'return 0<1;'
assert 0 'return 1<1;'
assert 0 'return 2<1;'
assert 1 'return 0<=1;'
assert 1 'return 1<=1;'
assert 0 'return 2<=1;'

assert 1 'return 1>0;'
assert 0 'return 1>1;'
assert 0 'return 1>2;'
assert 1 'return 1>=0;'
assert 1 'return 1>=1;'
assert 0 'return 1>=2;'

assert 9 "return (20 / 4) + (6 - 2);"
assert 2 "return (5 == 5) + (3 != 4);"
assert 20 "return (10 > 5) * 20 + (3 < 2);"
assert 100 "return (15 <= 15) * 100 + (7 >= 8) * 50;"
assert 11 "return -(-5) + 3 * 2;"
assert 1 "return ((2 + 3) * 4) == (5 * 4);"
assert 4 "return (100 / (5 + 5)) - (3 * 2);"

assert 6 'foo = 1;\nbar = 2 + 3;\nreturn foo + bar;'
assert 6 'foo_ = 1;bar1 = 2 + 3;return foo_ + bar1;'
assert 2 'return a=b=2;'
assert 8 'return a= 5 + (b=1+2);'
assert 15 'return 3 + (a = 6*2);'
assert 170 'a = 5;b = -a + 10;c = +b * -3;return d = -(a + b) * (+c - 2);'
assert 1 'return x = (((1 + 2) * 3) - 4) / 5;'
assert 6 'a=1;a=a+2;return a=a+3;'

assert 0 '3+4+5;'
assert 5 'return 5;3+4+5;'
assert 0 ''

assert_error 'return 3+4'
assert_error "return 3 @ 4;"
assert_error "return 3.14 + 2;"

assert_error "return (3 + 4;"
assert_error "return 3 + 4)*5;"
assert_error "return ((3 + 4);"
assert_error "return (3 + 4)) / 3;"

assert_error "return +;"
assert_error "return 3 +;"
assert_error "return 3 * * 4;"
assert_error "return 3 == == 4;"
assert_error "return 3 < > 4;"

assert_error "return a+5= 5 + (b=1+2);"

assert_error "return "
assert_error "return ;"
assert_error "return    "
assert_error "return    ;"
assert_error "return ()"
assert_error "return ();"

if [ "$1" = "1" ]; then
    assert_error "return 5 / 0;"
    assert_error "return (1+2*4)/(6-2*3);"
fi

echo OK
