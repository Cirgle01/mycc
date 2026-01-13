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

// 从操作数加载值到dst
static void load_to_dest(Opnd *opnd, char* dst) {
    if (opnd == NULL) return;
    
    switch (opnd->type) {
    case OPD_NUM:
        // 立即数：直接mov
        printf("    mov %s, %d\n", dst, opnd->val);
        break;
        
    case OPD_LOCAL:
        // 局部变量：从rbp偏移位置加载
        printf("    mov %s, [rbp - %d]\n", dst, opnd->val);
        break;
        
    case OPD_TEMP:
        // 临时变量：从临时栈中pop
        pop(dst);
        break;
    }
}

// 将rax的值存储到目标操作数
static void store_from_rax(Opnd *dst) {
    if (dst == NULL) return;
    
    switch (dst->type) {
    case OPD_TEMP:
        // 临时变量：push到栈中
        push();
        break;
        
    case OPD_LOCAL:
        // 局部变量：存储到rbp偏移位置
        printf("    mov [rbp - %d], rax\n", dst->val);
        break;
        
    case OPD_NUM:
        // 不能存储到立即数
        error("bug: 不能存储值到立即数");
        break;
    }
}

// 根据四元式生成代码
static Opnd *gen_quad(Quad *quad) {
    Opnd *last_ret_opnd = NULL;
    for (; quad != NULL;quad = quad->next) {
        last_ret_opnd = quad->ret;
        // 单元运算符
        if (quad->arg2 == NULL) {
            // 取值
            load_to_dest(quad->arg1, "rax");

            switch (quad->opr) {
            case OPR_IS:
                assert(quad->next == NULL);
                break;
            case OPR_NEG:
                printf("    neg rax\n");
                break;
            case OPR_ASSIGN:
                if (quad->ret->type != OPD_LOCAL)
                    error("赋值操作的左值不是变量");
                break;
            default:
                error("bug: 生成汇编时遇到未知单元运算符");
            }
            // 存值
            store_from_rax(quad->ret);
            continue;
        }
        // 二元运算符
        // 右操作数加载到rdi
        load_to_dest(quad->arg2, "rdi");
        // 左操作数加载到rax
        load_to_dest(quad->arg1, "rax");

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
        default:
            error("bug: 生成汇编时遇到未知二元运算符");
        }
        store_from_rax(quad->ret);
    }
    return last_ret_opnd;
}

// 生成汇编代码
void codegen(Quad *quad, int local_offset) {
    if (quad == NULL) error("中间代码为空");
    // 汇编开头
    printf(".intel_syntax noprefix\n");
    // 设置不启用可执行堆栈, 防止编译时警告
    printf(".section .note.GNU-stack,\"\",@progbits\n");
    printf(".section .text\n");

    printf(".globl main\n");
    printf("main:\n");

    // 初始化临时变量栈深度
    depth = 0;
    
    if (local_offset > 0) {
     // 序言：设置栈帧，分配局部变量空间
        printf("    push rbp\n");                  // 保存调用者的rbp
        printf("    mov rbp, rsp\n");              // 设置新的栈帧基址
        printf("    sub rsp, %d\n", local_offset);  // 分配局部变量空间
    }

    // 解析四元式序列生成汇编代码
    Opnd *last_opnd =  gen_quad(quad);
    // 如果最后一个四元式结果为临时变量, 将其弹出
    if (last_opnd->type == OPD_TEMP) {
        pop("rax");
    }
    if (depth != 0) error("bug: 生成的汇编代码栈剩余%d", depth);

    if (local_offset > 0) {
        // 尾声：恢复栈帧并返回
        // 最后一个表达式的结果应该在rax中（作为返回值）
        printf("    mov rsp, rbp\n");  // 恢复栈指针（释放局部变量空间）
        printf("    pop rbp\n");       // 恢复调用者的rbp
    }
    printf("    ret\n");
}

