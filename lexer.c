#include "mycc.h"

// 保存待编译表达式, 用于错误位置显示
char *origin_code;

// 报告错误与错误位置
void error_at_origin(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    // 查找问题代码所在行
    char *line_start = origin_code;
    char *last_line_start = NULL;
    int line_counter = 1;
    for (char *sp = origin_code; sp < loc ; ++sp) {
        if (*sp == '\n') {
            last_line_start = line_start;
            line_start = sp + 1;
            ++line_counter;
        }
    }

    // 对loc指向'\0'(即token为TK_EOF)进行修正
    if (*loc == '\0') {
        line_start = last_line_start;
        line_counter--;
    }

    int pos = loc - line_start;
    fprintf(stderr, "c%d: ", line_counter);
    // 输出单行内容至stderr
    for (char* p = line_start; *p != '\n'; ++p) {
        putc(*p, stderr);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "%*s", pos+4, " "); // 输出 pos 个空格
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    va_end(ap);
    exit(1);
}

//
// token生成
//

// 创建新token并连接到cur后面
static Token *new_token(TokenType kind, Token *cur, char *loc, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->type = kind;
    tok->loc = loc;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// 检查多字符运算符
static bool startswith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

// 将输入字符串p进行token切分并返回
Token *tokenize(char *p) {
    // 设置全局变量
    origin_code = p;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 跳过空白字符
        if (isspace(*p)) {
            p++;
            continue;
        }
        // 是字母, 创建变量类型token
        if (isalpha(*p)) {
            if (startswith(p, "return") && !is_alnum(p[6])) {
                // 是 return 
                cur = new_token(TK_RETURN, cur, p, 6);
                p += 6;
                continue;
            } else {
                // 是 变量名
                cur = new_token(TK_IDENT, cur, p, 0);
                char *q = p;
                // 移动指针至变量名后一个字符
                while(is_alnum(*++p));
                cur->len = p - q;
                continue;
            }
        }
        // 是数字, 创建新token
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            // strtol(p, &p, 10):十进制向后读取数字,返回 long
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }
        // 是双字符符号, 创建新token
        if (startswith(p, "==") || startswith(p, "!=")
            || startswith(p, "<=") || startswith(p, ">=")) {
            cur = new_token(TK_PUNCT, cur, p, 2);
            p += 2;
            continue;
        }
        // 是单字符符号, 创建新token
        if (strchr("+-*/()<>=;", *p)) {
            cur = new_token(TK_PUNCT, cur, p++, 1);
            continue;
        }
        
        error_at_origin(p, "无法识别的字符");
    }
    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

//
// token 操作函数
//

// 如果下一个token是期望的token类型, 则向前读取一个token并返回真, 否则返回假
bool consume(Token **token, TokenType type) {
    if ((*token)->type != type)
        return false;
    *token = (*token)->next;
    return true;
}
// 如果下一个token是期望的符号, 则向前读取一个token并返回真, 否则返回假
bool consume_punct(Token **token, char *op) {
    if ((*token)->type != TK_PUNCT 
        || strlen(op) != (*token)->len
        || memcmp((*token)->loc, op, (*token)->len))
        return false;
    *token = (*token)->next;
    return true;
}

// 如果下一个token是变量, 则向前读取一个token并返回之前的变量token, 否则返回NULL
Token *consume_ident(Token **token) {
    if ((*token)->type != TK_IDENT)
        return NULL;
    Token *ident = (*token);
    *token = (*token)->next;
    return ident;
}

// 如果下一个token是期望的符号，则向前读取一个token, 否则报告错误
void expect_punct(Token **token, char *op) {
    if ((*token)->type != TK_PUNCT 
        || strlen(op) != (*token)->len 
        || memcmp((*token)->loc, op, (*token)->len))
        error_at_origin((*token)->loc, "语法错误:应是'%s'但不是", op);
    *token = (*token)->next;
}

// 如果下一个token是数字,则向前读取一个token并返回其数值, 否则报告错误
int expect_number(Token **token) {
    if ((*token)->type != TK_NUM)
        error_at_origin((*token)->loc, "语法错误:应是数字但不是");
    int val = (*token)->val;
    *token = (*token)->next;
    return val;
}

// 检查是否在末尾token
bool at_eof(Token *token) {
  return token->type == TK_EOF;
}

static void print_token_type(Token *token) {
    switch (token->type) {
    case TK_NUM:
        printf("数字");
        break;
    case TK_IDENT:
        printf("变量");
        break;
    case TK_PUNCT:
        printf("符号");
        break;
    case TK_RETURN:
        printf("RETURN");
        break;
    default:
        error("bug: 打印token时遇到未知类型");
    }
}

// 打印所有token
void print_tokens(Token *token){
    printf("token序列:\n");
    for (Token *cur = token; !at_eof(cur); cur = cur->next)
    {
        printf("(");
        if (cur->type == TK_NUM){
            printf("%d", cur->val);
        } else {
            printf("\"");
            for (int i = 0; i < cur->len; ++i) {
                printf("%c", *(cur->loc + i));
            }
            printf("\"");
        }
        printf(", ");
        print_token_type(cur);
        printf(")\n");
    }
    printf("\n");
}