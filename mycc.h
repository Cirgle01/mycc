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

#define TMP_STR_SIZE 100

//
// lexer.c
//

//token 类别
typedef enum {
    TK_PUNCT,  // 符号
    TK_IDENT,  // 变量
    TK_NUM,    // 整数类型
    TK_EOF,    // 终止符号
} TokenType;

// token结构体定义
typedef struct Token {
    TokenType type;      // Token类型
    struct Token *next;  // 指向下一个Token
    int val;             // TK_NUM 时有值
    char *loc;           // token首字符指针(原输入代码中)
    int len;             // token长度
} Token;

void error_at_origin(char *loc, char *fmt, ...);
Token *tokenize(char *p);
bool consume(Token **token, char *op);
Token *consume_ident(Token **token);
void expect(Token **token, char *op);
int expect_number(Token **token);
bool at_eof(Token *token);
void print_tokens(Token *token);

//
// parser.c
//

// 四元式运算符
typedef enum {
    OPR_ADD,    // +
    OPR_SUB,    // -
    OPR_MUL,    // *
    OPR_DIV,    // /
    OPR_EQ,     // ==
    OPR_NE,     // !=
    OPR_LT,     // <
    OPR_LE,     // <=
    OPR_NEG,    // unary -
    OPR_IS,     // unary temp=
    OPR_ASSIGN  // unary local=
} Optor;

// 四元式操作数类型
typedef enum {
    OPD_NUM,   // 数字
    OPD_TEMP,  // 临时变量
    OPD_LOCAL  // 变量
} OpndType;

// 四元式操作数
typedef struct Opnd {
    int val;        // 数值|临时变量编号|局部变量偏移量
    OpndType type;  // 类型
} Opnd;

typedef struct Quad {
    Optor opr;  // 运算符
    Opnd *arg1;  // 操作数1
    Opnd *arg2;  // 操作数2
    Opnd *ret;   // 返回值
    struct Quad *next;  //下一个四元式
} Quad;

// 局部变量结构体
typedef struct LVar {
    struct LVar *next;    // 指向下一个变量或 NULL
    char *name;           // 变量名
    int len;              // 名称长度
    int offset;            // 变量偏移量
} LVar;

Quad *parse_to_quads(Token **token);
void print_quads(Quad *quads);
Quad *optimize_quad(Quad *quad);

//
// codegen.c
//

void codegen(Quad *quad);

//
// util.c
//

void error(char *fmt, ...);
char *read_file(char *path);
int is_alnum(char c);

#endif