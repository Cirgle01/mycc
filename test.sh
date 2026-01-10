#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    echo "$input" > tmp.txt
    ./mycc tmp.txt > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input --> $actual"
    else
        echo "$input --> $expected expected, but got $actual"
        exit 1
    fi
}

assert_error() {
    input="$1"

    echo "$input" > tmp.txt
    ./mycc tmp.txt > tmp.s

    if [ $? -ne 0 ]; then
        echo "find error in $input"
    else
        echo "not find error in $input"
        exit 1
    fi
}
# assert 0 0
# assert 42 42

# assert 41 " 12 + 34 - 5 "

# assert 47 '5+6*7'
# assert 15 '5*(9-6)'
# assert 4 '(3+5)/2'

# assert 6 '-5+11'
# assert 10 '- - +10'

# assert 0 '0==1'
# assert 1 '42==42'
# assert 1 '0!=1'
# assert 0 '42!=42'

# assert 1 '0<1'
# assert 0 '1<1'
# assert 0 '2<1'
# assert 1 '0<=1'
# assert 1 '1<=1'
# assert 0 '2<=1'

# assert 1 '1>0'
# assert 0 '1>1'
# assert 0 '1>2'
# assert 1 '1>=0'
# assert 1 '1>=1'
# assert 0 '1>=2'

# assert 9 "(20 / 4) + (6 - 2)"
# assert 2 "(5 == 5) + (3 != 4)"
# assert 20 "(10 > 5) * 20 + (3 < 2)"
# assert 100 "(15 <= 15) * 100 + (7 >= 8) * 50"
# assert 11 "-(-5) + 3 * 2"
# assert 1 "((2 + 3) * 4) == (5 * 4)"
# assert 4 "(100 / (5 + 5)) - (3 * 2)"

assert_error "3 @ 4"
assert_error "abc + 3"
assert_error "3.14 + 2"
assert_error "3 + 4;"

assert_error "(3 + 4"
assert_error "3 + 4)*5"
assert_error "((3 + 4)"
assert_error "(3 + 4)) / 3"

assert_error "+"
assert_error "3 +"
assert_error "3 * * 4"
assert_error "3 == == 4"
assert_error "3 < > 4"

assert_error ""
assert_error "   "
assert_error "()"

echo OK
