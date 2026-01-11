#include "mycc.h"

// 保存待编译表达式, 用于错误位置显示
char *origin_code;

// 报告错误与错误位置
void error_at_origin(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - origin_code;
    fprintf(stderr, "%s", origin_code);
    if (pos)  fprintf(stderr, "%*s", pos, " "); // 输出 pos 个空格
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
        if (strchr("+-*/()<>", *p)) {
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

// 如果下一个token是期望的符号, 则向前读取一个token并返回真, 否则返回假(依赖全局变量token)
bool consume(Token **token, char *op) {
    if ((*token)->type != TK_PUNCT 
        || strlen(op) != (*token)->len
        || memcmp((*token)->loc, op, (*token)->len))
        return false;
    *token = (*token)->next;
    return true;
}

// 如果下一个token是期望的符号，则向前读取一个token, 否则报告错误(依赖全局变量token)
void expect(Token **token, char *op) {
    if ((*token)->type != TK_PUNCT 
        || strlen(op) != (*token)->len 
        || memcmp((*token)->loc, op, (*token)->len))
        error_at_origin((*token)->loc, "语法错误:应是'%s'但不是", op);
    *token = (*token)->next;
}

// 如果下一个token是数字,则向前读取一个token并返回其数值, 否则报告错误(依赖全局变量token)
int expect_number(Token **token) {
    if ((*token)->type != TK_NUM)
        error_at_origin((*token)->loc, "语法错误:应是数字但不是");
    int val = (*token)->val;
    *token = (*token)->next;
    return val;
}

// 检查是否在末尾token(依赖全局变量token)
bool at_eof(Token *token) {
  return token->type == TK_EOF;
}

// 打印所有token
void print_tokens(Token *token){
    printf("token序列:\n");
    for (Token *cur = token; !at_eof(cur); cur = cur->next)
    {
        if (cur->type == TK_NUM){
            printf("(%d, 数字)\n", cur->val);
        } 
        else if (cur->type == TK_PUNCT) {
            char punct[TMP_STR_SIZE] = {0};
            strncpy(punct, cur->loc, cur->len);
            printf("(\"%s\", 符号)\n", punct);
        }
        else
            error_at_origin(cur->loc, "打印时遇到未知的token类型");
    }
    printf("\n");
}