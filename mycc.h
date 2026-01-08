#ifndef MYCC_H
#define MYCC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>

//
// lexer.c
//

//token 类别
typedef enum {
    TK_RESERVED, // 符号
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

//
// parser.c
//

// 抽象语法树节点类别
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_NUM, // 整数
} NodeType;

// 抽象语法树节点结构体
typedef struct Node {
    NodeType type;        // 节点类型
    struct Node *lhs;     // 左子节点
    struct Node *rhs;     // 右子节点
    int val;              // 仅当 kind 为 ND_NUM 时使用
} Node;

Node *parse(Token **token);

//
// codegen.c
//

void codegen(Node *node);

//
// util.c
//

void error(char *fmt, ...);
char *read_file(char *path);

#endif