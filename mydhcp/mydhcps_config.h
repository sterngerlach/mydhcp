
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps_config.h */

#ifndef MYDHCPS_CONFIG_H
#define MYDHCPS_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#include "mydhcp.h"
#include "mydhcps_ip_addr_list.h"

/*
 * 設定の内容を保持する構造体
 */
struct dhcp_server_config {
    uint16_t                  ttl;
    struct ip_addr_list_entry available_ip_addr_list_head;
};

/*
 * 設定の内容を保持する構造体を初期化
 */
void initialize_dhcp_server_config(struct dhcp_server_config* config);

/*
 * 設定の内容を保持する構造体を破棄
 */
void free_dhcp_server_config(struct dhcp_server_config* config);

/*
 * IPアドレスとサブネットマスクの組の文字列を解析
 */
bool parse_ip_addr_mask_pair(
    const char* str, struct in_addr* addr, struct in_addr* mask);

/*
 * 設定ファイルを読み込み
 */
bool load_config(const char* file_name, struct dhcp_server_config* config);

#endif /* MYDHCPS_CONFIG_H */

