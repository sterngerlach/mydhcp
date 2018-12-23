
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcp.h */

#ifndef MYDHCP_H
#define MYDHCP_H

#include <stdint.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * タイムアウトと判定される待ち時間
 */
#define TIMEOUT_VALUE                   10

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
 * DHCPヘッダのTypeフィールドを表す各種定数
 */
#define DHCP_HEADER_TYPE_DISCOVER       1
#define DHCP_HEADER_TYPE_OFFER          2
#define DHCP_HEADER_TYPE_REQUEST        3
#define DHCP_HEADER_TYPE_ACK            4
#define DHCP_HEADER_TYPE_RELEASE        5

/*
 * DHCPヘッダのCodeフィールドを表す各種定数
 */
#define DHCP_HEADER_CODE_OFFER_OK           0
#define DHCP_HEADER_CODE_OFFER_NG           1
#define DHCP_HEADER_CODE_REQUEST_ALLOC      2
#define DHCP_HEADER_CODE_REQUEST_TIME_EXT   3
#define DHCP_HEADER_CODE_ACK_OK             0
#define DHCP_HEADER_CODE_ACK_NG             4

/*
 * DHCPヘッダを出力
 */
void dump_dhcp_header(FILE* fp, const struct dhcp_header* header);

/*
 * DHCPヘッダのTypeフィールドを表す定数を文字列に変換
 */
const char* dhcp_header_type_to_string(uint8_t type);

/*
 * DHCPヘッダのCodeフィールドを表す定数を文字列に変換
 */
const char* dhcp_header_code_to_string(uint8_t type, uint8_t code);

#endif /* MYDHCP_H */

