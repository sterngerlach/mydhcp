
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcp.h */

#ifndef MYDHCP_H
#define MYDHCP_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "linked_list.h"

/*
 * 疑似DHCPヘッダを表す構造体
 */
struct dhcp_header {
    uint8_t     type;
    uint8_t     code;
    uint16_t    ttl;
    in_addr_t   addr;
    in_addr_t   mask;
};

/*
 * 接続されたクライアントの情報を保持する構造体
 */
struct dhcp_client_list_entry {
    struct list_entry   list_entry;
    int16_t             status;
    uint16_t            ttl_counter;
    struct in_addr      id;
    struct in_addr      addr;
    struct in_addr      mask;
    uint16_t            ttl;
};

/*
 * 接続されたクライアントのリストの先頭
 */
struct dhcp_client_list_entry dhcp_client_list_head;

#endif /* MYDHCP_H */

