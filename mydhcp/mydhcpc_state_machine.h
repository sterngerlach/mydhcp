
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcpc_state_machine.h */

#ifndef MYDHCPC_STATE_MACHINE_H
#define MYDHCPC_STATE_MACHINE_H

#include <stdbool.h>

#include "mydhcp.h"

/*
 * クライアントの状態を表す列挙体
 */
enum dhcp_client_state {
    DHCP_CLIENT_STATE_NONE,
    DHCP_CLIENT_STATE_INIT,
    DHCP_CLIENT_STATE_WAIT_OFFER,
    DHCP_CLIENT_STATE_WAIT_OFFER_RETRY,
    DHCP_CLIENT_STATE_WAIT_ALLOC_ACK,
    DHCP_CLIENT_STATE_WAIT_ALLOC_ACK_RETRY,
    DHCP_CLIENT_STATE_IP_ADDRESS_IN_USE,
    DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK,
    DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK_RETRY,
    DHCP_CLIENT_STATE_TERMINATE,
};

/*
 * クライアントで発生するイベントを表す構造体
 */
enum dhcp_client_event {
    DHCP_CLIENT_EVENT_NO_EVENT,
    DHCP_CLIENT_EVENT_INVALID_HEADER,
    DHCP_CLIENT_EVENT_ACK,
    DHCP_CLIENT_EVENT_NACK,
    DHCP_CLIENT_EVENT_OFFER,
    DHCP_CLIENT_EVENT_INVALID_OFFER,
    DHCP_CLIENT_EVENT_HALF_TTL,
    DHCP_CLIENT_EVENT_SIGHUP,
    DHCP_CLIENT_EVENT_TIMEOUT,
};

/*
 * DHCPクライアントの情報を保持する構造体
 */
struct dhcp_client_context;

/*
 * クライアントが状態とイベントに応じて実行する遷移関数の型
 */
typedef bool (*dhcp_client_event_handler)(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context);

/*
 * クライアントの状態遷移を表す構造体
 */
struct dhcp_client_state_transition {
    enum dhcp_client_state      state;      /* クライアントの現在の状態 */
    enum dhcp_client_event      event;      /* 発生したイベント */
    dhcp_client_event_handler   handler;    /* 現在の状態とイベントに応じて実行される遷移関数 */
};

/*
 * クライアントの状態を表す列挙体を文字列に変換
 */
const char* dhcp_client_state_to_string(enum dhcp_client_state state);

/*
 * クライアントで発生するイベントを表す列挙体を文字列に変換
 */
const char* dhcp_client_event_to_string(enum dhcp_client_event event);

/*
 * クライアントの現在の状態とイベントに対する遷移関数の取得
 */
dhcp_client_event_handler lookup_client_state_transition_table(
    const struct dhcp_client_state_transition* client_state_transition_table,
    enum dhcp_client_state state, enum dhcp_client_event event);

#endif /* MYDHCPC_STATE_MACHINE_H */

