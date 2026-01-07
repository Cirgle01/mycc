#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>

typedef enum {
    TK_RESERVED, // 符号
    TK_NUM,      // 整数类型
    TK_EOF,      // 终止符号
} TokenType;

typedef struct Token {
    TokenType type;      //Token类型
    struct Token *next;  //指向下一个Token
    int val;             //TK_NUM 时有值
    char *str;           //TK_RESERVED 时有值
} Token;

// 错误报告函数, 接受与printf相同的参数
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

// 创建新token并连接到cur后面
Token *new_token(TokenType kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->type = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}


//全局变量token
Token *token;

// 如果下一个Token是期望的符号, 则向前读取一个token并返回真, 否则返回假
bool consume(char op) {
    if (token->type != TK_RESERVED || token->str[0] != op)
        return false;
    token = token->next;
    return true;
}

// 如果下一个token是期望的符号，则向前读取一个令牌, 否则报告错误
void expect(char op) {
    if (token->type != TK_RESERVED || token->str[0] != op)
        error("不是'%c'", op);
    token = token->next;
}

// 如果下一个令牌是数字,则向前读取一个令牌并返回其数值, 否则报告错误
int expect_number() {
    if (token->type != TK_NUM)
        error("不是数字");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
  return token->type == TK_EOF;
}

// 将输入字符串p进行token切分并返回
Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 跳过空白字符
        if (isspace(*p)) {
            p++;
            continue;
        }
        // 是符号, 创建新token
        if (*p == '+' || *p == '-') {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }
        // 是数字, 创建新token
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            // strtol(p, &p, 10):十进制向后读取数字,返回 long
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error("无法识别的字符");
    }
    new_token(TK_EOF, cur, p);
    return head.next;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("参数数量错误");
        return 1;
    }

    //切分token
    token = tokenize(argv[1]);

    // 汇编开头
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // 表达式开头必须是数字，检查这一点并输出第一条mov指令
    printf("  mov rax, %d\n", expect_number());

    // 在消耗'+ <数字>'或'- <数字>'token序列的同时输出汇编代码
    while (!at_eof()) {
        if (consume('+')) {
            printf("    add rax, %d\n", expect_number());
            continue;
        }

        expect('-');
        printf("    sub rax, %d\n", expect_number());
    }

    //汇编返回
    printf("  ret\n");
    return 0;
}
