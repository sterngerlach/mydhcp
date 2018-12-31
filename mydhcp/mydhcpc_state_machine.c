
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcpc_state_machine.c */

#include <assert.h>
#include <stdlib.h>

#include "mydhcpc_state_machine.h"

/*
 * クライアントの状態を表す列挙体を文字列に変換
 */
const char* dhcp_client_state_to_string(enum dhcp_client_state state)
{
    static const char* client_state_str[] = {
        [DHCP_CLIENT_STATE_NONE]                    = "NONE",
        [DHCP_CLIENT_STATE_INIT]                    = "INIT",
        [DHCP_CLIENT_STATE_WAIT_OFFER]              = "WAIT_OFFER",
        [DHCP_CLIENT_STATE_WAIT_OFFER_RETRY]        = "WAIT_OFFER_RETRY",
        [DHCP_CLIENT_STATE_WAIT_ALLOC_ACK]          = "WAIT_ALLOC_ACK",
        [DHCP_CLIENT_STATE_WAIT_ALLOC_ACK_RETRY]    = "AIT_ALLOC_ACK_RETRY",
        [DHCP_CLIENT_STATE_IP_ADDRESS_IN_USE]       = "IP_ADDRESS_IN_USE",
        [DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK]       = "WAIT_TIME_EXT_ACK",
        [DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK_RETRY] = "WAIT_TIME_EXT_ACK_RETRY",
        [DHCP_CLIENT_STATE_TERMINATE]               = "TERMINATE",
    };

    if (state > DHCP_CLIENT_STATE_TERMINATE)
        return "Unknown";

    return client_state_str[state] ?
           client_state_str[state] : "Unknown";
}

/*
 * クライアントで発生するイベントを表す列挙体を文字列に変換
 */
const char* dhcp_client_event_to_string(enum dhcp_client_event event)
{
    static const char* client_event_str[] = {
        [DHCP_CLIENT_EVENT_NO_EVENT]        = "NO_EVENT",
        [DHCP_CLIENT_EVENT_INVALID_HEADER]  = "INVALID_HEADER",
        [DHCP_CLIENT_EVENT_ACK]             = "ACK",
        [DHCP_CLIENT_EVENT_NACK]            = "NACK",
        [DHCP_CLIENT_EVENT_OFFER]           = "OFFER",
        [DHCP_CLIENT_EVENT_INVALID_OFFER]   = "INVALID_OFFER",
        [DHCP_CLIENT_EVENT_HALF_TTL]        = "HALF_TTL",
        [DHCP_CLIENT_EVENT_SIGHUP]          = "SIGHUP",
        [DHCP_CLIENT_EVENT_TIMEOUT]         = "TIMEOUT",
    };

    if (event > DHCP_CLIENT_EVENT_TIMEOUT)
        return "Unknown";

    return client_event_str[event] ?
           client_event_str[event] : "Unknown";
}

/*
 * クライアントの現在の状態とイベントに対する遷移関数の取得
 */
dhcp_client_event_handler lookup_client_state_transition_table(
    const struct dhcp_client_state_transition* client_state_transition_table,
    enum dhcp_client_state state, enum dhcp_client_event event)
{
    const struct dhcp_client_state_transition* trans;

    for (trans = client_state_transition_table;
        trans->state != DHCP_CLIENT_STATE_NONE; ++trans)
        if (trans->state == state && trans->event == event)
            return trans->handler;

    return NULL;
}

