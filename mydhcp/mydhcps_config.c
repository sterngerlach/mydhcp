
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* config.c */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "util.h"

/*
 * 割当可能なIPアドレスとネットマスクの組を保持する構造体を破棄
 */
void free_ip_addr_list(struct ip_addr_list_entry* list_head)
{
    struct ip_addr_list_entry* iter;
    struct ip_addr_list_entry* iter_next;

    assert(list_head != NULL);

    list_for_each_entry_safe(iter, iter_next, &list_head->list_entry,
                             struct ip_addr_list_entry, list_entry) {
        free(iter);
        iter = NULL;
    }
}

/*
 * 割当可能なIPアドレスとネットマスクの組を表示
 */
void dump_ip_addr_list(FILE* fp, struct ip_addr_list_entry* list_head)
{
    struct ip_addr_list_entry* iter;

    assert(list_head != NULL);
    
    fprintf(fp, "List of available IP addresses (and masks): \n");

    list_for_each_entry(iter, &list_head->list_entry,
                        struct ip_addr_list_entry, list_entry) {
        fprintf(fp, "%s\t%s\n",
                inet_ntoa(iter->addr), inet_ntoa(iter->mask));
    }
}

/*
 * 割当可能なIPアドレスとネットマスクの組をリストに追加
 */
bool append_available_ip_addr(
    struct ip_addr_list_entry* list_head,
    struct in_addr addr,
    struct in_addr mask)
{
    struct ip_addr_list_entry* new_entry;

    assert(list_head != NULL);

    /* 新たなIPアドレスとネットマスクの組を割り当て */
    new_entry = (struct ip_addr_list_entry*)malloc(sizeof(struct ip_addr_list_entry));

    if (new_entry == NULL) {
        print_error(__func__, "malloc() failed: %s\n", strerror(errno));
        return false;
    }

    /* IPアドレスとネットマスクの組を設定 */
    new_entry->addr = addr;
    new_entry->mask = mask;

    /* IPアドレスとネットマスクの組を先頭に追加 */
    insert_list_entry_tail(&new_entry->list_entry, &list_head->list_entry);

    return true;
}

/*
 * 割当可能なIPアドレスとネットマスクの組を返す
 */
bool get_available_ip_addr(
    struct ip_addr_list_entry* list_head,
    struct in_addr* addr,
    struct in_addr* mask)
{
    struct ip_addr_list_entry* entry;

    assert(list_head != NULL);
    assert(addr != NULL);
    assert(addr != NULL);

    /* 割当可能なIPアドレスがない場合 */
    if (is_list_empty(&list_head->list_entry))
        return false;

    /* リンクリストの先頭からIPアドレスを取得 */
    entry = get_first_list_entry(
        &list_head->list_entry, struct ip_addr_list_entry, list_entry);

    /* リンクリストからIPアドレスを削除 */
    remove_list_entry(&entry->list_entry);

    /* 受け取り用の変数にIPアドレスをコピー */
    *addr = entry->addr;
    *mask = entry->mask;

    /* メモリ領域の解放 */
    free(entry);
    entry = NULL;

    return true;
}

/*
 * 使用済みのIPアドレスとネットマスクの組をリストに戻す
 */
bool recall_ip_addr(
    struct ip_addr_list_entry* list_head,
    struct in_addr addr,
    struct in_addr mask)
{
    return append_available_ip_addr(list_head, addr, mask);
}

/*
 * 設定の内容を保持する構造体を初期化
 */
void initialize_dhcp_server_config(struct dhcp_server_config* config)
{
    assert(config != NULL);

    config->ttl = UINT16_C(0);
    initialize_list_head(&config->available_ip_addr_list_head.list_entry);
}

/*
 * 設定の内容を保持する構造体を破棄
 */
void free_dhcp_server_config(struct dhcp_server_config* config)
{
    assert(config != NULL);

    config->ttl = UINT16_C(0);
    free_ip_addr_list(&config->available_ip_addr_list_head);
}

/*
 * IPアドレスとネットマスクの組の文字列を解析
 */
bool parse_ip_addr_mask_pair(
    char* str, struct in_addr* addr, struct in_addr* mask)
{
    char* p;
    char* addr_str;
    char* mask_str;

    assert(str != NULL);
    assert(addr != NULL);
    assert(mask != NULL);

    p = str;

    /* 空白を読み飛ばす */
    while (*p && isspace(*p)) ++p;
    /* IPアドレスの開始アドレスを記録 */
    addr_str = p;
    /* IPアドレスを読み飛ばす */
    while (*p && !isspace(*p)) ++p;
    /* IPアドレスとネットマスクの区切り文字をヌル文字に置き換え */
    ++p; p[-1] = '\0';
    /* 空白を読み飛ばす */
    while (*p && isspace(*p)) ++p;
    /* ネットマスクの開始アドレスを記録 */
    mask_str = p;
    /* ネットマスクを読み飛ばす */
    while (*p && !isspace(*p)) ++p;
    /* ネットマスクの終わりをヌル文字に置き換え */
    *p = '\0';
    
    /* IPアドレスの文字列をネットワークオーダーのバイナリに変換 */
    if (!inet_aton(addr_str, addr)) {
        print_error(__func__,
                    "inet_aton() failed: "
                    "could not convert ip address \'%s\' to binary\n",
                    addr_str);
        return false;
    }

    /* ネットマスクの文字列をネットワークオーダーのバイナリに変換 */
    if (!inet_aton(mask_str, mask)) {
        print_error(__func__,
                    "inet_aton() failed: "
                    "could not convert ip address mask \'%s\' to binary\n",
                    mask_str);
        return false;
    }

    return true;
}

/*
 * 設定ファイルを読み込み
 */
bool load_config(const char* file_name, struct dhcp_server_config* config)
{
    FILE* fp = NULL;
    char* line = NULL;
    size_t length;
    ssize_t cc;
    struct in_addr addr;
    struct in_addr mask;

    assert(file_name != NULL);
    assert(config != NULL);
    
    /* 設定ファイルを開く */
    if ((fp = fopen(file_name, "r")) == NULL) {
        print_error(__func__, "could not open \'%s\': %s\n",
                    file_name, strerror(errno));
        return false;
    }

    /* 使用期限の秒数が存在しない場合はエラー */
    if ((cc = getline(&line, &length, fp)) == -1) {
        print_error(__func__, "could not read ttl: %s\n", strerror(errno));
        free(line);
        return false;
    }
    
    /* 使用期限の秒数を設定 */
    sscanf(line, SCNu16, &config->ttl);

    /* 割当可能なIPアドレスとネットマスクの組を読み込み */
    while ((cc = getline(&line, &length, fp)) != -1) {
        if (!parse_ip_addr_mask_pair(line, &addr, &mask)) {
            print_error(__func__,
                        "parse_ip_addr_mask_pair() failed: "
                        "failed to parse line \'%s\'\n",
                        line);
            free(line);
            return false;
        }
        
        /* 割当可能なIPアドレスとネットマスクの組をリンクリストに追加 */
        if (!append_available_ip_addr(&config->available_ip_addr_list_head,
                                      mask, addr)) {
            print_error(__func__, "append_ip_addr() failed\n");
            free(line);
            return false;
        }
    }
    
    /* 設定ファイルを閉じる */
    fclose(fp);
    fp = NULL;

    return true;
}

