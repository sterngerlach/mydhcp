
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps_client_list.c */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "mydhcps_client_list.h"
#include "util.h"

/*
 * 接続されたクライアントのリストの初期化
 */
void initialize_client_list(struct dhcp_client_list_entry* list_head)
{
    assert(list_head != NULL);

    initialize_list_head(&list_head->list_entry);
}

/*
 * 接続されたクライアントのリストの破棄
 */
void free_client_list(struct dhcp_client_list_entry* list_head)
{
    struct dhcp_client_list_entry* iter;
    struct dhcp_client_list_entry* iter_next;

    assert(list_head != NULL);

    list_for_each_entry_safe(iter, iter_next, &list_head->list_entry,
                             struct dhcp_client_list_entry, list_entry) {
        free(iter);
        iter = NULL;
    }
}

/*
 * 接続されたクライアントのリストを表示
 */
void dump_client_list(FILE* fp, const struct dhcp_client_list_entry* list_head)
{
    struct dhcp_client_list_entry* iter;

    assert(list_head != NULL);

    fprintf(fp, "List of clients: \n");

    list_for_each_entry(iter, &list_head->list_entry,
                        struct dhcp_client_list_entry, list_entry) {
        fprintf(fp,
                "client id: " ANSI_ESCAPE_COLOR_RED "%s" ANSI_ESCAPE_COLOR_RESET
                ", state: " ANSI_ESCAPE_COLOR_RED "%s" ANSI_ESCAPE_COLOR_RESET
                ", ttl_counter: " ANSI_ESCAPE_COLOR_RED "%" PRIu16 ANSI_ESCAPE_COLOR_RESET
                ", addr: " ANSI_ESCAPE_COLOR_RED "%s" ANSI_ESCAPE_COLOR_RESET
                ", mask: " ANSI_ESCAPE_COLOR_RED "%s" ANSI_ESCAPE_COLOR_RESET
                ", ttl: " ANSI_ESCAPE_COLOR_RED "%" PRIu16 ANSI_ESCAPE_COLOR_RESET "\n",
                inet_ntoa(iter->id), dhcp_server_state_to_string(iter->state),
                iter->ttl_counter, inet_ntoa(iter->addr),
                inet_ntoa(iter->mask), ntohs(iter->ttl));
    }
}

/*
 * 新たなクライアントをリストに追加
 */
bool append_dhcp_client(
    struct dhcp_client_list_entry* list_head,
    struct in_addr id,
    in_port_t port,
    enum dhcp_server_state state,
    uint16_t ttl_counter)
{
    struct dhcp_client_list_entry* new_entry;

    assert(list_head != NULL);

    /* 接続されたクライアントの情報を保持する構造体を作成 */
    new_entry = (struct dhcp_client_list_entry*)malloc(sizeof(struct dhcp_client_list_entry));

    if (new_entry == NULL) {
        print_error(__func__, "malloc() failed: %s\n", strerror(errno));
        return false;
    }

    /* クライアントの情報を設定 */
    memset(new_entry, 0, sizeof(struct dhcp_client_list_entry));
    new_entry->state = state;
    new_entry->ttl_counter = ttl_counter;
    new_entry->id = id;
    new_entry->port = port;
    new_entry->addr.s_addr = htonl(0);
    new_entry->mask.s_addr = htonl(0);
    new_entry->ttl = htons(0);

    /* クライアントをリストに追加 */
    insert_list_entry_tail(&new_entry->list_entry, &list_head->list_entry);

    return true;
}

/*
 * 指定されたクライアントをリストから削除
 */
void remove_dhcp_client(
    struct dhcp_client_list_entry* client)
{
    assert(client != NULL);
    
    /* リンクリストからクライアントを削除 */
    remove_list_entry(&client->list_entry);

    /* メモリ領域の解放 */
    free(client);
    client = NULL;
}

/*
 * 指定されたクライアントにIPアドレスが割り当てられているかどうかを判定
 */
bool is_ip_address_assigned_to_dhcp_client(
    struct dhcp_client_list_entry* client)
{
    assert(client != NULL);

    /* アドレスが0.0.0.0であれば割り当てられていないと判定 */
    return client->addr.s_addr != htonl(0);
}

/*
 * 指定されたIPアドレスを持つクライアントの取得
 */
struct dhcp_client_list_entry* lookup_dhcp_client(
    struct dhcp_client_list_entry* list_head, struct in_addr id)
{
    struct dhcp_client_list_entry* iter;

    list_for_each_entry(iter, &list_head->list_entry,
                        struct dhcp_client_list_entry, list_entry) {
        if (iter->id.s_addr == id.s_addr)
            return iter;
    }

    return NULL;
}

