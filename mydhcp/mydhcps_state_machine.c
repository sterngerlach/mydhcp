
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps_state_machine.c */

#include <stdlib.h>

#include "mydhcps_state_machine.h"

/*
 * サーバの状態遷移を表す配列
 */
struct dhcp_server_state_transition server_state_transition_table[] = {
    {
        .state = DHCP_SERVER_STATE_NONE,
        .event = DHCP_SERVER_EVENT_NONE,
        .handler = NULL
    },
};

/*
 * サーバの状態を表す列挙体を文字列に変換
 */
const char* convert_server_state_to_string(enum dhcp_server_state state)
{
    static const char* server_state_str[] = {
        [DHCP_SERVER_STATE_NONE]
            = "DHCP_SERVER_STATE_NONE",
        [DHCP_SERVER_STATE_INIT]
            = "DHCP_SERVER_STATE_INIT",
        [DHCP_SERVER_STATE_WAIT_REQUEST]
            = "DHCP_SERVER_STATE_WAIT_REQUEST",
        [DHCP_SERVER_STATE_WAIT_REQUEST_RETRY]
            = "DHCP_SERVER_STATE_WAIT_REQUEST_RETRY",
        [DHCP_SERVER_STATE_IP_ADDRESS_IN_USE]
            = "DHCP_SERVER_STATE_IP_ADDRESS_IN_USE",
        [DHCP_SERVER_STATE_TERMINATE]
            = "DHCP_SERVER_STATE_TERMINATE",
    };

    return server_state_str[state];
}

/*
 * サーバで発生するイベントを表す列挙体を文字列に変換
 */
const char* convert_server_event_to_string(enum dhcp_server_event event)
{
    static const char* server_event_str[] = {
        [DHCP_SERVER_EVENT_NONE]
            = "DHCP_SERVER_EVENT_NONE",
        [DHCP_SERVER_EVENT_TIMEOUT]
            = "DHCP_SERVER_EVENT_TIMEOUT",
        [DHCP_SERVER_EVENT_DISCOVER_RECEIVED]
            = "DHCP_SERVER_EVENT_DISCOVER_RECEIVED",
        [DHCP_SERVER_EVENT_ALLOC_REQUEST_RECEIVED]
            = "DHCP_SERVER_EVENT_ALLOC_REQUEST_RECEIVED",
        [DHCP_SERVER_EVENT_INVALID_ALLOC_REQUEST_RECEIVED]
            = "DHCP_SERVER_EVENT_INVALID_ALLOC_REQUEST_RECEIVED",
        [DHCP_SERVER_EVENT_TIME_EXTENSION_REQUEST_RECEIVED]
            = "DHCP_SERVER_EVENT_TIME_EXTENSION_REQUEST_RECEIVED",
        [DHCP_SERVER_EVENT_INVALID_TIME_EXTENSION_REQUEST_RECEIVED]
            = "DHCP_SERVER_EVENT_INVALID_TIME_EXTENSION_REQUEST_RECEIVED",
        [DHCP_SERVER_EVENT_RELEASE_RECEIVED]
            = "DHCP_SERVER_EVENT_RELEASE_RECEIVED",
        [DHCP_SERVER_EVENT_TTL_TIMEOUT]
            = "DHCP_SERVER_EVENT_TTL_TIMEOUT",
    };

    return server_event_str[event];
}

/*
 * サーバの現在の状態とイベントに対する遷移関数の取得
 */
dhcp_server_event_handler lookup_server_state_transition_table(
    enum dhcp_server_state state, enum dhcp_server_event event)
{
    struct dhcp_server_state_transition* trans;

    for (trans = server_state_transition_table;
        trans->state != DHCP_SERVER_STATE_NONE; ++trans)
        if (trans->state == state && trans->event == event)
            return trans->handler;

    return NULL;
}

