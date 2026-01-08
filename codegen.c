#include "mycc.h"

// 将语法树转换为汇编代码
static void gen_expr(Node *node) {
    // 仅单个数字的情况
    if (node->type == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }

    gen_expr(node->lhs);
    gen_expr(node->rhs);

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
    case ND_EQ:
        printf("    cmp rax, rdi\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_NE:
        printf("    cmp rax, rdi\n");
        printf("    setne al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LT:
        printf("    cmp rax, rdi\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LE:
        printf("    cmp rax, rdi\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
        break;
    }
    printf("    push rax\n");
}

void codegen(Node *node)
{
    // 汇编开头
    printf(".intel_syntax noprefix\n");
    // 设置不启用可执行堆栈, 防止编译时警告
    printf(".section .note.GNU-stack,\"\",@progbits\n");
    printf(".section .text\n");
    printf(".globl main\n");
    printf("main:\n");

    // 根据抽象语法树生成汇编代码
    gen_expr(node);

    // 汇编 出栈
    printf("    pop rax\n");

    // 汇编 返回
    printf("    ret\n");
}