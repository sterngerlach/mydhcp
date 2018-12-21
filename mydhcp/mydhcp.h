
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

#endif /* MYDHCP_H */

