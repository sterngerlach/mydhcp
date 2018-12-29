
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcp.c */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>

#include "mydhcp.h"
#include "util.h"

/*
 * DHCPヘッダを出力
 */
void dump_dhcp_header(FILE* fp, const struct dhcp_header* header)
{
    char addr_str[INET_ADDRSTRLEN];
    char mask_str[INET_ADDRSTRLEN];

    assert(header != NULL);

    if (inet_ntop(AF_INET, &header->addr, addr_str, sizeof(addr_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *addr_str = '\0';
    }

    if (inet_ntop(AF_INET, &header->mask, mask_str, sizeof(mask_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *mask_str = '\0';
    }

    fprintf(fp,
            "type: " ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET ", "
            "code: " ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET ", "
            "ttl: " ANSI_ESCAPE_COLOR_GREEN "%" PRIu16 ANSI_ESCAPE_COLOR_RESET ", "
            "addr: " ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET ", "
            "mask: " ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
            dhcp_header_type_to_string(header->type),
            dhcp_header_code_to_string(header->type, header->code),
            ntohs(header->ttl), addr_str, mask_str);
}

/*
 * DHCPヘッダのTypeフィールドを表す定数を文字列に変換
 */
const char* dhcp_header_type_to_string(uint8_t type)
{
    static const char* header_type_str[] = {
        [DHCP_HEADER_TYPE_DISCOVER] = "DISCOVER",
        [DHCP_HEADER_TYPE_OFFER]    = "OFFER",
        [DHCP_HEADER_TYPE_REQUEST]  = "REQUEST",
        [DHCP_HEADER_TYPE_ACK]      = "ACK",
        [DHCP_HEADER_TYPE_RELEASE]  = "RELEASE",
    };

    assert(type <= DHCP_HEADER_TYPE_RELEASE);

    return header_type_str[type] ?
           header_type_str[type] : "Unknown";
}

/*
 * DHCPヘッダのCodeフィールドを表す定数を文字列に変換
 */
const char* dhcp_header_code_to_string(uint8_t type, uint8_t code)
{
    static const char* header_code_offer_str[] = {
        [DHCP_HEADER_CODE_OFFER_OK] = "OFFER_OK",
        [DHCP_HEADER_CODE_OFFER_NG] = "OFFER_NG",
    };

    static const char* header_code_request_str[] = {
        [DHCP_HEADER_CODE_REQUEST_ALLOC]    = "REQUEST_ALLOC",
        [DHCP_HEADER_CODE_REQUEST_TIME_EXT] = "REQUEST_TIME_EXT",
    };

    static const char* header_code_ack_str[] = {
        [DHCP_HEADER_CODE_ACK_OK] = "ACK_OK",
        [DHCP_HEADER_CODE_ACK_NG] = "ACK_NG",
    };

    static const char** header_code_str[] = {
        [DHCP_HEADER_TYPE_OFFER] = header_code_offer_str,
        [DHCP_HEADER_TYPE_REQUEST] = header_code_request_str,
        [DHCP_HEADER_TYPE_ACK] = header_code_ack_str,
    };

    assert(type <= DHCP_HEADER_TYPE_ACK);
    assert((type == DHCP_HEADER_TYPE_OFFER) ? (code <= DHCP_HEADER_CODE_OFFER_NG) :
           (type == DHCP_HEADER_TYPE_REQUEST) ? (code <= DHCP_HEADER_CODE_REQUEST_TIME_EXT) :
           (type == DHCP_HEADER_TYPE_ACK) ? (code <= DHCP_HEADER_CODE_ACK_NG) :
           true);

    return header_code_str[type] ?
           header_code_str[type][code] ?
           header_code_str[type][code] : "Unknown" : "Unknown";
}

