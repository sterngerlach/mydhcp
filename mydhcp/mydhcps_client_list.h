
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
 * id, port, addr, mask, ttlはネットワークバイトオーダーで保持
 */
struct dhcp_client_list_entry {
    struct list_entry       list_entry;     /* リンクリストのポインタ */
    enum dhcp_server_state  state;          /* サーバの状態 */
    uint16_t                ttl_counter;    /* IPアドレスの使用期限までの残り時間 */
    struct in_addr          id;             /* クライアントの識別子(IPアドレス) */
    in_port_t               port;           /* クライアントが使用するポート番号 */
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
 * 新たなクライアントをリストに追加
 */
bool append_dhcp_client(
    struct dhcp_client_list_entry* list_head,
    struct in_addr id,
    in_port_t port,
    enum dhcp_server_state state,
    uint16_t ttl_counter);

/*
 * 指定されたクライアントをリストから削除
 */
void remove_dhcp_client(
    struct dhcp_client_list_entry* client);

/*
 * 指定されたクライアントにIPアドレスが割り当てられているかどうかを判定
 */
bool is_ip_address_assigned_to_dhcp_client(
    struct dhcp_client_list_entry* client);

/*
 * 指定されたIPアドレスを持つクライアントの取得
 */
struct dhcp_client_list_entry* lookup_dhcp_client(
    struct dhcp_client_list_entry* list_head, struct in_addr id);

#endif /* MYDHCPS_CLIENT_LIST_H */

