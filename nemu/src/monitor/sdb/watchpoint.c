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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

// // free 上是潜在的空 wp，它的状态不应该发生改变
// WP* new_wp() {
//   WP* p = free_;
//   while (p != NULL) {
//       if (p->tag == false) {
//           p->tag = true;
//           return p;
//       }
//       p = p->next;
//   }

//   assert(0);
//   return NULL;
// }

// // 只需要将free_wp 的状态改为 false，然后清空一些值
// void free_wp(WP* wp) {
//   WP* p = free_;
//   while (p != NULL) {
//       if (wp->NO == p->NO) {
//           p->NO = 0;
//           memset(p->expr, 0, sizeof(p->expr));
//           p->old_value = 0;
//           p->new_value = 0;
//           p->tag = false;
//           break;
//       }
//       p = p->next;
//   }
// }

// // wp_delete 直接 free 掉
// int delete_wp(int No) {
//   WP* p = free_;
//   while (p != NULL) {
//       if (p->NO == No) {
//           free_wp(p);
//           return 0;
//       }
//       p = p->next;
//   }
//   return -1;  // 删除行为错误
// }

// int create_wp(char* args) {
//   WP* temp = new_wp();
//   strcpy(temp->expr, args);
//   bool success = false;
//   int v = expr(temp->expr, &success);
//   if (success == true) {
//       temp->old_value = v;
//       temp->tag = true;
//   } else {
//       printf("wrong expr() process.\n");
//       return -1;
//   }
//   printf("Watch Point created success.\n");

//   return 0;
// }
