
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps_state_machine.h */

#ifndef MYDHCPS_STATE_MACHINE_H
#define MYDHCPS_STATE_MACHINE_H

#include "mydhcp.h"

/*
 * サーバの状態を表す列挙体(各クライアントごとに状態を保持)
 */
enum dhcp_server_state {
    DHCP_SERVER_STATE_NONE,
    DHCP_SERVER_STATE_INIT,
    DHCP_SERVER_STATE_WAIT_REQUEST,
    DHCP_SERVER_STATE_WAIT_REQUEST_RETRY,
    DHCP_SERVER_STATE_IP_ADDRESS_IN_USE,
    DHCP_SERVER_STATE_TERMINATE,
};

/*
 * サーバで発生するイベントを表す列挙体
 */
enum dhcp_server_event {
    DHCP_SERVER_EVENT_NONE,
    DHCP_SERVER_EVENT_INVALID_HEADER,
    DHCP_SERVER_EVENT_DISCOVER,
    DHCP_SERVER_EVENT_ALLOC_REQUEST,
    DHCP_SERVER_EVENT_INVALID_ALLOC_REQUEST,
    DHCP_SERVER_EVENT_TIME_EXT_REQUEST,
    DHCP_SERVER_EVENT_INVALID_TIME_EXT_REQUEST,
    DHCP_SERVER_EVENT_RELEASE,
    DHCP_SERVER_EVENT_TTL_TIMEOUT,
};

/*
 * 接続されたクライアントの情報を保持する構造体
 */
struct dhcp_client_list_entry;

/*
 * サーバが状態とイベントに応じて実行する遷移関数の型
 */
typedef void (*dhcp_server_event_handler)(
    const struct dhcp_header* header, struct dhcp_client_list_entry* client);

/*
 * サーバの状態遷移を表す構造体
 */
struct dhcp_server_state_transition {
    enum dhcp_server_state      state;      /* サーバの現在の状態 */
    enum dhcp_server_event      event;      /* 発生したイベント */
    dhcp_server_event_handler   handler;    /* 現在の状態とイベントに対して実行される遷移関数 */
};

/*
 * サーバの状態遷移を表す配列
 */
extern struct dhcp_server_state_transition server_state_transition_table[];

/*
 * サーバの状態を表す列挙体を文字列に変換
 */
const char* dhcp_server_state_to_string(enum dhcp_server_state state);

/*
 * サーバで発生するイベントを表す列挙体を文字列に変換
 */
const char* dhcp_server_event_to_string(enum dhcp_server_event event);

/*
 * サーバの現在の状態とイベントに対する遷移関数の取得
 */
dhcp_server_event_handler lookup_server_state_transition_table(
    enum dhcp_server_state state, enum dhcp_server_event event);

#endif /* MYDHCPS_STATE_MACHINE_H */

