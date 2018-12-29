
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcpc.c */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mydhcp.h"
#include "mydhcpc_context.h"
#include "mydhcpc_signal.h"
#include "mydhcpc_state_machine.h"
#include "util.h"

/*
 * サーバとの通信の終了処理
 */
bool on_disconnect_server(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    assert(context != NULL);
    assert(context->client_sock >= 0);

    (void)received_header;

    /* ソケットのクローズ */
    if (close(context->client_sock) < 0) {
        print_error(__func__,
                    "close() failed: %s, failed to close client socket\n",
                    strerror(errno));
        return false;
    }

    print_message(__func__,
                  "client state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  dhcp_client_state_to_string(context->state),
                  dhcp_client_state_to_string(DHCP_CLIENT_STATE_TERMINATE));
    
    /* クライアントの情報を更新 */
    context->state = DHCP_CLIENT_STATE_TERMINATE;
    context->ttl_counter = UINT16_C(0);
    context->client_sock = -1;
    context->server_addr.s_addr = htonl(0);
    context->addr.s_addr = htonl(0);
    context->mask.s_addr = htonl(0);
    context->ttl = UINT16_C(0);
    
    /* アプリケーションを終了 */
    app_exit = 1;

    return true;
}

/*
 * 初期状態で実行される処理
 */
bool on_init(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    int client_sock;
    ssize_t send_bytes;
    struct dhcp_header header;
    struct sockaddr_in server_addr;

    assert(received_header == NULL);
    assert(context != NULL);
    assert(context->client_sock < 0);

    /* ソケットの作成 */
    if ((client_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        print_error(__func__,
                    "socket() failed: %s, failed to create client socket\n",
                    strerror(errno));
        return false;
    }

    print_message(__func__,
                  "client socket (fd: %d) successfully created\n",
                  client_sock);
    
    /* サーバのアドレス構造体を作成 */
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = context->server_addr.s_addr;

    /* DISCOVERメッセージを作成 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_DISCOVER;

    /* DISCOVERメッセージを送信 */
    send_bytes = sendto(client_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&server_addr, sizeof(server_addr));

    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to server %s\n",
                    strerror(errno), inet_ntoa(context->server_addr));
        on_disconnect_server(NULL, context);
        return false;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send full dhcp header to server %s\n",
                    inet_ntoa(context->server_addr));
        on_disconnect_server(NULL, context);
        return false;
    }

    print_message(__func__, "dhcp header has been sent to server %s: ",
                  inet_ntoa(context->server_addr));
    dump_dhcp_header(stderr, &header);

    print_message(__func__,
                  "client state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  dhcp_client_state_to_string(context->state),
                  dhcp_client_state_to_string(DHCP_CLIENT_STATE_WAIT_OFFER));

    /* クライアントの情報を更新 */
    context->state = DHCP_CLIENT_STATE_WAIT_OFFER;
    context->ttl_counter = TIMEOUT_VALUE;
    context->client_sock = client_sock;

    return true;
}

/*
 * OFFERメッセージ(正常)を受信した際の処理
 */
bool on_offer_received(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    uint16_t ttl;
    struct in_addr addr;
    struct in_addr mask;
    ssize_t send_bytes;
    struct dhcp_header header;
    struct sockaddr_in server_addr;
    
    assert(received_header != NULL);
    assert(received_header->code == DHCP_HEADER_CODE_OFFER_OK);
    assert(context != NULL);

    /* TTLと, サーバから配布されたIPアドレス及びサブネットマスクを取得 */
    ttl = received_header->ttl;
    addr.s_addr = received_header->addr;
    mask.s_addr = received_header->mask;

    /* サーバのアドレス構造体を作成 */
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = context->server_addr.s_addr;

    /* REQUESTメッセージを作成 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_REQUEST;
    header.code = DHCP_HEADER_CODE_REQUEST_ALLOC;
    header.ttl = ttl;
    header.addr = addr.s_addr;
    header.mask = mask.s_addr;

    /* REQUESTメッセージを送信 */
    send_bytes = sendto(context->client_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&server_addr, sizeof(server_addr));

    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to server %s\n",
                    strerror(errno), inet_ntoa(context->server_addr));
        on_disconnect_server(NULL, context);
        return false;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send full dhcp header to server %s\n",
                    inet_ntoa(context->server_addr));
        on_disconnect_server(NULL, context);
        return false;
    }

    print_message(__func__, "dhcp header has been sent to server %s: ",
                  inet_ntoa(context->server_addr));
    dump_dhcp_header(stderr, &header);
    
    print_message(__func__,
                  "client state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  dhcp_client_state_to_string(context->state),
                  dhcp_client_state_to_string(DHCP_CLIENT_STATE_WAIT_ALLOC_ACK));

    /* クライアントの情報を更新 */
    /* ttl_counterメンバにはタイムアウトまでの秒数を設定 */
    /* addr, maskメンバには割り当てられる予定のIPアドレスとサブネットマスクを設定 */
    /* ttlメンバにはサーバから受信したデフォルトのIPアドレス使用期限を設定 */
    context->state = DHCP_CLIENT_STATE_WAIT_ALLOC_ACK;
    context->ttl_counter = TIMEOUT_VALUE;
    context->addr.s_addr = addr.s_addr;
    context->mask.s_addr = mask.s_addr;
    context->ttl = ttl;

    return true;
}

/*
 * OFFERメッセージ(エラー)を受信した際の処理
 */
bool on_offer_error_received(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    assert(received_header != NULL);
    assert(received_header->code == DHCP_HEADER_CODE_OFFER_NG);
    assert(context != NULL);

    print_message(__func__,
                  ANSI_ESCAPE_COLOR_RED
                  "available ip address and mask not found"
                  ANSI_ESCAPE_COLOR_RESET "\n");
    
    /* サーバとの通信を終了 */
    return on_disconnect_server(received_header, context);
}

/*
 * OFFERメッセージの受信がタイムアウトした際の処理
 */
bool on_offer_timeout(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    ssize_t send_bytes;
    struct dhcp_header header;
    struct sockaddr_in server_addr;

    assert(received_header == NULL);
    assert(context != NULL);
    
    /* サーバのアドレス構造体を作成 */
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = context->server_addr.s_addr;

    /* DISCOVERメッセージを作成 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_DISCOVER;

    /* DISCOVERメッセージを送信 */
    send_bytes = sendto(context->client_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&server_addr, sizeof(server_addr));

    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to server %s\n",
                    strerror(errno), inet_ntoa(context->server_addr));
        on_disconnect_server(received_header, context);
        return false;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send full dhcp header to server %s\n",
                    inet_ntoa(context->server_addr));
        on_disconnect_server(received_header, context);
        return false;
    }
    
    print_message(__func__, "dhcp header has been sent to server %s: ",
                  inet_ntoa(context->server_addr));
    dump_dhcp_header(stderr, &header);

    print_message(__func__,
                  "client state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  dhcp_client_state_to_string(context->state),
                  dhcp_client_state_to_string(DHCP_CLIENT_STATE_WAIT_OFFER_RETRY));

    /* クライアントの情報を更新 */
    context->state = DHCP_CLIENT_STATE_WAIT_OFFER_RETRY;
    context->ttl_counter = TIMEOUT_VALUE;

    return true;
}

/*
 * ACKメッセージ(IPアドレスの割り当て要求)を受信した際の処理
 */
bool on_alloc_ack_received(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    uint16_t ttl;
    struct in_addr addr;
    struct in_addr mask;
    char addr_str[INET_ADDRSTRLEN];
    char mask_str[INET_ADDRSTRLEN];

    assert(received_header != NULL);
    assert(received_header->code == DHCP_HEADER_CODE_ACK_OK);
    assert(context != NULL);
    assert(ntohs(context->ttl) > 0);

    /* TTLと, サーバから配布されたIPアドレス及びサブネットマスクを取得 */
    ttl = received_header->ttl;
    addr.s_addr = received_header->addr;
    mask.s_addr = received_header->mask;
    
    if (inet_ntop(AF_INET, &addr, addr_str, sizeof(addr_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *addr_str = '\0';
    }

    if (inet_ntop(AF_INET, &mask, mask_str, sizeof(mask_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *mask_str = '\0';
    }

    print_message(__func__,
                  ANSI_ESCAPE_COLOR_BLUE
                  "ip address %s (mask: %s, ttl: %" PRIu16 ") is assigned to client"
                  ANSI_ESCAPE_COLOR_RESET "\n",
                  addr_str, mask_str, ntohs(context->ttl));

    print_message(__func__,
                  "client state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  dhcp_client_state_to_string(context->state),
                  dhcp_client_state_to_string(DHCP_CLIENT_STATE_IP_ADDRESS_IN_USE));

    /* クライアントの情報を更新 */
    context->state = DHCP_CLIENT_STATE_IP_ADDRESS_IN_USE;
    context->ttl_counter = ntohs(ttl);
    context->addr.s_addr = addr.s_addr;
    context->mask.s_addr = mask.s_addr;
    context->ttl = ttl;
    
    return true;
}

/*
 * NACKメッセージ(IPアドレスの割り当て要求)を受信した際の処理
 */
bool on_alloc_negative_ack_received(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    char addr_str[INET_ADDRSTRLEN];
    char mask_str[INET_ADDRSTRLEN];

    assert(received_header != NULL);
    assert(received_header->code == DHCP_HEADER_CODE_ACK_NG);
    assert(context != NULL);
    
    if (inet_ntop(AF_INET, &context->addr, addr_str, sizeof(addr_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *addr_str = '\0';
    }

    if (inet_ntop(AF_INET, &context->mask, mask_str, sizeof(mask_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *mask_str = '\0';
    }

    print_message(__func__,
                  ANSI_ESCAPE_COLOR_RED
                  "ip address %s (mask: %s, ttl: %" PRIu16 ") could not be assigned, "
                  "because of invalid arguments specified in request message"
                  ANSI_ESCAPE_COLOR_RESET "\n",
                  addr_str, mask_str, ntohs(context->ttl));

    /* サーバとの通信を終了 */
    return on_disconnect_server(received_header, context);
}

/*
 * ACKメッセージ(IPアドレスの割り当て要求)の受信がタイムアウトした際の処理
 */
bool on_alloc_ack_timeout(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    ssize_t send_bytes;
    struct dhcp_header header;
    struct sockaddr_in server_addr;

    assert(received_header == NULL);
    assert(context != NULL);
    
    /* サーバのアドレス構造体を作成 */
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = context->server_addr.s_addr;

    /* REQUESTメッセージを作成 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_REQUEST;
    header.code = DHCP_HEADER_CODE_REQUEST_ALLOC;
    header.ttl = context->ttl;
    header.addr = context->addr.s_addr;
    header.mask = context->mask.s_addr;

    /* REQUESTメッセージを送信 */
    send_bytes = sendto(context->client_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&server_addr, sizeof(server_addr));

    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to server %s\n",
                    strerror(errno), inet_ntoa(context->server_addr));
        on_disconnect_server(received_header, context);
        return false;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send full dhcp header to server %s\n",
                    inet_ntoa(context->server_addr));
        on_disconnect_server(received_header, context);
        return false;
    }
    
    print_message(__func__, "dhcp header has been sent to server %s: ",
                  inet_ntoa(context->server_addr));
    dump_dhcp_header(stderr, &header);
    
    print_message(__func__,
                  "client state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  dhcp_client_state_to_string(context->state),
                  dhcp_client_state_to_string(DHCP_CLIENT_STATE_WAIT_ALLOC_ACK_RETRY));

    /* クライアントの情報を更新 */
    context->state = DHCP_CLIENT_STATE_WAIT_ALLOC_ACK_RETRY;
    context->ttl_counter = TIMEOUT_VALUE;
    
    return true;
}

/*
 * IPアドレスの使用期限の半分が経過した際の処理
 */
bool on_half_ttl_passed(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    ssize_t send_bytes;
    struct dhcp_header header;
    struct sockaddr_in server_addr;

    assert(received_header == NULL);
    assert(context != NULL);
    
    /* サーバのアドレス構造体を作成 */
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = context->server_addr.s_addr;

    /* REQUESTメッセージを作成 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_REQUEST;
    header.code = DHCP_HEADER_CODE_REQUEST_TIME_EXT;
    header.ttl = context->ttl;
    header.addr = context->addr.s_addr;
    header.mask = context->mask.s_addr;

    /* REQUESTメッセージを送信 */
    send_bytes = sendto(context->client_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&server_addr, sizeof(server_addr));

    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to server %s\n",
                    strerror(errno), inet_ntoa(context->server_addr));
        on_disconnect_server(received_header, context);
        return false;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send full dhcp header to server %s\n",
                    inet_ntoa(context->server_addr));
        on_disconnect_server(received_header, context);
        return false;
    }

    print_message(__func__, "dhcp header has been sent to server %s: ",
                  inet_ntoa(context->server_addr));
    dump_dhcp_header(stderr, &header);
    
    print_message(__func__,
                  "client state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  dhcp_client_state_to_string(context->state),
                  dhcp_client_state_to_string(DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK));
    
    /* クライアントの情報を更新 */
    context->state = DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK;
    context->ttl_counter = TIMEOUT_VALUE;
    
    return true;
}

/*
 * SIGHUPシグナルを受信した際の処理
 */
bool on_sighup(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    ssize_t send_bytes;
    struct dhcp_header header;
    struct sockaddr_in server_addr;
    
    assert(received_header == NULL);
    assert(context != NULL);

    print_message(__func__, "signal SIGHUP has been received\n");

    /* サーバのアドレス構造体を作成 */
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = context->server_addr.s_addr;

    /* RELEASEメッセージの作成 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_RELEASE;
    header.addr = context->addr.s_addr;
    
    /* REQUESTメッセージを送信 */
    send_bytes = sendto(context->client_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to server %s\n",
                    strerror(errno), inet_ntoa(context->server_addr));
        return false;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send full dhcp header to server %s\n",
                    inet_ntoa(context->server_addr));
        return false;
    }

    print_message(__func__, "dhcp header has been sent to server %s: ",
                  inet_ntoa(context->server_addr));
    dump_dhcp_header(stderr, &header);
    
    /* サーバとの通信を終了 */
    return on_disconnect_server(NULL, context);
}

/*
 * ACKメッセージ(使用期間の延長要求)を受信した際の処理
 */
bool on_time_ext_ack_received(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    uint16_t ttl;
    struct in_addr addr;
    struct in_addr mask;
    char addr_str[INET_ADDRSTRLEN];
    char mask_str[INET_ADDRSTRLEN];

    assert(received_header != NULL);
    assert(received_header->code == DHCP_HEADER_CODE_ACK_OK);
    assert(context != NULL);
    
    /* TTLと, サーバから配布されたIPアドレス及びサブネットマスクを取得 */
    ttl = received_header->ttl;
    addr.s_addr = received_header->addr;
    mask.s_addr = received_header->mask;

    if (inet_ntop(AF_INET, &addr, addr_str, sizeof(addr_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *addr_str = '\0';
    }

    if (inet_ntop(AF_INET, &mask, mask_str, sizeof(mask_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *mask_str = '\0';
    }

    print_message(__func__,
                  ANSI_ESCAPE_COLOR_BLUE
                  "time extension request for ip address %s "
                  "(mask: %s, ttl: %" PRIu16 ") is acknowledged"
                  ANSI_ESCAPE_COLOR_RESET "\n",
                  addr_str, mask_str, ttl);

    print_message(__func__,
                  "client state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  dhcp_client_state_to_string(context->state),
                  dhcp_client_state_to_string(DHCP_CLIENT_STATE_IP_ADDRESS_IN_USE));

    /* クライアントの情報を更新 */
    context->state = DHCP_CLIENT_STATE_IP_ADDRESS_IN_USE;
    context->ttl_counter = ntohs(ttl);
    context->addr.s_addr = addr.s_addr;
    context->mask.s_addr = mask.s_addr;
    context->ttl = ttl;

    return true;
}

/*
 * NACKメッセージ(使用期間の延長要求)を受信した際の処理
 */
bool on_time_ext_negative_ack_received(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    char addr_str[INET_ADDRSTRLEN];
    char mask_str[INET_ADDRSTRLEN];

    assert(received_header != NULL);
    assert(received_header->code == DHCP_HEADER_CODE_ACK_NG);
    assert(context != NULL);

    if (inet_ntop(AF_INET, &context->addr, addr_str, sizeof(addr_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *addr_str = '\0';
    }

    if (inet_ntop(AF_INET, &context->mask, mask_str, sizeof(mask_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *mask_str = '\0';
    }

    print_message(__func__,
                  ANSI_ESCAPE_COLOR_RED
                  "time extension request for ip address %s "
                  "(mask: %s, ttl: %" PRIu16 ") is denied"
                  ANSI_ESCAPE_COLOR_RESET "\n",
                  addr_str, mask_str, ntohs(context->ttl));
    
    /* サーバとの通信を終了 */
    return on_disconnect_server(received_header, context);
}

/*
 * ACKメッセージ(使用期間の延長要求)の受信がタイムアウトした際の処理
 */
bool on_time_ext_ack_timeout(
    const struct dhcp_header* received_header,
    struct dhcp_client_context* context)
{
    ssize_t send_bytes;
    struct dhcp_header header;
    struct sockaddr_in server_addr;

    assert(received_header == NULL);
    assert(context != NULL);

    /* サーバのアドレス構造体を作成 */
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = context->server_addr.s_addr;

    /* REQUESTメッセージを作成 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_REQUEST;
    header.code = DHCP_HEADER_CODE_REQUEST_TIME_EXT;
    header.ttl = context->ttl;
    header.addr = context->addr.s_addr;
    header.mask = context->mask.s_addr;

    /* REQUESTメッセージを送信 */
    send_bytes = sendto(context->client_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&server_addr, sizeof(server_addr));

    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to server %s\n",
                    strerror(errno), inet_ntoa(context->server_addr));
        on_disconnect_server(received_header, context);
        return false;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send full dhcp header to server %s\n",
                    inet_ntoa(context->server_addr));
        on_disconnect_server(received_header, context);
        return false;
    }

    print_message(__func__, "dhcp header has been sent to server %s: ",
                  inet_ntoa(context->server_addr));
    dump_dhcp_header(stderr, &header);
    
    print_message(__func__,
                  "client state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  dhcp_client_state_to_string(context->state),
                  dhcp_client_state_to_string(DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK_RETRY));
    
    /* クライアントの情報を更新 */
    context->state = DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK_RETRY;
    context->ttl_counter = TIMEOUT_VALUE;

    return true;
}

/*
 * クライアントの状態遷移を表す配列
 */
struct dhcp_client_state_transition client_state_transition_table[] = {
    {
        .state = DHCP_CLIENT_STATE_INIT,
        .event = DHCP_CLIENT_EVENT_NO_EVENT,
        .handler = on_init
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_OFFER,
        .event = DHCP_CLIENT_EVENT_OFFER,
        .handler = on_offer_received
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_OFFER,
        .event = DHCP_CLIENT_EVENT_INVALID_OFFER,
        .handler = on_offer_error_received
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_OFFER,
        .event = DHCP_CLIENT_EVENT_TIMEOUT,
        .handler = on_offer_timeout
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_OFFER_RETRY,
        .event = DHCP_CLIENT_EVENT_OFFER,
        .handler = on_offer_received
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_OFFER_RETRY,
        .event = DHCP_CLIENT_EVENT_INVALID_OFFER,
        .handler = on_offer_error_received
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_OFFER_RETRY,
        .event = DHCP_CLIENT_EVENT_TIMEOUT,
        .handler = on_disconnect_server
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_ALLOC_ACK,
        .event = DHCP_CLIENT_EVENT_ACK,
        .handler = on_alloc_ack_received
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_ALLOC_ACK,
        .event = DHCP_CLIENT_EVENT_NACK,
        .handler = on_alloc_negative_ack_received
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_ALLOC_ACK,
        .event = DHCP_CLIENT_EVENT_TIMEOUT,
        .handler = on_alloc_ack_timeout
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_ALLOC_ACK_RETRY,
        .event = DHCP_CLIENT_EVENT_ACK,
        .handler = on_alloc_ack_received
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_ALLOC_ACK_RETRY,
        .event = DHCP_CLIENT_EVENT_NACK,
        .handler = on_alloc_negative_ack_received
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_ALLOC_ACK_RETRY,
        .event = DHCP_CLIENT_EVENT_TIMEOUT,
        .handler = on_disconnect_server
    },
    {
        .state = DHCP_CLIENT_STATE_IP_ADDRESS_IN_USE,
        .event = DHCP_CLIENT_EVENT_HALF_TTL,
        .handler = on_half_ttl_passed
    },
    {
        .state = DHCP_CLIENT_STATE_IP_ADDRESS_IN_USE,
        .event = DHCP_CLIENT_EVENT_SIGHUP,
        .handler = on_sighup
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK,
        .event = DHCP_CLIENT_EVENT_ACK,
        .handler = on_time_ext_ack_received
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK,
        .event = DHCP_CLIENT_EVENT_NACK,
        .handler = on_time_ext_negative_ack_received
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK,
        .event = DHCP_CLIENT_EVENT_TIMEOUT,
        .handler = on_time_ext_ack_timeout
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK_RETRY,
        .event = DHCP_CLIENT_EVENT_ACK,
        .handler = on_time_ext_ack_received
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK_RETRY,
        .event = DHCP_CLIENT_EVENT_NACK,
        .handler = on_time_ext_negative_ack_received
    },
    {
        .state = DHCP_CLIENT_STATE_WAIT_TIME_EXT_ACK_RETRY,
        .event = DHCP_CLIENT_EVENT_TIMEOUT,
        .handler = on_disconnect_server
    },
};

/*
 * クライアントで発生したイベントの処理
 */
bool handle_event(
    const struct dhcp_header* header,
    struct dhcp_client_context* context,
    enum dhcp_client_event event)
{
    dhcp_client_event_handler handler;

    assert(context != NULL);

    /* 現在の状態とイベントに対応する遷移関数を取得 */
    handler = lookup_client_state_transition_table(client_state_transition_table,
                                                   context->state, event);

    /* 状態とイベントに対応する遷移関数がない場合 */
    if (handler == NULL) {
        print_error(__func__,
                    ANSI_ESCAPE_COLOR_RED
                    "lookup_client_state_transition_table() failed: "
                    "corresponding event handler not found, therefore "
                    "connection to pseudo-dhcp server %s will be shut down"
                    ANSI_ESCAPE_COLOR_RESET "\n",
                    inet_ntoa(context->server_addr));
        /* サーバとの接続を終了 */
        on_disconnect_server(NULL, context);
        return false;
    }

    /* 対応する遷移関数を実行 */
    return (*handler)(header, context);
}

/*
 * インターバルタイマーのタイムアウトの処理
 */
bool handle_alrm(
    struct dhcp_client_context* context)
{
    char addr_str[INET_ADDRSTRLEN];
    char mask_str[INET_ADDRSTRLEN];

    /* IPアドレスの使用期限(またはタイムアウト)までの残り時間を更新 */
    if (context->ttl_counter > 0)
        context->ttl_counter--;

    /* 残り時間を表示 */
    if (context->state == DHCP_CLIENT_STATE_IP_ADDRESS_IN_USE) {
        if (inet_ntop(AF_INET, &context->addr, addr_str, sizeof(addr_str)) == NULL) {
            print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
            *addr_str = '\0';
        }

        if (inet_ntop(AF_INET, &context->mask, mask_str, sizeof(mask_str)) == NULL) {
            print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
            *mask_str = '\0';
        }

        print_message(__func__,
                      "ttl of ip address "
                      ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET
                      " (mask: " ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET
                      "): " ANSI_ESCAPE_COLOR_GREEN "%" PRIu16 ANSI_ESCAPE_COLOR_RESET "\n",
                      addr_str, mask_str, context->ttl_counter);
    }

    /* IPアドレスの使用期限の半分が経過 */
    if (ntohs(context->ttl) != 0 &&
        context->ttl_counter <= ntohs(context->ttl) / 2 &&
        context->state == DHCP_CLIENT_STATE_IP_ADDRESS_IN_USE)
        return handle_event(NULL, context, DHCP_CLIENT_EVENT_HALF_TTL);
    
    /* IPアドレスの使用期限切れ, またはタイムアウトの発生 */
    if (context->ttl_counter == 0)
        return handle_event(NULL, context, DHCP_CLIENT_EVENT_TIMEOUT);

    return true;
}

/*
 * シグナルSIGHUPの処理
 */
bool handle_hup(
    struct dhcp_client_context* context)
{
    return handle_event(NULL, context, DHCP_CLIENT_EVENT_SIGHUP);
}

/*
 * クライアントが受信したDHCPヘッダの処理
 * DHCPヘッダの内容からイベントを判定
 */
bool handle_dhcp_header(
    ssize_t recv_bytes,
    const struct dhcp_header* header,
    struct dhcp_client_context* context)
{
    /* 受信したデータサイズとDHCPヘッダのサイズが異なる場合 */
    if (recv_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "invalid dhcp header: "
                    "expected length was %zd bytes, but %zd were received\n",
                    sizeof(struct dhcp_header), recv_bytes);
        handle_event(header, context, DHCP_CLIENT_EVENT_INVALID_HEADER);
        return false;
    }

    /* 受信したDHCPヘッダを出力 */
    print_message(__func__, "dhcp header received from server %s: ",
                  inet_ntoa(context->server_addr));
    dump_dhcp_header(stderr, header);

    /* メッセージの種別に応じてイベントを判別 */
    switch (header->type) {
        case DHCP_HEADER_TYPE_OFFER:
            /* OFFERメッセージである場合 */
            switch (header->code) {
                case DHCP_HEADER_CODE_OFFER_OK:
                    return handle_event(header, context, DHCP_CLIENT_EVENT_OFFER);
                case DHCP_HEADER_CODE_OFFER_NG:
                    return handle_event(header, context, DHCP_CLIENT_EVENT_INVALID_OFFER);
                default:
                    /* プロトコルに整合しないDHCPヘッダの処理 */
                    return handle_event(header, context, DHCP_CLIENT_EVENT_INVALID_HEADER);
            }
            
            /* ここには到達できないはず */
            return false;

        case DHCP_HEADER_TYPE_ACK:
            /* ACKメッセージである場合 */
            switch (header->code) {
                case DHCP_HEADER_CODE_ACK_OK:
                    return handle_event(header, context, DHCP_CLIENT_EVENT_ACK);
                case DHCP_HEADER_CODE_ACK_NG:
                    return handle_event(header, context, DHCP_CLIENT_EVENT_NACK);
                default:
                    /* プロトコルに整合しないDHCPヘッダの処理 */
                    return handle_event(header, context, DHCP_CLIENT_EVENT_INVALID_HEADER);
            }

            /* ここには到達できないはず */
            return false;
    }

    /* それ以外のタイプである場合はエラー */
    print_error(__func__,
                ANSI_ESCAPE_COLOR_RED
                "invalid dhcp header: unknown message type: %s"
                ANSI_ESCAPE_COLOR_RESET "\n",
                dhcp_header_type_to_string(header->type));
    return handle_event(header, context, DHCP_CLIENT_EVENT_INVALID_HEADER);
}

/*
 * クライアントの実行
 */
bool run_dhcp_client(struct dhcp_client_context* context)
{
    ssize_t recv_bytes;
    int recv_errno;
    struct dhcp_header recv_header;
    struct sockaddr_in server_addr;
    socklen_t addr_len;

    /* シグナルSIGALRMをブロック */
    if (!block_sigalrm()) {
        print_error(__func__, "block_sigalrm() failed\n");
        return false;
    }
    
    /* シグナルハンドラを設定 */
    if (!setup_signal_handlers()) {
        print_error(__func__, "setup_signal_handlers() failed\n");
        return false;
    }

    /* インターバルタイマーの準備 */
    if (!setup_interval_timer()) {
        print_error(__func__, "setup_interval_timer() failed\n");
        return false;
    }

    /* 初期状態での処理を実行 */
    if (!handle_event(NULL, context, DHCP_CLIENT_EVENT_NO_EVENT)) {
        print_error(__func__, "failed to send discover message to server %s\n",
                    inet_ntoa(context->server_addr));
        return false;
    }

    /* サーバのアドレス構造体を作成 */
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = context->server_addr.s_addr;

    /* クライアントのメインループの実行 */
    is_sigalrm_handled = 0;
    app_exit = 0;

    while (!app_exit) {
        /* シグナルSIGALRMのブロックを解除 */
        if (!unblock_sigalrm()) {
            print_error(__func__, "unblock_sigalrm() failed\n");
            /* サーバとの通信を終了 */
            on_disconnect_server(NULL, context);
            return false;
        }

        /* DHCPヘッダを受信 */
        addr_len = sizeof(server_addr);
        recv_bytes = recvfrom(context->client_sock, &recv_header, sizeof(recv_header), 0,
                              (struct sockaddr*)&server_addr, &addr_len);
        recv_errno = errno;

        /* シグナルSIGALRMを再びブロック */
        if (!block_sigalrm()) {
            print_error(__func__, "block_sigalrm() failed\n");
            /* サーバとの通信を終了 */
            on_disconnect_server(NULL, context);
            return false;
        }

        if (recv_bytes < 0) {
            if (recv_errno == EINTR && is_sigalrm_handled == 1) {
                /* シグナルSIGALRMの処理 */
                if (!handle_alrm(context)) {
                    print_error(__func__, "handle_alrm() failed\n");
                    return false;
                }
                /* フラグを元に戻す */
                is_sigalrm_handled = 0;
            } else if (recv_errno == EINTR && app_exit == 1) {
                /* シグナルSIGHUPの処理 */
                if (!handle_hup(context)) {
                    print_error(__func__, "handle_hup() failed\n");
                    return false;
                }
            } else {
                /* その他のエラーの場合 */
                print_error(__func__, "recvfrom() failed: %s\n", strerror(recv_errno));
                /* サーバとの通信を終了 */
                on_disconnect_server(NULL, context);
                return false;
            }
        } else {
            /* 受信したDHCPヘッダの処理 */
            if (!handle_dhcp_header(recv_bytes, &recv_header, context)) {
                print_error(__func__, "handle_dhcp_header() failed\n");
                return false;
            }
        }
    }

    return true;
}

int main(int argc, char** argv)
{
    struct dhcp_client_context context;
    bool exit_status;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <server IP address>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    /* クライアントの情報を初期化 */
    if (!initialize_dhcp_client_context(&context, argv[1])) {
        print_error(__func__, "initialize_dhcp_client_context() failed\n");
        return EXIT_FAILURE;
    }

    /* クライアントの実行 */
    exit_status = run_dhcp_client(&context);

    /* クライアントの情報を破棄 */
    free_dhcp_client_context(&context);

    return exit_status ? EXIT_SUCCESS : EXIT_FAILURE;
}

