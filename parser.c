#include "mycc.h"


//
// 四元式创建部分
//

// 全局变量：四元式链表和临时变量计数器
static Quad *quad_head = NULL;
static Quad *quad_tail = NULL;
static int temp_counter = 0;

// 创建新四元式（简化版）
static Quad *new_quad(Optor opr, Opnd *arg1, Opnd *arg2, Opnd *ret) {
    Quad *quad = calloc(1, sizeof(Quad));
    quad->opr = opr;
    quad->arg1 = arg1;
    quad->arg2 = arg2;
    quad->ret = ret;
    quad->next = NULL;
    
    // 全局四元式头尾设置
    if (quad_tail) quad_tail->next = quad;
    else quad_head = quad;
    quad_tail = quad;
    
    return quad;
}

// 数字操作数
static Opnd *num_opnd(int val) {
    Opnd *opd = calloc(1, sizeof(Opnd));
    opd->val = val;
    opd->istemp = 0;
    return opd;
}

//临时变量操作数
static Opnd *temp_opnd(int val) {
    Opnd *opd = calloc(1, sizeof(Opnd));
    opd->val = val;
    opd->istemp = 1;
    return opd;
}

/* 文法:
expr       = equality
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")unary | primary
primary    = num | "(" expr ")"
*/

// 生成新的临时变量
static Opnd *new_temp(void) {
    return temp_opnd(temp_counter++);
}

// 递归下降解析函数（返回操作数）
static Opnd *expr(Token **token);
static Opnd *equality(Token **token);
static Opnd *relational(Token **token);
static Opnd *add(Token **token);
static Opnd *mul(Token **token);
static Opnd *primary(Token **token);
static Opnd *unary(Token **token);

static Opnd *expr(Token **token) {
    return equality(token);
}

static Opnd *equality(Token **token) {
    Opnd *left = relational(token);
    
    for (;;) {
        if (consume(token, "==")) {
            Opnd *right = relational(token);
            Opnd *result = new_temp();
            new_quad(OPR_EQ, left, right, result);
            left = result;
        }
        else if (consume(token, "!=")) {
            Opnd *right = relational(token);
            Opnd *result = new_temp();
            new_quad(OPR_NE, left, right, result);
            left = result;
        }
        else {
            return left;
        }
    }
}

static Opnd *relational(Token **token) {
    Opnd *left = add(token);
    
    for (;;) {
        if (consume(token, "<")) {
            Opnd *right = add(token);
            Opnd *result = new_temp();
            new_quad(OPR_LT, left, right, result);
            left = result;
        }
        else if (consume(token, ">")) {
            // a > b 转换为 b < a
            Opnd *right = add(token);
            Opnd *result = new_temp();
            new_quad(OPR_LT, right, left, result);
            left = result;
        }
        else if (consume(token, "<=")) {
            Opnd *right = add(token);
            Opnd *result = new_temp();
            new_quad(OPR_LE, left, right, result);
            left = result;
        }
        else if (consume(token, ">=")) {
            // a >= b 转换为 b <= a
            Opnd *right = add(token);
            Opnd *result = new_temp();
            new_quad(OPR_LE, right, left, result);
            left = result;
        }
        else {
            return left;
        }
    }
}

static Opnd *add(Token **token) {
    Opnd *left = mul(token);
    
    for (;;) {
        if (consume(token, "+")) {
            Opnd *right = mul(token);
            Opnd *result = new_temp();
            new_quad(OPR_ADD, left, right, result);
            left = result;
        }
        else if (consume(token, "-")) {
            Opnd *right = mul(token);
            Opnd *result = new_temp();
            new_quad(OPR_SUB, left, right, result);
            left = result;
        }
        else {
            return left;
        }
    }
}

static Opnd *mul(Token **token) {
    Opnd *left = unary(token);
    
    for (;;) {
        if (consume(token, "*")) {
            Opnd *right = unary(token);
            Opnd *result = new_temp();
            new_quad(OPR_MUL, left, right, result);
            left = result;
        }
        else if (consume(token, "/")) {
            Opnd *right = unary(token);
            Opnd *result = new_temp();
            new_quad(OPR_DIV, left, right, result);
            left = result;
        }
        else {
            return left;
        }
    }
}

static Opnd *unary(Token **token) {
    if (consume(token, "+")) {
        return unary(token);  // 正号，直接传递
    }
    if (consume(token, "-")) {
        Opnd *operand = unary(token);
        Opnd *result = new_temp();
        new_quad(OPR_NEG, operand, NULL, result);
        return result;
    }
    return primary(token);
}

static Opnd *primary(Token **token) {
    if (consume(token, "(")) {
        Opnd *result = expr(token);
        expect(token, ")");
        return result;
    }
    // 数字字面量
    return num_opnd(expect_number(token));
}

// 主解析函数
Quad *parse_to_quads(Token **token) {
    temp_counter = 0;
    quad_head = quad_tail = NULL;
    
    Opnd *result = expr(token);
    if ((*token)->type != TK_EOF)
        error_at_origin((*token)->loc, "语法错误:意外的token");
    
    // 如果返回数字常量, 为最终结果生成一个赋值四元式
    if (!result->istemp) {
        Opnd *final_temp = new_temp();
        new_quad(OPR_IS, result, NULL, final_temp);
        result = final_temp;
    }
    
    return quad_head;
}

// 将运算符枚举转换为字符串
static const char *opr_to_str(Optor opr) {
    switch (opr) {
    case OPR_ADD: return "+";
    case OPR_SUB: return "-";
    case OPR_MUL: return "*";
    case OPR_DIV: return "/";
    case OPR_EQ:  return "==";
    case OPR_NE:  return "!=";
    case OPR_LT:  return "<";
    case OPR_LE:  return "<=";
    case OPR_NEG: return "neg";
    case OPR_IS:  return "=";
    default: return "???";
    }
}

// 输出单个操作数的函数
static void print_opnd(Opnd *opnd) {
    if (opnd == NULL) {
        printf("_");
        return;
    }
    
    if (opnd->istemp) {
        printf("t%d", opnd->val);
    } else {
        printf("%d", opnd->val);
    }
}

// 输出四元式序列到文件（或标准输出）
void print_quads(Quad *quads) {
    printf("四元式序列:\n");
    
    for (Quad *q = quads; q != NULL; q = q->next) {
        printf("(%-4s, ", opr_to_str(q->opr));
        
        print_opnd(q->arg1);
        printf(", ");
        
        // 一元运算符的arg2为NULL
        if (q->opr == OPR_NEG) {
            printf("_");
        } else {
            print_opnd(q->arg2);
        }
        
        printf(", ");
        print_opnd(q->ret);
        printf(")\n");
    }
    printf("\n");
}