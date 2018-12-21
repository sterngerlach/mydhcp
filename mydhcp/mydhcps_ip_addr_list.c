
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps_ip_addr_list.c */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "mydhcps_ip_addr_list.h"
#include "util.h"

/*
 * 割当可能なIPアドレスとサブネットマスクの組を保持する構造体を初期化
 */
void initialize_ip_addr_list(struct ip_addr_list_entry* list_head)
{
	assert(list_head != NULL);

	initialize_list_head(&list_head->list_entry);
}

/*
 * 割当可能なIPアドレスとサブネットマスクの組を保持する構造体を破棄
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
 * 割当可能なIPアドレスとサブネットマスクの組を表示
 */
void dump_ip_addr_list(FILE* fp, const struct ip_addr_list_entry* list_head)
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
 * 割当可能なIPアドレスとサブネットマスクの組をリストに追加
 */
bool append_available_ip_addr(
    struct ip_addr_list_entry* list_head,
    struct in_addr addr,
    struct in_addr mask)
{
    struct ip_addr_list_entry* new_entry;

    assert(list_head != NULL);

    /* 新たなIPアドレスとサブネットマスクの組を割り当て */
    new_entry = (struct ip_addr_list_entry*)malloc(sizeof(struct ip_addr_list_entry));

    if (new_entry == NULL) {
        print_error(__func__, "malloc() failed: %s\n", strerror(errno));
        return false;
    }

    /* IPアドレスとサブネットマスクの組を設定 */
    new_entry->addr = addr;
    new_entry->mask = mask;

    /* IPアドレスとサブネットマスクの組を先頭に追加 */
    insert_list_entry_tail(&new_entry->list_entry, &list_head->list_entry);

    return true;
}

/*
 * 割当可能なIPアドレスとサブネットマスクの組を返す
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
 * 使用済みのIPアドレスとサブネットマスクの組をリストに戻す
 */
bool recall_ip_addr(
    struct ip_addr_list_entry* list_head,
    struct in_addr addr,
    struct in_addr mask)
{
    return append_available_ip_addr(list_head, addr, mask);
}

