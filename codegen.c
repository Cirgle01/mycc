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

static void gen_quad(Quad *quad) {
    int last_ret = -1;
    //                   记录上一个四元式结果的临时变量, 移动指针
    for (; quad != NULL;last_ret = quad->ret->val, quad = quad->next) {
        // 单操作数情况
        if (quad->arg2 == NULL) {
            switch (quad->opr) {
            case OPR_IS:
                assert(quad->next == NULL);
                printf("    mov rax, %d\n", quad->arg1->val);
                break;
            case OPR_NEG:
                if (!quad->arg1->istemp)
                    printf("    mov rax, %d\n", quad->arg1->val);
                else
                    assert(quad->arg1->val == last_ret);
                printf("    neg rax\n");
                break;
            }
            continue;
        }

        // 准备操作数
        // 2个临时变量
        if (quad->arg1->istemp && quad->arg2->istemp) {
            if (quad->arg1->val == last_ret) {
                // rax 直接使用
                pop("rdi");
            }
            else {
                assert(quad->arg2->val == last_ret);
                // 移动操作数
                printf("    mov rdi, rax\n");
                pop("rax");
            }
        }
        // 仅arg1为临时变量
        else if (quad->arg1->istemp) {
            assert(quad->arg1->val == last_ret);
            // rax 直接使用
            printf("    mov rdi, %d\n", quad->arg2->val);
        }
        // 仅arg2为临时变量
        else if (quad->arg2->istemp) {
            assert(quad->arg2->val == last_ret);
            // 移动操作数
            printf("    mov rdi, rax\n");
            printf("    mov rax, %d\n", quad->arg1->val);
        }
        // 0个临时变量
        else {
            //  如果不是第一个四元式, 上一个运算结果需要入栈
            if (last_ret != -1) push();
            printf("    mov rax, %d\n", quad->arg1->val);
            printf("    mov rdi, %d\n", quad->arg2->val);
        }
        
        // 运算
        switch (quad->opr) {
        case OPR_ADD: // +
            printf("    add rax, rdi\n");
            break;
        case OPR_SUB: // -
            printf("    sub rax, rdi\n");
            break;
        case OPR_MUL: // *
            printf("    imul rax, rdi\n");
            break;
        case OPR_DIV: // /
            printf("    cqo\n");
            printf("    idiv rdi\n");
            break;
        case OPR_EQ:  // ==
        case OPR_NE:  // !=
        case OPR_LT:  // <
        case OPR_LE:  // <=
            printf("    cmp rax, rdi\n");

            if (quad->opr == OPR_EQ)
                printf("    sete al\n");
            else if (quad->opr == OPR_NE)
                printf("    setne al\n");
            else if (quad->opr == OPR_LT)
                printf("    setl al\n");
            else if (quad->opr == OPR_LE)
                printf("    setle al\n");

            printf("    movzb rax, al\n");
            break;
        }
    }
    
}

// 生成汇编代码
void codegen(Quad *quad) {
    // 汇编开头
    printf(".intel_syntax noprefix\n");
    // 设置不启用可执行堆栈, 防止编译时警告
    printf(".section .note.GNU-stack,\"\",@progbits\n");
    printf(".section .text\n");

    printf(".globl main\n");
    printf("main:\n");

    // 解析四元式序列生成汇编代码
    gen_quad(quad);

    // 汇编 返回
    printf("    ret\n");

    assert(depth == 0);
}