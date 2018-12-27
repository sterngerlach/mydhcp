
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcpc_context.h */

#ifndef MYDHCPC_CONTEXT_H
#define MYDHCPC_CONTEXT_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mydhcpc_state_machine.h"

/*
 * DHCPクライアントの情報を保持する構造体
 * server_addr, addr, mask, ttlはネットワークバイトオーダーで保持
 */
struct dhcp_client_context {
    enum dhcp_client_state  state;          /* クライアントの現在の状態 */
    uint16_t                ttl_counter;    /* IPアドレスの使用期限(またはタイムアウト)までの残り時間 */
    int                     client_sock;    /* クライアントが使用するソケット */
    struct in_addr          server_addr;    /* サーバのIPアドレス */
    struct in_addr          addr;           /* 割り当てられたIPアドレス */
    struct in_addr          mask;           /* 割り当てられたIPアドレスのサブネットマスク */
    uint16_t                ttl;            /* IPアドレスの使用期限 */
};

/*
 * DHCPクライアントの情報を初期化
 */
bool initialize_dhcp_client_context(
    struct dhcp_client_context* context, const char* server_addr_str);

/*
 * DHCPクライアントの情報を破棄
 */
void free_dhcp_client_context(struct dhcp_client_context* context);

#endif /* MYDHCPC_CONTEXT_H */

