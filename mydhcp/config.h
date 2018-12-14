
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* config.h */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "linked_list.h"
#include "mydhcp.h"

/*
 * 割当可能なIPアドレスのリストを保持する構造体
 */
struct ip_addr_list_entry {
    struct list_entry   list_entry;
    struct in_addr      addr;
    struct in_addr      mask;
};

/*
 * 設定の内容を保持する構造体
 */
struct dhcp_server_config {
    uint16_t                  ttl;
    struct ip_addr_list_entry available_ip_addr_list_head;
};

/*
 * 割当可能なIPアドレスとネットマスクの組を保持する構造体を破棄
 */
void free_ip_addr_list(struct ip_addr_list_entry* list_head);

/*
 * 割当可能なIPアドレスとネットマスクの組を表示
 */
void dump_ip_addr_list(FILE* fp, struct ip_addr_list_entry* list_head);

/*
 * 割当可能なIPアドレスとネットマスクの組をリストに追加
 */
bool append_available_ip_addr(
    struct ip_addr_list_entry* list_head,
    struct in_addr addr,
    struct in_addr mask);

/*
 * 割当可能なIPアドレスとネットマスクの組を返す
 */
bool get_available_ip_addr(
    struct ip_addr_list_entry* list_head,
    struct in_addr* addr,
    struct in_addr* mask);

/*
 * 使用済みのIPアドレスとネットマスクの組をリストに戻す
 */
bool recall_ip_addr(
    struct ip_addr_list_entry* list_head,
    struct in_addr addr,
    struct in_addr mask);

/*
 * 設定の内容を保持する構造体を初期化
 */
void initialize_dhcp_server_config(struct dhcp_server_config* config);

/*
 * 設定の内容を保持する構造体を破棄
 */
void free_dhcp_server_config(struct dhcp_server_config* config);

/*
 * IPアドレスとネットマスクの組の文字列を解析
 */
bool parse_ip_addr_mask_pair(
    char* str, struct in_addr* addr, struct in_addr* mask);

/*
 * 設定ファイルを読み込み
 */
bool load_config(const char* file_name, struct dhcp_server_config* config);

#endif /* CONFIG_H */

