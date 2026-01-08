#include "mycc.h"

static Node *expr(Token **token);
static Node *equality(Token **token);
static Node *relational(Token **token);
static Node *add(Token **token);
static Node *mul(Token **token);
static Node *primary(Token **token);
static Node *unary(Token **token);

//
// 语法树构建部分
//

// 创建新的语法树(非叶子)节点
static Node *new_node(NodeType type, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->type = type;
    node->lhs = lhs;
    node->rhs = rhs;
  return node;
}

// 创建新的语法树叶子节点
static Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->type = ND_NUM;
    node->val = val;
  return node;
}

/* 文法:
expr       = equality
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary
primary    = num | "(" expr ")"
*/

static Node *expr(Token **token){
    return equality(token);
}

static Node *equality(Token **token){
    Node *node = relational(token);
    if (consume(token, "=="))
        node = new_node(ND_EQ, node, relational(token));
    else if (consume(token, "!="))
        node = new_node(ND_NE, node, relational(token));
    else
        return node;
}

// > >= 用左右互换的 < <= 表示
static Node *relational(Token **token){
    Node *node = add(token);

    for (;;) {
        if (consume(token, "<"))
            node = new_node(ND_LT, node, add(token));
        else if (consume(token, ">"))
            node = new_node(ND_LT, add(token), node);
        else if (consume(token, "<="))
            node = new_node(ND_LE, node, add(token));
        else if (consume(token, ">="))
            node = new_node(ND_LE, add(token), node);
        else
            return node;
    }
}

static Node *add(Token **token) {
    Node *node = mul(token);

    for (;;) {
        if (consume(token, "+"))
            node = new_node(ND_ADD, node, mul(token));
        else if (consume(token, "-"))
            node = new_node(ND_SUB, node, mul(token));
        else
            return node;
    }
}

static Node *mul(Token **token) {
    Node *node = unary(token);

    for (;;) {
        if (consume(token, "*"))
            node = new_node(ND_MUL, node, unary(token));
        else if (consume(token, "/"))
            node = new_node(ND_DIV, node, unary(token));
        else
        return node;
    }
}

static Node *unary(Token **token) {
    if (consume(token, "+"))
        return unary(token);
    if (consume(token, "-"))
        return new_node(ND_SUB, new_node_num(0), unary(token));
    return primary(token);
}

static Node *primary(Token **token) {
    // 如果有'(',则判断为 '(' expr ')'
    if (consume(token, "(")) {
        Node *node = expr(token);
        expect(token, ")");
        return node;
    }
    // 否则是整数(叶子节点)
    return new_node_num(expect_number(token));
}

// 构建抽象语法树
Node *parse(Token **token){
  Node *node = expr(token);
  return node;
}