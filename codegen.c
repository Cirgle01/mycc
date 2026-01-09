#include "mycc.h"

//栈检测变量
static int depth;

//rax入栈
static void push(void) {
  printf("    push rax\n");
  depth++;
}

//出栈
static void pop(char *arg) {
  printf("    pop %s\n", arg);
  depth--;
}

// 将语法树转换为汇编代码
static void gen_expr(Node *node) {
    // 仅单个数字的情况
    switch (node->type){
    case ND_NUM:
        printf("    mov rax, %d\n", node->val);
        return;
    }

    gen_expr(node->rhs);
    // 将右分支入栈
    push();
    gen_expr(node->lhs);
    // 将右分支结果出栈至rdi寄存器
    pop("rdi");

    switch (node->type) {
    case ND_ADD:
        printf("    add rax, rdi\n");
        return;
    case ND_SUB:
        printf("    sub rax, rdi\n");
        return;
    case ND_MUL:
        printf("    imul rax, rdi\n");
        return;
    case ND_DIV:
        printf("    cqo\n");
        printf("    idiv rdi\n");
        return;
    // 比较运算符
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
        printf("    cmp rax, rdi\n");

        if (node->type == ND_EQ)
            printf("    sete al\n");
        else if (node->type == ND_NE)
            printf("    setne al\n");
        else if (node->type == ND_LT)
            printf("    setl al\n");
        else if (node->type == ND_LE)
            printf("    setle al\n");

        printf("    movzb rax, al\n");
        return;
    }
    error("语法树中有非法表达式");
}

// 生成汇编代码
void codegen(Node *node)
{
    // 汇编开头
    printf(".intel_syntax noprefix\n");
    // 设置不启用可执行堆栈, 防止编译时警告
    printf(".section .note.GNU-stack,\"\",@progbits\n");
    printf(".section .text\n");

    printf(".globl main\n");
    printf("main:\n");

    // 解析抽象语法树生成汇编代码
    gen_expr(node);

    // 汇编 返回
    printf("    ret\n");

    assert(depth == 0);
}