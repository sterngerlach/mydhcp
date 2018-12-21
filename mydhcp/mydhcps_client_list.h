
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps_client_list.h */

#ifndef MYDHCPS_CLIENT_LIST_H
#define MYDHCPS_CLIENT_LIST_H

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "linked_list.h"
#include "mydhcps_state_machine.h"

/*
 * 接続されたクライアントの情報を保持する構造体
 */
struct dhcp_client_list_entry {
    struct list_entry       list_entry;     /* リンクリストのポインタ */
    enum dhcp_server_state  state;          /* サーバの状態 */
    uint16_t                ttl_counter;    /* IPアドレスの使用期限までの残り時間 */
    struct in_addr          id;             /* クライアントの識別子(IPアドレス) */
    struct in_addr          addr;           /* クライアントに割り当てたIPアドレス */
    struct in_addr          mask;           /* クライアントに割り当てたIPアドレスのサブネットマスク */
    uint16_t                ttl;            /* IPアドレスの使用期限 */
};

/*
 * 接続されたクライアントのリストの初期化
 */
void initialize_client_list(struct dhcp_client_list_entry* list_head);

/*
 * 接続されたクライアントのリストの破棄
 */
void free_client_list(struct dhcp_client_list_entry* list_head);

/*
 * 接続されたクライアントのリストを表示
 */
void dump_client_list(FILE* fp, const struct dhcp_client_list_entry* list_head);

/*
 * 指定されたIPアドレスを持つクライアントの取得
 */
struct dhcp_client_list_entry* lookup_dhcp_client(
    struct dhcp_client_list_entry* list_head, struct in_addr id);

#endif /* MYDHCPS_CLIENT_LIST_H */

