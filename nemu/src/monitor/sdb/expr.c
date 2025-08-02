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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, 
  TK_EQ = 1,
  TK_NOTEQ = 2,
  TK_NUM = 3,
  TK_REGISTER = 4,
  TK_HEX = 5,
  TK_OR = 6,
  TK_AND = 7,
  TK_LEFT = 8,
  TK_RIGHT = 9,
  TK_LEQ = 10,
  TK_MEQ = 11,
  /* TODO: Add more token types */

};



static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  // {" +", TK_NOTYPE},    // spaces
  // {"\\+", '+'},         // plus
  // {"==", TK_EQ},        // equal

  {" +", TK_NOTYPE},  // spaces
  {"\\+", '+'},       // plus
  {"\\-", '-'},       // sub
  {"\\*", '*'},       // multi
  {"\\/", '/'},       // div

  {"\\(", TK_LEFT},   // (
  {"\\)", TK_RIGHT},  // )

  {"==", TK_EQ},     // equal
  {"<=", TK_LEQ},    // less or equal
  {">=", TK_MEQ},    // more or equal
  {"!=", TK_NOTEQ},  // !=

  {"\\|\\|", TK_OR},
  {"\\&\\&", TK_AND},
  {"\\!", '!'},

  {"\\$(ra|sp|gp|tp|t[0-6]|s[0-9]|a[0-7])", TK_REGISTER},  // register

  {"0[xX][0-9a-fA-F]+", TK_HEX},  // 0xX
  {"[0-9]*", TK_NUM},             // num
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

typedef struct {
    Token data[100];
    int top;
} Stack;

// 栈操作
void push(Stack* s, Token token) {
    s->data[++s->top] = token;
}

Token pop(Stack* s) {
    return s->data[s->top--];
}

Token peek(Stack* s) {
    return s->data[s->top];
}

int isEmpty(Stack* s) {
    return s->top == -1;
}

// 0:number
// 1:op
// 2:()
int isType(int type) {
    switch (type) {
        case TK_NUM:
            return 0;
        case '/':
        case '-':
        case '*':
        case '+':
            return 1;
        case '(':
        case ')':
            return 2;
        default:
            return -1;
    }
}
// 比较优先级
int priority(int type) {
    switch (type) {
        case TK_OR:
            return -4;
        case TK_AND:
            return -3;
        case TK_NOTEQ:
        case TK_EQ:
            return -2;
        case TK_LEQ:
        case TK_MEQ:
            return -1;

        case '+':
        case '-':
            return 0;
        case '*':
        case '/':
            return 1;
        default:
            return -5;
    }
}


static bool make_token(char* e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
      /* Try all rules one by one. */
      for (i = 0; i < NR_REGEX; i++) {
          if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
              pmatch.rm_so == 0) {  // 匹配成功
              // char* substr_start = e + position;
              int substr_len = pmatch.rm_eo;

              // Log("match rules[%d] = \"%s\" at position %d with len %d:
              // %.*s",
              //     i, rules[i].regex, position, substr_len, substr_len,
              //     substr_start);

              position += substr_len;

              /* TODO: Now a new token is recognized with rules[i]. Add codes
               * to record the token in the array `tokens'. For certain types
               * of tokens, some extra actions should be performed.
               */

              // 匹配
              Token temp_token;
              switch (rules[i].token_type) {
                  case '+':
                      temp_token.type = '+';
                      tokens[nr_token++] = temp_token;
                      break;
                  case '-':
                      temp_token.type = '-';
                      tokens[nr_token++] = temp_token;
                      break;
                  case '*':
                      temp_token.type = '*';
                      tokens[nr_token++] = temp_token;
                      break;
                  case '/':
                      temp_token.type = '/';
                      tokens[nr_token++] = temp_token;
                      break;

                  case '!':
                      temp_token.type = '!';
                      tokens[nr_token++] = temp_token;
                      break;
                  case TK_LEFT:
                      temp_token.type = TK_LEFT;
                      tokens[nr_token++] = temp_token;
                      break;
                  case TK_RIGHT:
                      temp_token.type = TK_RIGHT;
                      tokens[nr_token++] = temp_token;
                      break;

                  case TK_NUM:
                      tokens[nr_token].type = TK_NUM;
                      strncpy(tokens[nr_token].str, &e[position - substr_len],
                              substr_len);
                      nr_token++;
                      break;
                  case TK_REGISTER:
                      tokens[nr_token].type = TK_REGISTER;
                      strncpy(tokens[nr_token].str, &e[position - substr_len],
                              substr_len);
                      nr_token++;
                      break;
                  case TK_HEX:  // hex
                      tokens[nr_token].type = TK_HEX;
                      strncpy(tokens[nr_token].str, &e[position - substr_len],
                              substr_len);
                      nr_token++;
                      break;

                  case TK_EQ:
                      tokens[nr_token].type = TK_EQ;
                      strcpy(tokens[nr_token].str, "==");
                      nr_token++;
                      break;
                  case TK_NOTEQ:
                      tokens[nr_token].type = TK_NOTEQ;
                      strcpy(tokens[nr_token].str, "!=");
                      nr_token++;
                      break;
                  case TK_OR:  // or
                      tokens[nr_token].type = TK_OR;
                      strcpy(tokens[nr_token].str, "||");
                      nr_token++;
                      break;
                  case TK_AND:  // and
                      tokens[nr_token].type = TK_AND;
                      strcpy(tokens[nr_token].str, "&&");
                      nr_token++;
                      break;
                  case TK_LEQ:  // <=
                      tokens[nr_token].type = TK_LEQ;
                      strcpy(tokens[nr_token].str, "<=");
                      nr_token++;
                      break;
                  case TK_MEQ:  // >=
                      tokens[nr_token].type = TK_MEQ;
                      strcpy(tokens[nr_token].str, ">=");
                      nr_token++;
                      break;
                  case TK_NOTYPE:  // 空格跳过
                      break;
                  default:  // 待处理其他情况
                      printf("i = %d and No rules is com.\n", i);
                      break;
              }

              break;
          }
      }

      if (i == NR_REGEX) {
          printf("no match at position %d\n%s\n%*.s^\n", position, e,
                 position, "");
          return false;
      }
  }
  // 继续处理，将寄存器、hex、引用等预处理，将所有的数全部转化为 10 进制字符串

  // 寄存器
  for (int i = 0; i < nr_token; i++) {
      if (tokens[i].type == TK_REGISTER) {
          bool flag = false;
          int tmp = isa_reg_str2val(tokens[i].str, &flag);
          if (flag) {
              sprintf(tokens[i].str, "%u", tmp);
              tokens[i].type = TK_NUM;
          } else {
              printf("Transfrom error. \n");
              assert(0);
          }
      }
  }

  // 16进制
  for (int i = 0; i < nr_token; i++) {
      if (tokens[i].type == TK_HEX) {
          int value = strtol(tokens[i].str, NULL, 16);
          sprintf(tokens[i].str, "%u", value);
          tokens[i].type = TK_NUM;
      }
  }

  // 处理负值
  int tokens_len = nr_token;

  for (int i = 0; i < nr_token; i++) {
      // 解决 --1 == 1
      if (i == 0 && i + 2 < nr_token && tokens[i].type == '-' &&
          tokens[i + 1].type == '-' && tokens[i + 2].type == TK_NUM) {
          tokens[i].type = TK_NOTYPE;
          tokens[i + 1].type = TK_NOTYPE;
      }
      //  每次产生新的 TK_NOTYPE，及时去掉它
      for (int j = 0; j < nr_token; j++) {
          if (tokens[j].type == TK_NOTYPE) {
              for (int k = j + 1; k < nr_token; k++) {
                  tokens[k - 1] = tokens[k];
              }
              tokens_len--;
          }
      }
      // 5---1 ==> 5-1，也就是 op -- ==> op
      if (i > 0 && i + 1 < nr_token && isType(tokens[i - 1].type) == 1 &&
          tokens[i].type == '-' && tokens[i + 1].type == '-') {
          tokens[i].type = TK_NOTYPE;
          tokens[i + 1].type = TK_NOTYPE;
      }
      for (int j = 0; j < nr_token; j++) {
          if (tokens[j].type == TK_NOTYPE) {
              for (int k = j + 1; k < nr_token; k++) {
                  tokens[k - 1] = tokens[k];
              }
              tokens_len--;
          }
      }

      if ((tokens[i].type == '-' && i > 0 && tokens[i - 1].type != TK_NUM &&
           tokens[i - 1].type != TK_RIGHT && tokens[i + 1].type == TK_NUM) ||
          (tokens[i].type == '-' && i == 0)) {
          tokens[i].type =
              TK_NOTYPE;  //虽然这是一个负数标志，但是后面必须要忽略它，因此将它定义为空格类型

          // 右移一位，加上'-'
          for (int j = 31; j > 0; j--) {
              tokens[i + 1].str[j] = tokens[i + 1].str[j - 1];
          }
          tokens[i + 1].str[0] = '-';

          for (int j = 0; j < nr_token; j++) {
              if (tokens[j].type == TK_NOTYPE) {
                  for (int k = j + 1; k < nr_token; k++) {
                      tokens[k - 1] = tokens[k];
                  }
                  tokens_len--;
              }
          }
      }
  }

  nr_token = tokens_len;  // 恢复
  // 非运算
  for (int i = 0; i < nr_token; i++) {
      if (tokens[i].type == '!') {
          tokens[i].type = TK_NOTYPE;
          int tmp = atoi(tokens[i + 1].str);
          if (tmp == 0) {
              memset(tokens[i + 1].str, 0, sizeof(tokens[i + 1].str));
              tokens[i + 1].str[0] = '1';
              tokens[i + 1].type = TK_NUM;
          } else {
              memset(tokens[i + 1].str, 0, sizeof(tokens[i + 1].str));
          }
          for (int j = 0; j < nr_token; j++) {
              if (tokens[j].type == TK_NOTYPE) {
                  for (int k = j + 1; k < tokens_len; k++) {
                      tokens[k - 1] = tokens[k];
                  }
                  tokens_len--;
              }
          }
      }
  }
  nr_token = tokens_len;  // 恢复

  // 解引用
  for (int i = 0; i < nr_token; i++) {
      // 如何判断它是一个引用？在表达式语法正确的情况下：
      // 类似于 *123；3**123；(2)**3
      // 引用前面的 token 一定是一个操作符,后面一定是一个数；
      // i=0 && tokens[i].type == '*'
      if ((tokens[i].type == '*' && i > 0 &&
           isType(tokens[i - 1].type) == 1 && tokens[i + 1].type == TK_NUM) ||
          (tokens[i].type == '*' && i == 0)) {
          tokens[i].type = TK_NOTYPE;

          // 转化
          int temp = 0;
          if (tokens[i + 1].type == TK_HEX) {
              temp = strtol(tokens[i + 1].str, NULL, 16);
          } else if (tokens[i + 1].type == TK_NUM) {
              temp = atoi(tokens[i + 1].str);
          }

          // 解引用
          uintptr_t a = (uintptr_t)temp;
          int value = *((int*)a);
          sprintf(tokens[i + 1].str, "%u", value);
          tokens[i + 1].type = TK_NUM;

          for (int j = 0; j < tokens_len; j++) {
              if (tokens[j].type == TK_NOTYPE) {
                  for (int k = j + 1; k < tokens_len; k++) {
                      tokens[k - 1] = tokens[k];
                  }
                  tokens_len--;
              }
          }
      }
  }
  nr_token = tokens_len;  // 恢复

  return true;
}

// 中缀转化为后缀表达式
void midToPost(Token* mid, Token* post, int token_len, int* new_len) {
    Stack ops;
    ops.top = -1;
    int j = 0;

    for (int i = 0; i < token_len; i++) {
        Token token = mid[i];
        if (token.type == TK_NUM) {
            post[j++] = token;
        } else if (token.type == TK_LEFT || isEmpty(&ops)) {
            push(&ops, token);

        } else if (token.type == TK_RIGHT) {
            while (!isEmpty(&ops) && (peek(&ops).type != TK_LEFT)) {
                post[j++] = pop(&ops);
            }
            pop(&ops);
        } else {
            while (!isEmpty(&ops) &&
                   (priority(peek(&ops).type) >= priority(token.type))) {
                post[j++] = pop(&ops);
            }
            push(&ops, token);
        }
    }

    while (!isEmpty(&ops)) {
        post[j++] = pop(&ops);
    }
    *new_len = j;  // 长度
}

// 计算后缀表达式
int evalRPN(Token* tokens, int len) {
    Stack nums;
    nums.top = -1;
    for (int i = 0; i < len; i++) {
        if (tokens[i].type == TK_NUM) {
            push(&nums, tokens[i]);
        } else {
            int num2 = atoi(peek(&nums).str);
            pop(&nums);
            int num1 = atoi(peek(&nums).str);
            pop(&nums);

            Token temp = {0};

            if (tokens[i].type == '+') {
                sprintf(temp.str, "%u", num1 + num2);
                temp.type = TK_NUM;
                push(&nums, temp);
            }
            if (tokens[i].type == '-') {
                sprintf(temp.str, "%u", num1 - num2);
                temp.type = TK_NUM;
                push(&nums, temp);
            }
            if (tokens[i].type == '*') {
                sprintf(temp.str, "%u", num1 * num2);
                temp.type = TK_NUM;
                push(&nums, temp);
            }
            if (tokens[i].type == '/') {
                if (num2 == 0)
                    goto EXCEPTZION;
                else
                    sprintf(temp.str, "%u", num1 / num2);
                temp.type = TK_NUM;
                push(&nums, temp);
            }
            if (tokens[i].type == TK_EQ) {
                sprintf(temp.str, "%u", num1 == num2);
                temp.type = TK_NUM;
                push(&nums, temp);
            }
            if (tokens[i].type == TK_NOTEQ) {
                sprintf(temp.str, "%u", num1 != num2);
                temp.type = TK_NUM;
                push(&nums, temp);
            }
            if (tokens[i].type == TK_LEQ) {
                sprintf(temp.str, "%d", num1 <= num2);
                temp.type = TK_NUM;
                push(&nums, temp);
            }
            if (tokens[i].type == TK_MEQ) {
                sprintf(temp.str, "%d", num1 >= num2);
                temp.type = TK_NUM;
                push(&nums, temp);
            }

            if (tokens[i].type == TK_AND) {
                sprintf(temp.str, "%u", num1 && num2);
                temp.type = TK_NUM;
                push(&nums, temp);
            }
            if (tokens[i].type == TK_OR) {
                sprintf(temp.str, "%u", num1 || num2);
                temp.type = TK_NUM;
                push(&nums, temp);
            }
        }
    }
    return atoi(peek(&nums).str);
EXCEPTZION:
    return 10086;
}

// static bool make_token(char *e) {
//   int position = 0;
//   int i;
//   regmatch_t pmatch;

//   nr_token = 0;

//   while (e[position] != '\0') {
//     /* Try all rules one by one. */
//     for (i = 0; i < NR_REGEX; i ++) {
//       if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
//         char *substr_start = e + position;
//         int substr_len = pmatch.rm_eo;

//         Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
//             i, rules[i].regex, position, substr_len, substr_len, substr_start);

//         position += substr_len;

//         /* TODO: Now a new token is recognized with rules[i]. Add codes
//          * to record the token in the array `tokens'. For certain types
//          * of tokens, some extra actions should be performed.
//          */

//         switch (rules[i].token_type) {
//           default: TODO();
//         }

//         break;
//       }
//     }

//     if (i == NR_REGEX) {
//       printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
//       return false;
//     }
//   }

//   return true;
// }

Token post[1024] = {0};
word_t expr(char* e, bool* success) {
    // 清空 tokens
    memset(tokens, 0, sizeof(tokens));
    memset(post, 0, sizeof(post));
    if (!make_token(e)) {
        *success = false;
        return 0;
    }
    int len = 0;
    *success = true;
    midToPost(tokens, post, nr_token, &len);
    return evalRPN(post, len);
}