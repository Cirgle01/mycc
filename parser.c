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
// 局部变量链表头尾
LVar *locals_head;
LVar *locals_tail;

// 创建新四元式（简化版）
static Quad *new_quad(Optor opr, Opnd arg1, Opnd arg2, Opnd ret) {
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
static Opnd num_opnd(int val) {
    Opnd opd;
    opd.val = val;
    opd.kind = OPD_CONST;
    return opd;
}

// 临时变量操作数
static Opnd temp_opnd(int val) {
    Opnd opd;
    opd.val = val;
    opd.kind = OPD_TEMP;
    return opd;
}

// 变量操作数
static Opnd local_opnd(int val) {
    Opnd opd;
    opd.val = val;
    opd.kind = OPD_LOCAL;
    return opd;
}

// 空操作数
static Opnd null_opnd(void) {
    Opnd opd;
    opd.val = 0;
    opd.kind = OPD_NULL;
    return opd;
}

// 生成新的临时变量
static Opnd new_temp(void) {
    return temp_opnd(temp_counter++);
}

// 按名称查找变量, 如果未找到返回NULL
LVar *find_lvar_name(Token *tok) {
    for (LVar *var = locals_head; var; var = var->next)
        if (var->len == tok->len && !memcmp(tok->loc, var->name, var->len))
            return var;
    return NULL;
}

// 按偏移查找变量, 如果未找到返回NULL
LVar *find_lvar_offset(int offset) {
    for (LVar *var = locals_head; var; var = var->next)
        if (var->offset == offset)  return var;
    return NULL;
}

/* 文法:
program    = stmt*
stmt    = expr ";"  | "return" expr ";"
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
static Opnd program(Token **token);
static Opnd stmt(Token **token);
static Opnd expr(Token **token);
static Opnd assign(Token **token);
static Opnd equality(Token **token);
static Opnd relational(Token **token);
static Opnd add(Token **token);
static Opnd mul(Token **token);
static Opnd unary(Token **token);
static Opnd primary(Token **token);

static Opnd program(Token **token) {
    Opnd res = null_opnd();
    while (!at_eof(*token)) {
        res = stmt(token);
    }
    return res;
}

static Opnd stmt(Token **token) {
    Opnd res;
    if (consume(token, TK_RETURN)) {
        res = expr(token);
        new_quad(OPR_RETURN, null_opnd(), null_opnd(), res);
    } else {
        res = expr(token);
    }
    if (!consume_punct(token, ";")) {
        error_at_origin((*token)->loc-1, "语法错误: 句尾缺少';'");
    }
    return res;
}

static Opnd expr(Token **token) {
    return assign(token);
}

static Opnd assign(Token **token) {
    Opnd res = equality(token);
    Token *last_token = (*token);
    if (consume_punct(token, "=")) {
        if (res.kind != OPD_LOCAL) 
            error_at_origin(last_token->loc, "语法错误: 等号左侧不是变量");
        LVar *assigned = find_lvar_offset(res.val);
        assigned->assign_count++;
        new_quad(OPR_ASSIGN, assign(token), null_opnd(), res);
    }
    return res;
}

static Opnd equality(Token **token) {
    Opnd left = relational(token);
    
    for (;;) {
        if (consume_punct(token, "==")) {
            Opnd right = relational(token);
            Opnd result = new_temp();
            new_quad(OPR_EQ, left, right, result);
            left = result;
        }
        else if (consume_punct(token, "!=")) {
            Opnd right = relational(token);
            Opnd result = new_temp();
            new_quad(OPR_NE, left, right, result);
            left = result;
        }
        else {
            return left;
        }
    }
}

static Opnd relational(Token **token) {
    Opnd left = add(token);
    
    for (;;) {
        if (consume_punct(token, "<")) {
            Opnd right = add(token);
            Opnd result = new_temp();
            new_quad(OPR_LT, left, right, result);
            left = result;
        }
        else if (consume_punct(token, ">")) {
            // a > b 转换为 b < a
            Opnd right = add(token);
            Opnd result = new_temp();
            new_quad(OPR_GT, left, right, result);
            left = result;
        }
        else if (consume_punct(token, "<=")) {
            Opnd right = add(token);
            Opnd result = new_temp();
            new_quad(OPR_LE, left, right, result);
            left = result;
        }
        else if (consume_punct(token, ">=")) {
            // a >= b 转换为 b <= a
            Opnd right = add(token);
            Opnd result = new_temp();
            new_quad(OPR_GE, left, right, result);
            left = result;
        }
        else {
            return left;
        }
    }
}

static Opnd add(Token **token) {
    Opnd left = mul(token);
    
    for (;;) {
        if (consume_punct(token, "+")) {
            Opnd right = mul(token);
            Opnd result = new_temp();
            new_quad(OPR_ADD, left, right, result);
            left = result;
        }
        else if (consume_punct(token, "-")) {
            Opnd right = mul(token);
            Opnd result = new_temp();
            new_quad(OPR_SUB, left, right, result);
            left = result;
        }
        else {
            return left;
        }
    }
}

static Opnd mul(Token **token) {
    Opnd left = unary(token);
    
    for (;;) {
        if (consume_punct(token, "*")) {
            Opnd right = unary(token);
            Opnd result = new_temp();
            new_quad(OPR_MUL, left, right, result);
            left = result;
        }
        else if (consume_punct(token, "/")) {
            Opnd right = unary(token);
            Opnd result = new_temp();
            new_quad(OPR_DIV, left, right, result);
            left = result;
        }
        else {
            return left;
        }
    }
}

static Opnd unary(Token **token) {
    if (consume_punct(token, "+")) {
        return unary(token);  // 正号，直接传递
    }
    if (consume_punct(token, "-")) {
        Opnd operand = unary(token);
        Opnd result = new_temp();
        new_quad(OPR_NEG, operand, null_opnd(), result);
        return result;
    }
    return primary(token);
}

static Opnd primary(Token **token) {
    if (consume_punct(token, "(")) {
        Opnd result = expr(token);
        expect_punct(token, ")");
        return result;
    }
    // 尝试ident
    Token *may_ident = consume_ident(token);
    if (may_ident != NULL) {
        Opnd loc;
        // 查询已有变量
        LVar *lvar = find_lvar_name(may_ident);
        // 已有:获取那个变量编号并创建节点
        if (!lvar) {
            // 没有:创建新变量并入栈;创建节点
            lvar = calloc(1, sizeof(LVar));
            // lvar->next = locals_tail;
            lvar->name = may_ident->loc;
            lvar->len = may_ident->len;
            lvar->next = NULL;
           if (locals_tail) {
                lvar->offset = locals_tail->offset + 8;
                locals_tail->next = lvar;
            }
            else {
                // 链表为空
                lvar->offset = 8;
                locals_head = lvar;
            }
            locals_tail = lvar;
        }
        lvar->appear_count++;
        loc = local_opnd(lvar->offset);
        return loc;
    }
    // 数字字面量
    return num_opnd(expect_number(token));
}

// 主解析函数, 从参数int*返回局部变量总偏移量
Quad *parse_to_quads(Token **token, int *sum_offset) {
    temp_counter = 0;
    quad_head = quad_tail = NULL;
    locals_head = NULL;

    Opnd result = program(token);

    // 检查是否有未赋值就使用的左值
    for (LVar *p = locals_head; p != NULL; p = p->next) {
        if (p->assign_count == 0) {
            char locname[TMP_STR_SIZE] = {0};
            strncpy(locname,p->name, p->len);
            warning("警告: 变量 %s 使用前未定义", locname);
        }
    }
    
    // 记录局部变量总偏移量
    if (locals_head != NULL) {
        (*sum_offset) = locals_tail->offset;
    }
    else {
        (*sum_offset) = 0;
    }

    return quad_head;
}

// 将运算符枚举转换为字符串
static const char *opr_to_str(Optor opr) {
    switch (opr) {
    case OPR_ADD:     return "+";
    case OPR_SUB:     return "-";
    case OPR_MUL:     return "*";
    case OPR_DIV:     return "/";
    case OPR_EQ:      return "==";
    case OPR_NE:      return "!=";
    case OPR_LT:      return "<";
    case OPR_LE:      return "<=";
    case OPR_GT:      return ">";
    case OPR_GE:      return ">=";
    case OPR_NEG:     return "neg";
    case OPR_RETURN:  return "ret";
    case OPR_ASSIGN:  return ":=";
    default: return "???";
    }
}

// 输出单个操作数的函数
static void print_opnd(Opnd opnd) {
    if (opnd.kind == OPD_NULL) {
        printf("_");
        return;
    }
    switch (opnd.kind)
    {
    case OPD_CONST:
        printf("%d", opnd.val);
        return;
    case OPD_TEMP:
        printf("t%d", opnd.val);
        return;
    case OPD_LOCAL:
        printf("a%d", opnd.val);
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
        
        if (q->arg1.kind == OPD_NULL) {
            // arg1为NULL
            printf("_");
        } else {
            print_opnd(q->arg1);
        }

        printf(", ");
        
        if (q->arg2.kind == OPD_NULL) {
            // arg2为NULL
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

void print_synbl() {
    if (locals_head != NULL) {
        //局部变量显示
        printf("\n符号表:\n");
        printf("名称\t表示\t类型\t地址\t赋值\t使用\n");
        for (LVar* p = locals_head; p != NULL; p = p->next) {
            char locname[TMP_STR_SIZE] = {0};
            strncpy(locname,p->name, p->len);
            printf("%s\t", locname);
            printf("a%d\t", p->offset);
            printf("整数\t");
            printf("rbp-%d\t", p->offset);
            printf("%d\t%d\n", p->assign_count, (p->appear_count) - (p->assign_count));
        }
    } else {
        printf("无局部变量\n");
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
    case OPR_GT:  return num1 > num2;
    case OPR_GE:  return num1 >= num2;
    case OPR_NEG: return -num1;
    case OPR_RETURN:  return num1;
    case OPR_ASSIGN: return num1;
    default: error("bug: 优化四元式时遇到未知运算符");
    }
}

static int same_opnd(Opnd opnd1, Opnd opnd2) {
    return (opnd1.kind == opnd2.kind) && (opnd1.val == opnd2.val);
}

// 优化四元式
Quad *optimize_quad(int *sum_offset) {
    //常量折叠
    for (Quad *p = quad_head, *last_p = NULL; p != NULL; last_p = p, p = p->next) {
        if (p->arg1.kind == OPD_CONST) {
            Opnd const_opnd;
            if (p->arg2.kind == OPD_NULL) {
                // 单元运算
                const_opnd = num_opnd(compute_opr(p->opr, p->arg1.val, 0));
            } else if (p->arg2.kind == OPD_CONST) {
                // 2元运算
                const_opnd = num_opnd(compute_opr(p->opr, p->arg1.val, p->arg2.val));
            }
            else continue; //不满足常量折叠条件, 跳出循环

            // 替换后面的该变量, 直至末尾或再次赋值
            Quad *sp;
            for (sp = p->next; sp != NULL && (sp->arg1.kind == OPD_NULL || !same_opnd(sp->ret, p->ret)); sp = sp->next)
            {
                if (same_opnd(sp->arg1, p->ret))
                    sp->arg1 = const_opnd;
                if (same_opnd(sp->arg2, p->ret))
                    sp->arg2 = const_opnd;
                if (same_opnd(sp->ret, p->ret))
                    sp->ret = const_opnd;
            }

            // 移除当前四元式
            if (p == quad_head) {
                // 是第一个四元式
                quad_head = p->next;
            } else  {
                last_p->next = p->next;
            }

            // 临时变量空间整理
            if (p->ret.kind == OPD_LOCAL && sp == NULL) {
                (*sum_offset) -= 8;
                for (Quad *sp = p->next; sp != NULL ; sp = sp->next) {
                    if (sp->arg1.kind == OPD_LOCAL && (sp->arg1.val) > (p->ret.val)) {
                        sp->arg1.val -= 8;
                    }
                    if (sp->arg2.kind == OPD_LOCAL && (sp->arg2.val) > (p->ret.val)) {
                        sp->arg2.val -= 8;
                    }
                    if (sp->ret.kind == OPD_LOCAL && (sp->ret.val) > (p->ret.val)) {
                        sp->ret.val -= 8;
                    }
                }
            }
        }
    }
    return quad_head;
}
