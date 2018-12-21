
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps_config.c */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mydhcps_config.h"
#include "mydhcps_ip_addr_list.h"
#include "util.h"

/*
 * 設定の内容を保持する構造体を初期化
 */
void initialize_dhcp_server_config(struct dhcp_server_config* config)
{
    assert(config != NULL);

    config->ttl = UINT16_C(0);
    initialize_ip_addr_list(&config->available_ip_addr_list_head);
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
 * IPアドレスとサブネットマスクの組の文字列を解析
 */
bool parse_ip_addr_mask_pair(
    const char* str, struct in_addr* addr, struct in_addr* mask)
{
    char* p;
    char* tmp;
    char* addr_str;
    char* mask_str;
    char* save_ptr;

    assert(str != NULL);
    assert(addr != NULL);
    assert(mask != NULL);

    if ((tmp = strdup(str)) == NULL) {
        print_error(__func__, "strdup() failed: %s\n", strerror(errno));
        return false;
    }

    /* 文字列の末尾の改行を削除 */
    chomp(tmp);

    /* IPアドレスへのポインタを取得 */
    addr_str = strtok_r(tmp, " \t", &save_ptr);

    /* IPアドレスがない場合はエラー */
    if (addr_str == NULL) {
        print_error(__func__, "address not found\n");
        free(tmp);
        return false;
    }

    /* サブネットマスクへのポインタを取得 */
    mask_str = strtok_r(NULL, " \t", &save_ptr);

    /* サブネットマスクがない場合はエラー */
    if (mask_str == NULL) {
        print_error(__func__, "ip address mask not found\n");
        free(tmp);
        return false;
    }

    /* 余計な記述がある場合はエラー */
    if ((p = strtok_r(NULL, " \t", &save_ptr)) != NULL) {
        print_error(__func__, "unknown configuration setting: \'%s\'\n", p);
        free(tmp);
        return false;
    }

    /* IPアドレスの文字列をネットワークオーダーのバイナリに変換 */
    if (!inet_aton(addr_str, addr)) {
        print_error(__func__,
                    "inet_aton() failed: "
                    "could not convert ip address \'%s\' to binary\n",
                    addr_str);
        free(tmp);
        return false;
    }

    /* サブネットマスクの文字列をネットワークオーダーのバイナリに変換 */
    if (!inet_aton(mask_str, mask)) {
        print_error(__func__,
                    "inet_aton() failed: "
                    "could not convert ip address mask \'%s\' to binary\n",
                    mask_str);
        free(tmp);
        return false;
    }

    free(tmp);

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
    sscanf(line, "%" SCNu16, &config->ttl);

    /* 割当可能なIPアドレスとサブネットマスクの組を読み込み */
    while ((cc = getline(&line, &length, fp)) != -1) {
        if (!parse_ip_addr_mask_pair(line, &addr, &mask)) {
            print_error(__func__,
                        "parse_ip_addr_mask_pair() failed: "
                        "failed to parse line \'%s\'\n",
                        line);
            free(line);
            return false;
        }
        
        /* 割当可能なIPアドレスとサブネットマスクの組をリンクリストに追加 */
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

