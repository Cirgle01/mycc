#include "mycc.h"


//
// 四元式创建部分
//

// 全局变量
// 四元式链表头尾
static Quad *quad_head;
static Quad *quad_tail;
// 临时变量计数器
static int temp_counter;
// 局部变量链表
LVar *locals;

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
    opd->type = OPD_NUM;
    return opd;
}

// 临时变量操作数
static Opnd *temp_opnd(int val) {
    Opnd *opd = calloc(1, sizeof(Opnd));
    opd->val = val;
    opd->type = OPD_TEMP;
    return opd;
}

// 变量操作数
static Opnd *local_opnd(int val) {
    Opnd *opd = calloc(1, sizeof(Opnd));
    opd->val = val;
    opd->type = OPD_LOCAL;
    return opd;
}

// 生成新的临时变量
static Opnd *new_temp(void) {
    return temp_opnd(temp_counter++);
}

// 按名称查找变量, 如果未找到返回NULL
LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->loc, var->name, var->len))
      return var;
  return NULL;
}

/* 文法:
expr       = equality
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")unary | primary
primary    = num | "(" expr ")"

program    = stmt*
stmt       = expr ";"
expr       = assign
assign     = equality ("=" assign)?
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary
primary    = num | ident | "(" expr ")"
*/

// 递归下降解析函数（返回操作数）
static Opnd *program(Token **token);
static Opnd *stmt(Token **token);
static Opnd *expr(Token **token);
static Opnd *assign(Token **token);
static Opnd *equality(Token **token);
static Opnd *relational(Token **token);
static Opnd *add(Token **token);
static Opnd *mul(Token **token);
static Opnd *unary(Token **token);
static Opnd *primary(Token **token);

static Opnd *program(Token **token) {
    Opnd *res = NULL;
    while (!at_eof(*token)) {
        res = stmt(token);
    }
    if (res == NULL) {
        error("语法错误: token序列为空, 即源代码为空");
    }
    return res;
}

static Opnd *stmt(Token **token) {
    Opnd *res = expr(token);
    if (!consume(token, ";")) {
        error_at_origin((*token)->loc-1, "句尾缺少';'");
    }
    return res;
}

static Opnd *expr(Token **token) {
    return assign(token);
}

static Opnd *assign(Token **token) {
    Opnd *res = equality(token);
    // if (res->type != OPD_LOCAL) 
    //     error_at_origin((*token)->loc, "语法错误: 等号左侧不是变量");
    if (consume(token, "=")) {
        new_quad(OPR_ASSIGN, assign(token), NULL, res);
    }
    return res;
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
    // 尝试ident
    Token *may_ident = consume_ident(token);
    if (may_ident != NULL) {
        Opnd *loc;
        // 查询已有变量
        LVar *lvar = find_lvar(may_ident);
        // 已有:获取那个变量编号并创建节点
        if (lvar) {
            loc = local_opnd(lvar->offset);
        }
        else {
            // 没有:创建新变量并入栈;创建节点
            lvar = calloc(1, sizeof(LVar));
            lvar->next = locals;
            lvar->name = may_ident->loc;
            lvar->len = may_ident->len;
            lvar->offset = locals ? locals->offset + 8 : 8;
            loc = local_opnd(lvar->offset);
            locals = lvar;
        }
        return loc;
    }
    // 数字字面量
    return num_opnd(expect_number(token));
}

// 主解析函数, 从参数int*返回局部变量总偏移量
Quad *parse_to_quads(Token **token, int *sum_offset) {
    temp_counter = 0;
    quad_head = quad_tail = NULL;
    locals = NULL;

    Opnd *result = program(token);
    // 似乎使用program()后, 当前token必定是EOF
    if ((*token)->type != TK_EOF)
        error("bug: parse_to_quads()未处理额外token");

    // 如果返回数字常量, 为最终结果生成一个赋值四元式
    if (result->type == OPD_NUM) {
        Opnd *final_temp = new_temp();
        new_quad(OPR_IS, result, NULL, final_temp);
        result = final_temp;
    }
    
    // 记录局部变量总偏移量
    if (locals != NULL) {
        (*sum_offset) = locals->offset;
    }
    else {
        (*sum_offset) = 0;
    }

    return quad_head;
}

// 将运算符枚举转换为字符串
static const char *opr_to_str(Optor opr) {
    switch (opr) {
    case OPR_ADD:    return "+";
    case OPR_SUB:    return "-";
    case OPR_MUL:    return "*";
    case OPR_DIV:    return "/";
    case OPR_EQ:     return "==";
    case OPR_NE:     return "!=";
    case OPR_LT:     return "<";
    case OPR_LE:     return "<=";
    case OPR_NEG:    return "neg";
    case OPR_IS:     return "=";
    case OPR_ASSIGN: return ":=";
    default: return "???";
    }
}

// 输出单个操作数的函数
static void print_opnd(Opnd *opnd) {
    if (opnd == NULL) {
        printf("_");
        return;
    }
    switch (opnd->type)
    {
    case OPD_NUM:
        printf("%d", opnd->val);
        return;
    case OPD_TEMP:
        printf("t%d", opnd->val);
        return;
    case OPD_LOCAL:
        printf("a%d", opnd->val);
        return;
    default:
        printf("???");
    }
}

// 输出四元式序列
void print_quads(Quad *quads) {
    printf("四元式序列:\n");
    
    for (Quad *q = quads; q != NULL; q = q->next) {
        printf("(%-4s, ", opr_to_str(q->opr));
        
        print_opnd(q->arg1);
        printf(", ");
        
        // 一元运算符的arg2为NULL
        if (q->arg2 == NULL) {
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

static int compute_opr(Optor opr, int num1, int num2) {
    switch (opr) {
    case OPR_ADD: return num1 + num2;
    case OPR_SUB: return num1 - num2;
    case OPR_MUL: return num1 * num2;
    case OPR_DIV: 
        if (num2 == 0)
            error("优化时遇到除0错误");
        return num1 / num2;
    case OPR_EQ:  return num1 == num2;
    case OPR_NE:  return num1 != num2;
    case OPR_LT:  return num1 < num2;
    case OPR_LE:  return num1 <= num2;
    case OPR_NEG: return -num1;
    case OPR_IS:  return num1;
    case OPR_ASSIGN: return num1;
    default: error("bug: 优化四元式时遇到未知运算符");
    }
}

static int same_opnd(Opnd *opnd1, Opnd *opnd2) {
    return (opnd1->type == opnd2->type) && (opnd1->val == opnd2->val);
}

Quad *optimize_quad(Quad *quad) {
    // 逐条优化四元式(最终仅剩一个结果)
    Opnd *ret;
    for ( ; quad != NULL; quad = quad->next)
    {
        Opnd *replace_opnd = quad->ret;
        Opnd *const_opnd;
        if (quad->arg1->type != OPD_NUM) error("优化时遇到未赋值变量使用");

        if (quad->arg2 == NULL) {
            //单元运算
            const_opnd = num_opnd(compute_opr(quad->opr, quad->arg1->val, 0));
        }
        else{
            //2元运算
            if (quad->arg2->type != OPD_NUM) error("优化时遇到未赋值变量使用");
            const_opnd = num_opnd(compute_opr(quad->opr, quad->arg1->val, quad->arg2->val));
        }

        // 替换后面的该变量
        for (Quad *sp = quad->next; sp != NULL && !same_opnd(sp->ret, replace_opnd); sp = sp->next)
        {
            if (sp->arg1 != NULL && same_opnd(sp->arg1, replace_opnd))
                sp->arg1 = const_opnd;
            if (sp->arg2 != NULL && same_opnd(sp->arg2, replace_opnd))
                sp->arg2 = const_opnd;
        }
        ret = const_opnd;
    }

    // 生成最终赋值四元式
    temp_counter = 0;
    quad_head = quad_tail = NULL;
    new_quad(OPR_IS, ret, NULL, new_temp());
    
    return quad_head;
}