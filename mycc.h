#ifndef MYCC_H
#define MYCC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

#define TMP_STR_SIZE 10

//
// lexer.c
//

//token 类别
typedef enum {
    TK_PUNCT, // 符号
    TK_NUM,      // 整数类型
    TK_EOF,      // 终止符号
} TokenType;

// token结构体定义
typedef struct Token {
    TokenType type;      //Token类型
    struct Token *next;  //指向下一个Token
    int val;             //TK_NUM 时有值
    char *loc;           //token首字符指针(原输入代码中)
    int len;             //token长度
} Token;

void error_at_origin(char *loc, char *fmt, ...);
Token *tokenize(char *p);
bool consume(Token **token, char *op);
void expect(Token **token, char *op);
int expect_number(Token **token);
bool at_eof(Token *token);
void print_tokens(Token *token);

//
// parser.c
//

// 四元式运算符
typedef enum {
    OPR_ADD, // +
    OPR_SUB, // -
    OPR_MUL, // *
    OPR_DIV, // /
    OPR_EQ,  // ==
    OPR_NE,  // !=
    OPR_LT,  // <
    OPR_LE,  // <=
    OPR_NEG, // unary -
    OPR_IS  // unary =
} Optor;

// 四元式操作数
typedef struct Opnd {
    int val;        // 数值
    bool istemp;  // 类型 是否为临时变量
} Opnd;

typedef struct Quad {
    Optor opr;  // 运算符
    Opnd *arg1;  // 操作数1
    Opnd *arg2;  // 操作数2
    Opnd *ret;   // 返回值
    struct Quad *next;  //下一个四元式
} Quad;

Quad *parse_to_quads(Token **token);
void print_quads(Quad *quads);

//
// codegen.c
//

// void codegen(Node *node);

//
// util.c
//

void error(char *fmt, ...);
char *read_file(char *path);

#endif