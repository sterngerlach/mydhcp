
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps_client_list.c */

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include "mydhcps_client_list.h"

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
                "client id: %s, state: %s, ttl_counter: %" PRIu16 ", "
                "addr: %s, mask: %s, ttl: %" PRIu16 "\n",
                inet_ntoa(iter->id), convert_server_state_to_string(iter->state),
                iter->ttl_counter, inet_ntoa(iter->addr),
                inet_ntoa(iter->mask), ntohs(iter->ttl));
    }
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

