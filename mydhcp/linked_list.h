
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* linked_list.h */

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdbool.h>
#include <stddef.h>

/*
 * 双方向リンクリストの要素を表す構造体
 */
struct list_entry {
    struct list_entry* next;
    struct list_entry* prev;
};

/*
 * 双方向リンクリストを初期化する
 */
static inline void initialize_list_head(struct list_entry* list_head)
{
    list_head->next = list_head;
    list_head->prev = list_head;
}

/*
 * 双方向リンクリストが空かどうかを返す
 */
static inline bool is_list_empty(const struct list_entry* list_head)
{
    return list_head->next == list_head;
}

/*
 * 指定された要素が双方向リンクリストの末尾に含まれているかどうかを返す
 */
static inline bool is_list_entry_last(
    const struct list_entry* entry, const struct list_entry* list_head)
{
    return entry->next == list_head;
}

/*
 * 双方向リンクリストに要素が1つだけ含まれているかどうかを返す
 */
static inline bool is_list_singular(const struct list_entry* list_head)
{
    return !is_list_empty(list_head) && (list_head->next == list_head->prev);
}

/*
 * 指定された要素を双方向リンクリストの連続した2つの要素間に挿入する
 */
static inline void insert_list_entry(
    struct list_entry* new_entry, struct list_entry* prev_entry,
    struct list_entry* next_entry)
{
    next_entry->prev = new_entry;
    new_entry->next = next_entry;
    new_entry->prev = prev_entry;
    prev_entry->next = new_entry;
}

/*
 * 指定された要素を双方向リンクリストの先頭に追加する
 */
static inline void insert_list_entry_head(
    struct list_entry* new_entry, struct list_entry* list_head)
{
    insert_list_entry(new_entry, list_head, list_head->next);
}

/*
 * 指定された要素を双方向リンクリストの末尾に追加する
 */
static inline void insert_list_entry_tail(
    struct list_entry* new_entry, struct list_entry* list_head)
{
    insert_list_entry(new_entry, list_head->prev, list_head);
}

/*
 * 指定された要素の後に新たな項目を追加する
 */
static inline void insert_list_entry_after(
    struct list_entry* new_entry, struct list_entry* entry)
{
    insert_list_entry(new_entry, entry, entry->next);
}

/*
 * 指定された要素の前に新たな項目を追加する
 */
static inline void insert_list_entry_before(
    struct list_entry* new_entry, struct list_entry* entry)
{
    insert_list_entry(new_entry, entry->prev, entry);
}

/*
 * 指定された要素を双方向リンクリストから削除する
 */
static inline void remove_list_entry(struct list_entry* entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->next = NULL;
    entry->prev = NULL;
}

/*
 * 双方向リンクリストの先頭の要素を削除する
 */
static inline struct list_entry* remove_list_entry_head(struct list_entry* list_head)
{
    struct list_entry* entry;

    if (is_list_empty(list_head))
        return NULL;

    entry = list_head->next;
    remove_list_entry(entry);

    return entry;
}

/*
 * 双方向リンクリストの末尾の要素を削除する
 */
static inline struct list_entry* remove_list_entry_tail(struct list_entry* list_head)
{
    struct list_entry* entry;
    
    if (is_list_empty(list_head))
        return NULL;

    entry = list_head->prev;
    remove_list_entry(entry);

    return entry;
}

/*
 * ポインタが指すメンバを含む構造体へのポインタを返す
 * ptrがtype構造体のmemberを指しているときに, 構造体の先頭のアドレスと
 * memberのアドレスとの差分を求めてptrから引くことで,
 * type構造体の先頭へのポインタが得られる
 */
#define container_of(ptr, type, member) \
    ((type*)((char*)(ptr) - ((ptrdiff_t)&(((type*)0)->member))))

/*
 * list_entry構造体を含む構造体へのポインタを返す
 * ptrがtype構造体に含まれるlist_entry構造体のメンバ(member)を
 * 指しているときに, type構造体の先頭へのポインタを得る
 */
#define get_list_entry(ptr, type, member) \
    container_of(ptr, type, member)

/*
 * 双方向リンクリストの最初の要素を取得する
 */
#define get_first_list_entry(ptr, type, member) \
    container_of((ptr)->next, type, member)

/*
 * 双方向リンクリストの末尾の要素を取得する
 */
#define get_last_list_entry(ptr, type, member) \
    container_of((ptr)->prev, type, member)

/*
 * 双方向リンクリストの次の要素を取得する
 */
#define get_next_list_entry(entry, type, member) \
    container_of((entry)->member.next, type, member)

/*
 * 双方向リンクリストの前の要素を取得する
 */
#define get_prev_list_entry(entry, type, member) \
    container_of((entry)->member.prev, type, member)

/*
 * 双方向リンクリストの要素を先頭から順番に取得する
 */
#define list_for_each(iter, list_head) \
    for (iter = (list_head)->next; iter != (list_head); iter = iter->next)

/*
 * 双方向リンクリストの要素を末尾から順番に取得する
 */
#define list_for_each_reverse(iter, list_head) \
    for (iter = (list_head)->prev; iter != (list_head); iter = iter->prev)

/*
 * 双方向リンクリストの要素を先頭から順番に取得する
 * 取得した要素の削除が可能
 */
#define list_for_each_safe(iter, iter_next, list_head) \
    for (iter = (list_head)->next, iter_next = iter->next; iter != (list_head); \
         iter = iter_next, iter_next = iter->next)

/*
 * 双方向リンクリストの要素を末尾から順番に取得する
 * 取得した要素の削除が可能
 */
#define list_for_each_reverse_safe(iter, iter_prev, list_head) \
    for (iter = (list_head)->prev, iter_prev = iter->prev; iter != (list_head); \
         iter = iter_prev, iter_prev = iter->prev)

/*
 * 双方向リンクリストの指定された要素から末尾までを順番に取得する
 */
#define list_for_each_from(iter, list_head, type, member) \
    for (; &iter->member != (list_head); \
         iter = get_next_list_entry(iter, type, member))

/*
 * 双方向リンクリストの指定された要素から先頭までを順番に取得する
 */
#define list_for_each_from_reverse(iter, list_head, type, member) \
    for (; &iter->member != (list_head); \
         iter = get_prev_list_entry(iter, type, member))

/*
 * 双方向リンクリストの要素を, 指定された型で先頭から順番に取得する
 */
#define list_for_each_entry(iter, list_head, type, member) \
    for (iter = get_first_list_entry(list_head, type, member); \
         &iter->member != (list_head); \
         iter = get_next_list_entry(iter, type, member))

/*
 * 双方向リンクリストの要素を, 指定された型で末尾から順番に取得する
 */
#define list_for_each_entry_reverse(iter, list_head, type, member) \
    for (iter = get_last_list_entry(list_head, type, member); \
         &iter->member != (list_head); \
         iter = get_prev_list_entry(iter, type, member))

/*
 * 双方向リンクリストの要素を, 指定された型で先頭から順番に取得する
 * 取得した要素の削除が可能
 */
#define list_for_each_entry_safe(iter, iter_next, list_head, type, member) \
    for (iter = get_first_list_entry(list_head, type, member), \
         iter_next = get_next_list_entry(iter, type, member); \
         &iter->member != (list_head); \
         iter = iter_next, \
         iter_next = get_next_list_entry(iter_next, type, member))

/*
 * 双方向リンクリストの要素を, 指定された型で末尾から順番に取得する
 * 取得した要素の削除が可能
 */
#define list_for_each_entry_safe_reverse(iter, iter_prev, list_head, type, member) \
    for (iter = get_last_list_entry(list_head, type, member), \
         iter_prev = get_prev_list_entry(iter, type, member); \
         &iter->member != (list_head); \
         iter = iter_prev, \
         iter_prev = get_prev_list_entry(iter_prev, type, member))

#endif /* LINKED_LIST_H */

