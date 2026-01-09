#include "mycc.h"

// 命令行参数选项
static bool opt_p; //展示模式
static bool opt_pt; //展示模式, 打印token序列
static bool opt_ps; //展示模式, 打印汇编代码
static bool opt_pq; //展示模式, 打印四元式序列

// 解析命令行参数
static void parse_args(int argc, char **argv) {
    if (argc > 4)
        error("参数数量错误");
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "-p") == 0) {
            opt_p = true;
            if (i + 1 < argc) {
                // 检查-p后内容
                for (int j = 0; j < strlen(argv[i+1]); ++j){
                    if (argv[i+1][j] == 't')
                        opt_pt = true;
                    else if (argv[i+1][j] == 's')
                        opt_ps = true;
                    else if (argv[i+1][j] == 'q')
                        opt_pq = true;
                    else 
                        error("未知的打印选项: %c", argv[i+1][j]);
                }
                ++i;
            } else
                error("-p选项后缺少打印内容选择参数(t/s)");

        } else {
            error("未知的参数");
        }
    }
}

int main(int argc, char **argv) {
    parse_args(argc, argv);

    Token *token = tokenize(read_file(argv[1]));
    if (opt_pt) 
        print_tokens(token);

    // Node *node = parse(&token);
    Quad *quad = parse_to_quads(&token);
    if (opt_pq)
        print_quads(quad);



    if (opt_p && opt_ps)
        printf("汇编代码:\n");
    // if (!opt_p || (opt_p && opt_ps))
    //     codegen(node);

    return 0;
}
