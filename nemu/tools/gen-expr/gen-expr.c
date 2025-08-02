/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>


// this should be enough
static char buf[65536] = {0};
static char code_buf[65536 + 128] = {};  // a little larger than `buf`
static char* code_format =
    "#include <stdio.h>\n"
    "int main() { "
    "  int result = %s;"
    "  printf(\"%%u\", result); "
    "  return 0; "
    "}\n";

int pos = 0;
int choose(int n) {
    if (n < 0) {
        return -1;
    }
    return rand() % n;
}
void gen_num() {
    buf[pos++] = choose(5) + '3';  // 3~7
}
void gen(char c) {
    buf[pos++] = c;
    if (choose(2) == 1) {  // 有一半的概率插入空格
        buf[pos++] = ' ';
    }
}
void gen_rand_op() {
    int n = choose(7);
    switch (n) {
        case 0:
            buf[pos++] = '+';
            break;
        case 1:
            buf[pos++] = '-';
            break;
        case 2:
            buf[pos++] = '*';
            break;
        case 3:
            buf[pos++] = '=';
            buf[pos++] = '=';
            break;
        case 4:
            buf[pos++] = '!';
            buf[pos++] = '=';
            break;
        case 5:
            buf[pos++] = '<';
            buf[pos++] = '=';
            break;
        case 6:
            buf[pos++] = '>';
            buf[pos++] = '=';
            break;
        default:
            buf[pos++] = '/';
    }
    if (choose(2) == 1) {  // 有一半的概率插入空格
        buf[pos++] = ' ';
    }
}

static void gen_rand_expr(int n) {
    if (n == 0) {
        gen_num();
        return;
    }
    switch (choose(3)) {
        case 0:
            gen_num();
            break;
        case 1:
            gen('(');
            gen_rand_expr(n - 1);
            gen(')');
            break;
        default:
            gen_rand_expr(n - 1);
            gen_rand_op();
            gen_rand_expr(n - 1);
            break;
    }
}

int main(int argc, char* argv[]) {
    int seed = time(0);
    srand(seed);
    int loop = 1;
    if (argc > 1) {
        sscanf(argv[1], "%d", &loop);
    }
    int i;
    for (i = 0; i < loop; i++) {
        pos = 0;
        memset(buf, 0, strlen(buf));
        gen_rand_expr(10);

        sprintf(code_buf, code_format, buf);

        FILE* fp = fopen("/tmp/.code.c", "w");
        assert(fp != NULL);
        fputs(code_buf, fp);
        fclose(fp);

        int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
        if (ret != 0)
            continue;

        fp = popen("/tmp/.expr", "r");
        assert(fp != NULL);

        int result;
        ret = fscanf(fp, "%d", &result);
        if (ret != 1) {      // 读取失败，除0 行为
            result = 10086;  // 10086 定义为 除0行为
        }
        pclose(fp);

        printf("%u %s\n", result, buf);
    }
    return 0;
}


// // this should be enough
// static char buf[65536] = {};
// static char code_buf[65536 + 128] = {}; // a little larger than `buf`
// static char *code_format =
// "#include <stdio.h>\n"
// "int main() { "
// "  unsigned result = %s; "
// "  printf(\"%%u\", result); "
// "  return 0; "
// "}";

// static void gen_rand_expr() {
//   buf[0] = '\0';
// }

// int main(int argc, char *argv[]) {
//   int seed = time(0);
//   srand(seed);
//   int loop = 1;
//   if (argc > 1) {
//     sscanf(argv[1], "%d", &loop);
//   }
//   int i;
//   for (i = 0; i < loop; i ++) {
//     gen_rand_expr();

//     sprintf(code_buf, code_format, buf);

//     FILE *fp = fopen("/tmp/.code.c", "w");
//     assert(fp != NULL);
//     fputs(code_buf, fp);
//     fclose(fp);

//     int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
//     if (ret != 0) continue;

//     fp = popen("/tmp/.expr", "r");
//     assert(fp != NULL);

//     int result;
//     ret = fscanf(fp, "%d", &result);
//     pclose(fp);

//     printf("%u %s\n", result, buf);
//   }
//   return 0;
// }
