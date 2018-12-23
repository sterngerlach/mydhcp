
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcp.c */

#include <assert.h>
#include <inttypes.h>

#include "mydhcp.h"

/*
 * DHCPヘッダを出力
 */
void dump_dhcp_header(FILE* fp, const struct dhcp_header* header)
{
    assert(header != NULL);

    fprintf(fp,
            "dhcp header type: %s, code: %s, ttl: %" PRIu16 ", "
            "addr: %s, mask: %s\n",
            dhcp_header_type_to_string(header->type),
            dhcp_header_code_to_string(header->type, header->code),
            header->ttl,
            inet_ntoa(*(struct in_addr*)&header->addr),
            inet_ntoa(*(struct in_addr*)&header->mask));
}

/*
 * DHCPヘッダのTypeフィールドを表す定数を文字列に変換
 */
const char* dhcp_header_type_to_string(uint8_t type)
{
    static const char* header_type_str[] = {
        [DHCP_HEADER_TYPE_DISCOVER] = "DHCP_HEADER_TYPE_DISCOVER",
        [DHCP_HEADER_TYPE_OFFER] = "DHCP_HEADER_TYPE_OFFER",
        [DHCP_HEADER_TYPE_REQUEST] = "DHCP_HEADER_TYPE_REQUEST",
        [DHCP_HEADER_TYPE_ACK] = "DHCP_HEADER_TYPE_ACK",
        [DHCP_HEADER_TYPE_RELEASE] = "DHCP_HEADER_TYPE_RELEASE",
    };

    return header_type_str[type];
}

/*
 * DHCPヘッダのCodeフィールドを表す定数を文字列に変換
 */
const char* dhcp_header_code_to_string(uint8_t type, uint8_t code)
{
    static const char* header_code_offer_str[] = {
        [DHCP_HEADER_CODE_OFFER_OK] = "DHCP_HEADER_CODE_OFFER_OK",
        [DHCP_HEADER_CODE_OFFER_NG] = "DHCP_HEADER_CODE_OFFER_NG",
    };

    static const char* header_code_request_str[] = {
        [DHCP_HEADER_CODE_REQUEST_ALLOC] = "DHCP_HEADER_CODE_REQUEST_ALLOC",
        [DHCP_HEADER_CODE_REQUEST_TIME_EXT] = "DHCP_HEADER_CODE_REQUEST_TIME_EXT",
    };

    static const char* header_code_ack_str[] = {
        [DHCP_HEADER_CODE_ACK_OK] = "DHCP_HEADER_CODE_ACK_OK",
        [DHCP_HEADER_CODE_ACK_NG] = "DHCP_HEADER_CODE_ACK_NG",
    };

    static const char** header_code_str[] = {
        [DHCP_HEADER_TYPE_OFFER] = header_code_offer_str,
        [DHCP_HEADER_TYPE_REQUEST] = header_code_request_str,
        [DHCP_HEADER_TYPE_ACK] = header_code_ack_str,
    };

    return header_code_str[type][code];
}

