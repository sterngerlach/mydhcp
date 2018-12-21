
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps_ip_addr_list.h */

#ifndef MYDHCPS_IP_ADDR_LIST_H
#define MYDHCPS_IP_ADDR_LIST_H

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "linked_list.h"

/*
 * 割当可能なIPアドレスのリストを保持する構造体
 */
struct ip_addr_list_entry {
    struct list_entry   list_entry;
    struct in_addr      addr;
    struct in_addr      mask;
};

/*
 * 割当可能なIPアドレスとサブネットマスクの組を保持する構造体を初期化
 */
void initialize_ip_addr_list(struct ip_addr_list_entry* list_head);

/*
 * 割当可能なIPアドレスとサブネットマスクの組を保持する構造体を破棄
 */
void free_ip_addr_list(struct ip_addr_list_entry* list_head);

/*
 * 割当可能なIPアドレスとサブネットマスクの組を表示
 */
void dump_ip_addr_list(FILE* fp, const struct ip_addr_list_entry* list_head);

/*
 * 割当可能なIPアドレスとサブネットマスクの組をリストに追加
 */
bool append_available_ip_addr(
    struct ip_addr_list_entry* list_head,
    struct in_addr addr,
    struct in_addr mask);

/*
 * 割当可能なIPアドレスとサブネットマスクの組を返す
 */
bool get_available_ip_addr(
    struct ip_addr_list_entry* list_head,
    struct in_addr* addr,
    struct in_addr* mask);

/*
 * 使用済みのIPアドレスとサブネットマスクの組をリストに戻す
 */
bool recall_ip_addr(
    struct ip_addr_list_entry* list_head,
    struct in_addr addr,
    struct in_addr mask);

#endif /* MYDHCPS_IP_ADDR_LIST_H */

