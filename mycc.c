#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>

//token 类型
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
    char *str;           //TK_RESERVED 时有值, 为指向源代码的指针
} Token;

// 抽象语法树节点类型
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // 整数
} NodeType;

// 抽象语法树节点结构体
typedef struct Node {
    NodeType type;        // 节点类型
    struct Node *lhs;     // 左子节点
    struct Node *rhs;     // 右子节点
    int val;              // 仅当 kind 为 ND_NUM 时使用
} Node;

// 保存程序传入的待编译表达式
char *user_input;

//全局变量token
Token *token;

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

// 创建新的语法树(非叶子)节点
Node *new_node(NodeType type, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->type = type;
    node->lhs = lhs;
    node->rhs = rhs;
  return node;
}

// 创建新的语法树叶子节点
Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->type = ND_NUM;
    node->val = val;
  return node;
}

// 可报告错误位置的错误函数(依赖全局变量user_input)
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    if (pos)  fprintf(stderr, "%*s", pos, " "); // 输出 pos 个空格
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    va_end(ap);
    exit(1);
}

// 如果下一个token是期望的符号, 则向前读取一个token并返回真, 否则返回假(依赖全局变量token)
bool consume(char op) {
    if (token->type != TK_RESERVED || token->str[0] != op)
        return false;
    token = token->next;
    return true;
}

// 如果下一个token是期望的符号，则向前读取一个token, 否则报告错误(依赖全局变量token)
void expect(char op) {
    if (token->type != TK_RESERVED || token->str[0] != op)
        error_at(token->str, "不是'%c'", op);
    token = token->next;
}

// 如果下一个token是数字,则向前读取一个token并返回其数值, 否则报告错误(依赖全局变量token)
int expect_number() {
    if (token->type != TK_NUM)
        error_at(token->str, "不是数字");
    int val = token->val;
    token = token->next;
    return val;
}

// 检查是否在末尾token(依赖全局变量token)
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
        }
        // 是符号, 创建新token
        else if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')') {
            cur = new_token(TK_RESERVED, cur, p++);
        }
        // 是数字, 创建新token
        else if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            // strtol(p, &p, 10):十进制向后读取数字,返回 long
            cur->val = strtol(p, &p, 10);
        }
        else{
            error_at(p, "无法识别的字符");
        }
    }
    new_token(TK_EOF, cur, p);
    return head.next;
}

/*
递归下降文法:
expr    = mul ("+" mul | "-" mul)*
mul     = unary ("*" unary | "/" unary)*
unary   = ("+" | "-")? primary
primary = num | "(" expr ")"
*/
// 函数声明
Node *expr();
Node *mul();
Node *primary();
Node *unary();

Node *expr() {
    Node *node = mul();

    for (;;) {
        if (consume('+'))
        node = new_node(ND_ADD, node, mul());
        else if (consume('-'))
        node = new_node(ND_SUB, node, mul());
        else
        return node;
    }
}

Node *mul() {
    Node *node = unary();

    for (;;) {
        if (consume('*'))
        node = new_node(ND_MUL, node, unary());
        else if (consume('/'))
        node = new_node(ND_DIV, node, unary());
        else
        return node;
    }
}

Node *unary() {
  if (consume('+'))
    return primary();
  if (consume('-'))
    return new_node(ND_SUB, new_node_num(0), primary());
  return primary();
}

Node *primary() {
    // 如果有'(',则判断为 '(' expr ')'
    if (consume('(')) {
        Node *node = expr();
        expect(')');
        return node;
    }
    // 否则是整数(叶子节点)
    return new_node_num(expect_number());
}

// 将语法树转换为汇编代码
void gen(Node *node) {
    // 仅单个数字的情况
    if (node->type == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->type) {
    case ND_ADD:
        printf("    add rax, rdi\n");
        break;
    case ND_SUB:
        printf("    sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("    imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("    cqo\n");
        printf("    idiv rdi\n");
        break;
    }

    printf("    push rax\n");
}


int main(int argc, char **argv) {
    if (argc != 2) {
        error("参数数量错误");
        return 1;
    }

    // 存储输入参数,用于错误显示
    user_input = argv[1];
    // 切分token
    token = tokenize(argv[1]);
    // 构建抽象语法树
    Node *node = expr();


    // 汇编开头
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // 根据抽象语法树生成汇编代码
    gen(node);

    // 汇编 出栈
    printf("    pop rax\n");

    // 汇编 返回
    printf("    ret\n");
    return 0;
}
