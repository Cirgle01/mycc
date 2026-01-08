#include "mycc.h"

int main(int argc, char **argv) {
    if (argc != 2)
        error("参数数量错误");

    // 切分token
    Token *token = tokenize(read_file(argv[1]));
    // 构建抽象语法树
    Node *node = parse(&token);
    // 代码生成
    codegen(node);
    
    return 0;
}
