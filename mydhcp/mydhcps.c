
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps.c */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mydhcp.h"
#include "mydhcps_client_list.h"
#include "mydhcps_config.h"
#include "mydhcps_signal.h"
#include "mydhcps_state_machine.h"
#include "util.h"

/*
 * 疑似DHCPサーバの設定
 */
static struct dhcp_server_config dhcp_server_config;

/*
 * 疑似DHCPサーバが割り当て可能なIPアドレスのリスト
 */
static struct ip_addr_list_entry* available_ip_addr_list_head;

/*
 * 接続されたクライアントのリスト
 */
static struct dhcp_client_list_entry dhcp_client_list_head;

/*
 * クライアントとの接続の終了処理
 */
void on_disconnect_client(
    const struct dhcp_header* received_header,
    struct dhcp_client_list_entry* client,
    int server_sock)
{
    struct in_addr client_addr;
    in_port_t client_port;

    assert(client != NULL);

    (void)received_header;
    (void)server_sock;
    
    /* クライアントにIPアドレスが割り当てられている場合は,
     * 割り当てたIPアドレスとサブネットマスクの組を元に戻す */
    if (is_ip_address_assigned_to_dhcp_client(client)) {
        if (!recall_ip_addr(available_ip_addr_list_head, client->addr, client->mask))
            print_error(__func__,
                        "recall_ip_addr() failed: ip address %s will never be available\n",
                        inet_ntoa(client->addr));
        else
            print_message(__func__,
                          "recall_ip_addr() succeeded: ip address %s is available again\n",
                          inet_ntoa(client->addr));
    }

    print_message(__func__,
                  "client " ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET
                  " state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  inet_ntoa(client->id),
                  dhcp_server_state_to_string(client->state),
                  dhcp_server_state_to_string(DHCP_SERVER_STATE_TERMINATE));

    client_addr = client->id;
    client_port = client->port;

    remove_dhcp_client(client);

    /* クライアントの情報をリンクリストから削除 */
    print_message(__func__,
                  ANSI_ESCAPE_COLOR_BLUE
                  "remove_dhcp_client() succeeded: client %s (port: %" PRIu16 ") disconnected"
                  ANSI_ESCAPE_COLOR_RESET "\n",
                  inet_ntoa(client_addr), client_port);

    return;
}

/*
 * DISCOVERメッセージを受信した際の処理
 */
void on_discover_received(
    const struct dhcp_header* received_header,
    struct dhcp_client_list_entry* client,
    int server_sock)
{
    struct dhcp_header header;
    struct in_addr addr;
    struct in_addr mask;
    ssize_t send_bytes;
    struct sockaddr_in client_addr;

    char addr_str[INET_ADDRSTRLEN];
    char mask_str[INET_ADDRSTRLEN];

    assert(received_header != NULL);
    assert(client != NULL);

    /* クライアントのアドレス構造体を作成 */
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = client->port;
    client_addr.sin_addr = client->id;

    /* 割り当て可能なIPアドレスを取得 */
    /* 割り当て可能なIPアドレスがない場合 */
    if (!get_available_ip_addr(available_ip_addr_list_head, &addr, &mask)) {
        print_error(__func__,
                    ANSI_ESCAPE_COLOR_RED
                    "get_available_ip_addr() failed: no available ip address found"
                    ANSI_ESCAPE_COLOR_RESET "\n");

        /* OFFERメッセージを作成 */
        memset(&header, 0, sizeof(struct dhcp_header));
        header.type = DHCP_HEADER_TYPE_OFFER;
        header.code = DHCP_HEADER_CODE_OFFER_NG;
        
        /* OFFERメッセージを送信 */
        send_bytes = sendto(server_sock, &header, sizeof(struct dhcp_header), 0,
                            (struct sockaddr*)&client_addr, sizeof(client_addr));
        
        /* 送信に失敗した場合 */
        if (send_bytes < 0) {
            print_error(__func__,
                        "sendto() failed: %s, could not send dhcp header to client %s\n",
                        strerror(errno), inet_ntoa(client->id));
            /* クライアントとの接続を終了 */
            on_disconnect_client(received_header, client, server_sock);
            return;
        }
        
        /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
        if (send_bytes != sizeof(struct dhcp_header)) {
            print_error(__func__,
                        "could not send full dhcp header to client %s\n",
                        inet_ntoa(client->id));
            /* クライアントとの接続を終了 */
            on_disconnect_client(received_header, client, server_sock);
            return;
        }

        print_message(__func__, "dhcp header has been sent to client %s: ", inet_ntoa(client->id));
        dump_dhcp_header(stderr, &header);

        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);

        return;
    }
    
    /* IPアドレス(ネットワークバイトオーダー)を文字列に変換 */
    if (inet_ntop(AF_INET, &addr, addr_str, sizeof(addr_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *addr_str = '\0';
    }
    
    /* サブネットマスク(ネットワークバイトオーダー)を文字列に変換 */
    if (inet_ntop(AF_INET, &mask, mask_str, sizeof(mask_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *mask_str = '\0';
    }

    /* クライアントに割り当てたIPアドレス, サブネットマスクの組とTTLを出力 */
    print_message(__func__,
                  ANSI_ESCAPE_COLOR_BLUE
                  "ip address %s (mask: %s, ttl: %" PRIu16 ") is assigned to client %s"
                  ANSI_ESCAPE_COLOR_RESET "\n",
                  addr_str, mask_str, dhcp_server_config.ttl, inet_ntoa(client->id));
    
    /* 割り当て可能なIPアドレスが存在 */
    /* OFFERメッセージを作成 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_OFFER;
    header.code = DHCP_HEADER_CODE_OFFER_OK;
    header.ttl = htons(dhcp_server_config.ttl);
    header.addr = addr.s_addr;
    header.mask = mask.s_addr;

    /* OFFERメッセージを送信 */
    send_bytes = sendto(server_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&client_addr, sizeof(client_addr));

    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to client %s\n",
                    strerror(errno), inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);
        return;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send full dhcp header to client %s\n",
                    inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);
        return;
    }

    print_message(__func__, "dhcp header has been sent to client %s: ", inet_ntoa(client->id));
    dump_dhcp_header(stderr, &header);

    print_message(__func__,
                  "client " ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET
                  " state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  inet_ntoa(client->id),
                  dhcp_server_state_to_string(client->state),
                  dhcp_server_state_to_string(DHCP_SERVER_STATE_WAIT_REQUEST));

    /* クライアントの情報を更新 */
    /* ttl_counterメンバはタイムアウトまでの秒数を保持 */
    /* addr, maskメンバは割り当てる予定のIPアドレスとサブネットマスクを保持 */
    /* ttlメンバはデフォルトのIPアドレスの使用期限を保持 */
    /* REQUESTメッセージのttlフィールドの値によってttlメンバは変化し得る */
    client->state = DHCP_SERVER_STATE_WAIT_REQUEST;
    client->ttl_counter = TIMEOUT_VALUE;
    client->addr = addr;
    client->mask = mask;
    client->ttl = htons(dhcp_server_config.ttl);

    return;
}

/*
 * REQUESTメッセージ(IPアドレスの割り当て要求)を受信した際の処理
 */
void on_alloc_request_received(
    const struct dhcp_header* received_header,
    struct dhcp_client_list_entry* client,
    int server_sock)
{
    struct dhcp_header header;
    struct sockaddr_in client_addr;
    ssize_t send_bytes;
    
    assert(received_header != NULL);
    assert(client != NULL);
    assert(client->addr.s_addr == received_header->addr);
    assert(client->mask.s_addr == received_header->mask);
    assert(ntohs(received_header->ttl) <= dhcp_server_config.ttl);

    /* クライアントのアドレス構造体を作成 */
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = client->port;
    client_addr.sin_addr = client->id;

    /* ACKメッセージを作成 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_ACK;
    header.code = DHCP_HEADER_CODE_ACK_OK;
    header.ttl = received_header->ttl;
    header.addr = client->addr.s_addr;
    header.mask = client->mask.s_addr;

    /* ACKメッセージを送信 */
    send_bytes = sendto(server_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&client_addr, sizeof(client_addr));

    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to client %s\n",
                    strerror(errno), inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);
        return;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send dhcp header to client %s\n",
                    inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);
        return;
    }
    
    print_message(__func__, "dhcp header has been sent to client %s: ", inet_ntoa(client->id));
    dump_dhcp_header(stderr, &header);

    print_message(__func__,
                  "client " ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET
                  " state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  inet_ntoa(client->id),
                  dhcp_server_state_to_string(client->state),
                  dhcp_server_state_to_string(DHCP_SERVER_STATE_IP_ADDRESS_IN_USE));

    /* クライアントの情報を更新 */
    client->state = DHCP_SERVER_STATE_IP_ADDRESS_IN_USE;
    client->ttl_counter = ntohs(received_header->ttl);
    client->ttl = received_header->ttl;

    return;
}

/*
 * 不正なREQUESTメッセージ(IPアドレスの割り当て要求)を受信した際の処理
 */
void on_invalid_alloc_request_received(
    const struct dhcp_header* received_header,
    struct dhcp_client_list_entry* client,
    int server_sock)
{
    struct dhcp_header header;
    struct sockaddr_in client_addr;
    ssize_t send_bytes;

    assert(received_header != NULL);
    assert(client != NULL);

    /* クライアントのアドレス構造体を作成 */
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = client->port;
    client_addr.sin_addr = client->id;

    /* ACKメッセージを作成 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_ACK;
    header.code = DHCP_HEADER_CODE_ACK_NG;
    
    /* ACKメッセージを送信 */
    send_bytes = sendto(server_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&client_addr, sizeof(client_addr));

    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to client %s\n",
                    strerror(errno), inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);
        return;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send dhcp header to client %s\n",
                    inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);
        return;
    }

    print_message(__func__, "dhcp header has been sent to client %s: ", inet_ntoa(client->id));
    dump_dhcp_header(stderr, &header);

    print_message(__func__,
                  ANSI_ESCAPE_COLOR_RED
                  "invalid ip address allocation request received, "
                  "therefore connection to client %s will be shut down"
                  ANSI_ESCAPE_COLOR_RESET "\n",
                  inet_ntoa(client->id));

    /* クライアントとの接続を終了 */
    on_disconnect_client(received_header, client, server_sock);
    
    return;
}

/*
 * REQUESTメッセージの受信がタイムアウトした際の処理
 */
void on_wait_request_timeout(
    const struct dhcp_header* received_header,
    struct dhcp_client_list_entry* client,
    int server_sock)
{
    struct dhcp_header header;
    struct sockaddr_in client_addr;
    ssize_t send_bytes;

    assert(received_header == NULL);
    assert(client != NULL);

    /* クライアントのアドレス構造体を作成 */
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = client->port;
    client_addr.sin_addr = client->id;

    /* OFFERメッセージを作成 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_OFFER;
    header.code = DHCP_HEADER_CODE_OFFER_OK;
    header.ttl = htons(dhcp_server_config.ttl);
    header.addr = client->addr.s_addr;
    header.mask = client->mask.s_addr;

    /* OFFERメッセージを送信 */
    send_bytes = sendto(server_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&client_addr, sizeof(client_addr));

    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to client %s\n",
                    strerror(errno), inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);
        return;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send dhcp header to client %s\n",
                    inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);
        return;
    }
    
    print_message(__func__, "dhcp header has been sent to client %s: ", inet_ntoa(client->id));
    dump_dhcp_header(stderr, &header);

    print_message(__func__,
                  "client " ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET 
                  " state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  inet_ntoa(client->id),
                  dhcp_server_state_to_string(client->state),
                  dhcp_server_state_to_string(DHCP_SERVER_STATE_WAIT_REQUEST_RETRY));

    /* クライアントの情報を更新 */
    client->state = DHCP_SERVER_STATE_WAIT_REQUEST_RETRY;
    client->ttl_counter = TIMEOUT_VALUE;
    client->ttl = htons(dhcp_server_config.ttl);
    
    return;
}

/*
 * REQUESTメッセージ(使用期間の延長要求)を受信した際の処理
 */
void on_time_ext_request_received(
    const struct dhcp_header* received_header,
    struct dhcp_client_list_entry* client,
    int server_sock)
{
    struct dhcp_header header;
    struct sockaddr_in client_addr;
    ssize_t send_bytes;

    assert(received_header != NULL);
    assert(client != NULL);
    assert(client->addr.s_addr == received_header->addr);
    assert(client->mask.s_addr == received_header->mask);
    assert(ntohs(received_header->ttl) <= dhcp_server_config.ttl);

    /* クライアントのアドレス構造体を作成 */
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = client->port;
    client_addr.sin_addr = client->id;
    
    /* ACKメッセージを作成 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_ACK;
    header.code = DHCP_HEADER_CODE_ACK_OK;
    header.ttl = client->ttl;
    header.addr = client->addr.s_addr;
    header.mask = client->mask.s_addr;

    /* ACKメッセージを送信 */
    send_bytes = sendto(server_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&client_addr, sizeof(client_addr));

    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to client %s\n",
                    strerror(errno), inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);
        return;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send dhcp header to client %s\n",
                    inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);
        return;
    }

    print_message(__func__, "dhcp header has been sent to client %s: ", inet_ntoa(client->id));
    dump_dhcp_header(stderr, &header);

    print_message(__func__,
                  "client " ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET
                  " state changed from "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET " to "
                  ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET "\n",
                  inet_ntoa(client->id),
                  dhcp_server_state_to_string(client->state),
                  dhcp_server_state_to_string(DHCP_SERVER_STATE_IP_ADDRESS_IN_USE));
    
    /* クライアントの情報を更新 */
    client->state = DHCP_SERVER_STATE_IP_ADDRESS_IN_USE;
    client->ttl_counter = ntohs(received_header->ttl);
    client->ttl = received_header->ttl;
    
    return;
}

/*
 * 不正なREQUESTメッセージ(使用期間の延長要求)を受信した際の処理
 */
void on_invalid_time_ext_request_received(
    const struct dhcp_header* received_header,
    struct dhcp_client_list_entry* client,
    int server_sock)
{
    struct dhcp_header header;
    struct sockaddr_in client_addr;
    ssize_t send_bytes;

    assert(received_header != NULL);
    assert(client != NULL);

    /* クライアントのアドレス構造体を作成 */
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = client->port;
    client_addr.sin_addr = client->id;

    /* ACKメッセージを送信 */
    memset(&header, 0, sizeof(struct dhcp_header));
    header.type = DHCP_HEADER_TYPE_ACK;
    header.code = DHCP_HEADER_CODE_ACK_NG;
    
    /* ACKメッセージを送信 */
    send_bytes = sendto(server_sock, &header, sizeof(struct dhcp_header), 0,
                        (struct sockaddr*)&client_addr, sizeof(client_addr));

    /* 送信に失敗した場合 */
    if (send_bytes < 0) {
        print_error(__func__,
                    "sendto() failed: %s, could not send dhcp header to client %s\n",
                    strerror(errno), inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);
        return;
    }

    /* 送信サイズがDHCPヘッダのサイズと異なる場合 */
    if (send_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    "could not send dhcp header to client %s\n",
                    inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(received_header, client, server_sock);
        return;
    }
    
    print_message(__func__, "dhcp header has been sent to client %s: ", inet_ntoa(client->id));
    dump_dhcp_header(stderr, &header);
    
    print_message(__func__,
                  ANSI_ESCAPE_COLOR_RED
                  "invalid time extension request received, "
                  "therefore connection to client %s will be shut down"
                  ANSI_ESCAPE_COLOR_RESET "\n",
                  inet_ntoa(client->id));

    /* クライアントとの接続を終了 */
    on_disconnect_client(received_header, client, server_sock);

    return;
}

/*
 * RELEASEメッセージを受信した際の処理
 */
void on_release_received(
    const struct dhcp_header* received_header,
    struct dhcp_client_list_entry* client,
    int server_sock)
{
    assert(received_header != NULL);
    assert(client != NULL);

    print_message(__func__,
                  ANSI_ESCAPE_COLOR_BLUE
                  "release message received, "
                  "therefore connection to client %s will be shut down"
                  ANSI_ESCAPE_COLOR_RESET "\n",
                  inet_ntoa(client->id));

    /* クライアントとの接続を終了 */
    on_disconnect_client(received_header, client, server_sock);

    return;
}

/*
 * IPアドレスの使用期限を過ぎた際の処理
 */
void on_ip_address_ttl_timeout(
    const struct dhcp_header* received_header,
    struct dhcp_client_list_entry* client,
    int server_sock)
{
    char id_str[INET_ADDRSTRLEN];
    char addr_str[INET_ADDRSTRLEN];

    assert(received_header == NULL);
    assert(client != NULL);

    /* クライアントのID(ネットワークバイトオーダー)を文字列に変換 */
    if (inet_ntop(AF_INET, &client->id, id_str, sizeof(id_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *id_str = '\0';
    }

    /* クライアントに割り当てたIPアドレス(ネットワークバイトオーダー)を文字列に変換 */
    if (inet_ntop(AF_INET, &client->addr, addr_str, sizeof(addr_str)) == NULL) {
        print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
        *addr_str = '\0';
    }

    print_message(__func__,
                  ANSI_ESCAPE_COLOR_RED
                  "ttl of ip address %s assigned to client %s expired, "
                  "therefore connection to client %s will be shut down"
                  ANSI_ESCAPE_COLOR_RESET "\n",
                  addr_str, id_str, id_str);

    /* クライアントとの接続を終了 */
    on_disconnect_client(received_header, client, server_sock);

    return;
}

/*
 * サーバの状態遷移を表す配列
 */
struct dhcp_server_state_transition server_state_transition_table[] = {
    {
        .state = DHCP_SERVER_STATE_INIT,
        .event = DHCP_SERVER_EVENT_DISCOVER,
        .handler = on_discover_received
    },
    {
        .state = DHCP_SERVER_STATE_WAIT_REQUEST,
        .event = DHCP_SERVER_EVENT_ALLOC_REQUEST,
        .handler = on_alloc_request_received
    },
    {
        .state = DHCP_SERVER_STATE_WAIT_REQUEST,
        .event = DHCP_SERVER_EVENT_INVALID_ALLOC_REQUEST,
        .handler = on_invalid_alloc_request_received
    },
    {
        .state = DHCP_SERVER_STATE_WAIT_REQUEST,
        .event = DHCP_SERVER_EVENT_TTL_TIMEOUT,
        .handler = on_wait_request_timeout
    },
    {
        .state = DHCP_SERVER_STATE_WAIT_REQUEST_RETRY,
        .event = DHCP_SERVER_EVENT_ALLOC_REQUEST,
        .handler = on_alloc_request_received
    },
    {
        .state = DHCP_SERVER_STATE_WAIT_REQUEST_RETRY,
        .event = DHCP_SERVER_EVENT_TTL_TIMEOUT,
        .handler = on_disconnect_client
    },
    {
        .state = DHCP_SERVER_STATE_WAIT_REQUEST_RETRY,
        .event = DHCP_SERVER_EVENT_INVALID_ALLOC_REQUEST,
        .handler = on_invalid_alloc_request_received
    },
    {
        .state = DHCP_SERVER_STATE_IP_ADDRESS_IN_USE,
        .event = DHCP_SERVER_EVENT_TIME_EXT_REQUEST,
        .handler = on_time_ext_request_received
    },
    {
        .state = DHCP_SERVER_STATE_IP_ADDRESS_IN_USE,
        .event = DHCP_SERVER_EVENT_INVALID_TIME_EXT_REQUEST,
        .handler = on_invalid_time_ext_request_received
    },
    {
        .state = DHCP_SERVER_STATE_IP_ADDRESS_IN_USE,
        .event = DHCP_SERVER_EVENT_RELEASE,
        .handler = on_release_received
    },
    {
        .state = DHCP_SERVER_STATE_IP_ADDRESS_IN_USE,
        .event = DHCP_SERVER_EVENT_TTL_TIMEOUT,
        .handler = on_ip_address_ttl_timeout
    },
    {
        .state = DHCP_SERVER_STATE_NONE,
        .event = DHCP_SERVER_EVENT_NONE,
        .handler = NULL
    },
};

/*
 * サーバのソケットの準備
 */
bool setup_server_socket(int* server_sock)
{
    /* サーバに割り当てるアドレス */
    struct sockaddr_in server_addr;

    assert(server_sock != NULL);
    
    /* ソケットの作成 */
    if ((*server_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        print_error(__func__,
                    "socket() failed: %s, failed to create server socket\n",
                    strerror(errno));
        return false;
    }

    print_message(__func__,
                  "server socket (fd: %d) successfully created\n",
                  *server_sock);
    
    /* ソケットに割り当てるアドレスの設定 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* ソケットにアドレスを割り当て */
    if (bind(*server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        print_error(__func__, "bind() failed: %s\n", strerror(errno));
        return false;
    }

    print_message(__func__,
                  "bind() succeeded: server ip address: %s (INADDR_ANY), port: %" PRIu16 "\n",
                  inet_ntoa(server_addr.sin_addr), DHCP_SERVER_PORT);

    return true;
}

/*
 * サーバのソケットの後始末
 */
void close_server_socket(int server_sock)
{
    if (close(server_sock) < 0)
        print_error(__func__, "close() failed: %s\n", strerror(errno));

    print_message(__func__, "server socket (fd: %d) closed\n", server_sock);
}

/*
 * サーバで発生したイベントの処理
 */
void handle_event(
    const struct dhcp_header* header,
    struct dhcp_client_list_entry* client,
    enum dhcp_server_event event,
    int server_sock)
{
    dhcp_server_event_handler handler;

    assert(client != NULL);
    assert(event != DHCP_SERVER_EVENT_NONE);

    /* 現在の状態とイベントに対応する遷移関数を取得 */
    handler = lookup_server_state_transition_table(server_state_transition_table,
                                                   client->state, event);

    /* 状態とイベントに対応する遷移関数がない場合 */
    /* 該当するクライアントとの処理を終了 */
    if (handler == NULL) {
        print_error(__func__,
                    ANSI_ESCAPE_COLOR_RED
                    "lookup_server_state_transition_table() failed: "
                    "corresponding event handler not found, therefore "
                    "connection to client %s will be shut down"
                    ANSI_ESCAPE_COLOR_RESET "\n",
                    inet_ntoa(client->id));
        /* クライアントとの接続を終了 */
        on_disconnect_client(header, client, server_sock);
    } else {
        /* 対応する遷移関数を実行 */
        (*handler)(header, client, server_sock);
    }
}

/*
 * インターバルタイマーのタイムアウトの処理
 */
void handle_alrm(int server_sock)
{
    struct dhcp_client_list_entry* iter;
    struct dhcp_client_list_entry* iter_next;

    char id_str[INET_ADDRSTRLEN];
    char addr_str[INET_ADDRSTRLEN];

    list_for_each_entry_safe(iter, iter_next,
                             &dhcp_client_list_head.list_entry,
                             struct dhcp_client_list_entry, list_entry) {
        /* IPアドレスの使用期限を更新 */
        if (iter->ttl_counter > 0)
            iter->ttl_counter--;

        /* 更新されたIPアドレスの使用期限を表示 */
        if (iter->state == DHCP_SERVER_STATE_IP_ADDRESS_IN_USE) {
            if (inet_ntop(AF_INET, &iter->id, id_str, sizeof(id_str)) == NULL) {
                print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
                *id_str = '\0';
            }

            if (inet_ntop(AF_INET, &iter->addr, addr_str, sizeof(addr_str)) == NULL) {
                print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
                *addr_str = '\0';
            }

            print_message(__func__,
                          "ttl of ip address "
                          ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET
                          " (client: " ANSI_ESCAPE_COLOR_GREEN "%s" ANSI_ESCAPE_COLOR_RESET
                          "): " ANSI_ESCAPE_COLOR_GREEN "%" PRIu16 ANSI_ESCAPE_COLOR_RESET "\n",
                          addr_str, id_str, iter->ttl_counter);
        }

        /* IPアドレスの使用期限が切れた場合 */
        /* タイムアウトのイベントを処理 */
        if (iter->ttl_counter == 0)
            handle_event(NULL, iter, DHCP_SERVER_EVENT_TTL_TIMEOUT, server_sock);
    }
}

/*
 * サーバが受信したDHCPヘッダの処理
 * DHCPヘッダの内容からイベントを判定
 */
void handle_dhcp_header(
    const struct sockaddr_in* client_addr,
    ssize_t recv_bytes, const struct dhcp_header* header,
    int server_sock)
{
    struct dhcp_client_list_entry* client;

    char client_addr_str[INET_ADDRSTRLEN];
    char client_mask_str[INET_ADDRSTRLEN];
    char header_addr_str[INET_ADDRSTRLEN];
    char header_mask_str[INET_ADDRSTRLEN];

    print_message(__func__, "message received from %s (port: %d)\n",
                  inet_ntoa(client_addr->sin_addr),
                  ntohs(client_addr->sin_port));
    
    /* クライアントの情報を取得 */
    /* 新規のクライアントである場合はNULL */
    client = lookup_dhcp_client(&dhcp_client_list_head,
                                client_addr->sin_addr);

    /* 新規のクライアントである場合 */
    if (client == NULL) {
        /* クライアントのリストに追加 */
        if (!append_dhcp_client(&dhcp_client_list_head,
                                client_addr->sin_addr, client_addr->sin_port,
                                DHCP_SERVER_STATE_INIT, TIMEOUT_VALUE)) {
            print_error(__func__, "append_dhcp_client() failed\n");
            return;
        }

        /* クライアントの情報を取得 */
        client = lookup_dhcp_client(&dhcp_client_list_head,
                                    client_addr->sin_addr);
        assert(client != NULL);
    }

    /* 受信したデータサイズとDHCPヘッダのサイズが異なる場合 */
    if (recv_bytes != sizeof(struct dhcp_header)) {
        print_error(__func__,
                    ANSI_ESCAPE_COLOR_RED
                    "invalid dhcp header: "
                    "expected length was %zd bytes, but %zd were received"
                    ANSI_ESCAPE_COLOR_RESET "\n",
                    sizeof(struct dhcp_header), recv_bytes);
        /* プロトコルに整合しないDHCPヘッダの処理 */
        handle_event(header, client, DHCP_SERVER_EVENT_INVALID_HEADER, server_sock);
        return;
    }

    /* 受信したDHCPヘッダを出力 */
    print_message(__func__, "dhcp header received from client %s: ",
                  inet_ntoa(client_addr->sin_addr));
    dump_dhcp_header(stderr, header);

    /* メッセージの種別に応じてイベントを判別 */
    switch (header->type) {
        case DHCP_HEADER_TYPE_DISCOVER:
            /* DISCOVERメッセージである場合 */
            /* Type以外のフィールドが0でなければエラー */
            if (header->code != 0 ||
                ntohs(header->ttl) != 0 ||
                ntohl(header->addr) != 0 ||
                ntohl(header->mask) != 0) {
                print_error(__func__,
                            ANSI_ESCAPE_COLOR_RED
                            "invalid dhcp header: "
                            "dhcp header fields except 'type' should be set to zero"
                            ANSI_ESCAPE_COLOR_RESET "\n");
                /* プロトコルに整合しないDHCPヘッダの処理 */
                handle_event(header, client, DHCP_SERVER_EVENT_INVALID_HEADER, server_sock);
                return;
            }
            
            /* DISCOVERメッセージの処理 */
            handle_event(header, client, DHCP_SERVER_EVENT_DISCOVER, server_sock);
            return;

        case DHCP_HEADER_TYPE_REQUEST:
            /* REQUESTメッセージである場合 */
            /* Codeフィールドのチェック */
            if (header->code != DHCP_HEADER_CODE_REQUEST_ALLOC &&
                header->code != DHCP_HEADER_CODE_REQUEST_TIME_EXT) {
                print_error(__func__,
                            ANSI_ESCAPE_COLOR_RED
                            "invalid 'type' field value %" PRIu8 ", "
                            "%" PRIu8 " (%s) or %" PRIu8 " (%s) expected"
                            ANSI_ESCAPE_COLOR_RESET "\n",
                            header->code,
                            DHCP_HEADER_CODE_REQUEST_ALLOC,
                            dhcp_header_code_to_string(DHCP_HEADER_TYPE_REQUEST,
                                                       DHCP_HEADER_CODE_REQUEST_ALLOC),
                            DHCP_HEADER_CODE_REQUEST_TIME_EXT,
                            dhcp_header_code_to_string(DHCP_HEADER_TYPE_REQUEST,
                                                       DHCP_HEADER_CODE_REQUEST_TIME_EXT));
                /* プロトコルに整合しないDHCPヘッダの処理 */
                handle_event(header, client, DHCP_SERVER_EVENT_INVALID_HEADER, server_sock);
                return;
            }
            
            /* クライアントに割り当てられたIPアドレス, サブネットマスク,
             * DHCPヘッダに含まれるIPアドレス, サブネットマスクを文字列に変換 */
            if (inet_ntop(AF_INET, &client->addr, client_addr_str, sizeof(client_addr_str)) == NULL) {
                print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
                *client_addr_str = '\0';
            }

            if (inet_ntop(AF_INET, &client->mask, client_mask_str, sizeof(client_mask_str)) == NULL) {
                print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
                *client_mask_str = '\0';
            }

            if (inet_ntop(AF_INET, &header->addr, header_addr_str, sizeof(header_addr_str)) == NULL) {
                print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
                *header_addr_str = '\0';
            }

            if (inet_ntop(AF_INET, &header->mask, header_mask_str, sizeof(header_mask_str)) == NULL) {
                print_error(__func__, "inet_ntop() failed: %s\n", strerror(errno));
                *header_mask_str = '\0';
            }

            /* 要求されたIPアドレスが割り当てたものと異なる場合はエラー */
            if (client->addr.s_addr != header->addr ||
                client->mask.s_addr != header->mask) {
                print_error(__func__,
                            ANSI_ESCAPE_COLOR_RED
                            "invalid 'addr' field value %s (mask: %s), %s (mask: %s) expected"
                            ANSI_ESCAPE_COLOR_RESET "\n",
                            header_addr_str, header_mask_str,
                            client_addr_str, client_mask_str);
                
                /* 不正なREQUESTメッセージの処理 */
                if (header->code == DHCP_HEADER_CODE_REQUEST_ALLOC)
                    handle_event(header, client, DHCP_SERVER_EVENT_INVALID_ALLOC_REQUEST, server_sock);
                else
                    handle_event(header, client, DHCP_SERVER_EVENT_INVALID_TIME_EXT_REQUEST, server_sock);

                return;
            }

            /* TTLフィールドが大き過ぎる場合はエラー */
            if (ntohs(header->ttl) > ntohs(client->ttl)) {
                print_error(__func__,
                            ANSI_ESCAPE_COLOR_RED
                            "invalid 'ttl' field value %" PRIu16 ", "
                            "less than or equal to %" PRIu16 " expected"
                            ANSI_ESCAPE_COLOR_RESET "\n",
                            ntohs(header->ttl), ntohs(client->ttl));
                
                /* 不正なREQUESTメッセージの処理 */
                if (header->code == DHCP_HEADER_CODE_REQUEST_ALLOC)
                    handle_event(header, client, DHCP_SERVER_EVENT_INVALID_ALLOC_REQUEST, server_sock);
                else
                    handle_event(header, client, DHCP_SERVER_EVENT_INVALID_TIME_EXT_REQUEST, server_sock);
                return;
            }
            
            /* REQUESTメッセージの処理 */
            if (header->code == DHCP_HEADER_CODE_REQUEST_ALLOC)
                handle_event(header, client, DHCP_SERVER_EVENT_ALLOC_REQUEST, server_sock);
            else
                handle_event(header, client, DHCP_SERVER_EVENT_TIME_EXT_REQUEST, server_sock);
            return;

        case DHCP_HEADER_TYPE_RELEASE:
            /* RELEASEメッセージである場合 */
            /* Type, Addr以外のフィールドが0でなければエラー */
            if (header->code != 0 ||
                ntohs(header->ttl) != 0 ||
                ntohl(header->mask) != 0) {
                print_error(__func__,
                            ANSI_ESCAPE_COLOR_RED
                            "invalid dhcp header: "
                            "dhcp header fields except 'ttl' and 'mask' should be set to zero"
                            ANSI_ESCAPE_COLOR_RESET "\n");
                /* プロトコルに整合しないDHCPヘッダの処理 */
                handle_event(header, client, DHCP_SERVER_EVENT_INVALID_HEADER, server_sock);
                return;
            }
            
            /* RELEASEメッセージの処理 */
            handle_event(header, client, DHCP_SERVER_EVENT_RELEASE, server_sock);
            return;
    }
    
    /* それ以外のタイプである場合はエラー */
    print_error(__func__,
                ANSI_ESCAPE_COLOR_RED
                "invalid dhcp header: unknown message type: %s"
                ANSI_ESCAPE_COLOR_RESET "\n",
                dhcp_header_type_to_string(header->type));
    handle_event(header, client, DHCP_SERVER_EVENT_INVALID_HEADER, server_sock);

    return;
}

/*
 * サーバの実行
 */
bool run_dhcp_server()
{
    int server_sock;

    ssize_t recv_bytes;
    int recv_errno;
    struct dhcp_header recv_header;
    struct sockaddr_in client_addr;
    socklen_t addr_len;

    /* シグナルSIGALRMをブロック */
    if (!block_sigalrm()) {
        print_error(__func__, "block_sigalrm() failed\n");
        return false;
    }

    /* シグナルハンドラを設定 */
    if (!setup_signal_handlers()) {
        print_error(__func__, "setup_sigalrm_handler() failed\n");
        return false;
    }

    /* サーバのソケットの準備 */
    if (!setup_server_socket(&server_sock)) {
        print_error(__func__, "setup_server_socket() failed\n");
        return false;
    }

    /* インターバルタイマーの準備 */
    if (!setup_interval_timer()) {
        print_error(__func__, "setup_interval_timer() failed\n");
        close_server_socket(server_sock);
        return false;
    }
    
    /* サーバのメインループの実行 */
    is_sigalrm_handled = 0;
    app_exit = 0;

    while (!app_exit) {
        /* シグナルSIGALRMのブロックを解除 */
        if (!unblock_sigalrm()) {
            print_error(__func__, "unblock_sigalrm() failed\n");
            close_server_socket(server_sock);
            return false;
        }
        
        /* DHCPヘッダを受信 */
        addr_len = sizeof(client_addr);
        recv_bytes = recvfrom(server_sock, &recv_header, sizeof(recv_header), 0,
                              (struct sockaddr*)&client_addr, &addr_len);
        recv_errno = errno;
        
        /* シグナルSIGALRMを再びブロック */
        if (!block_sigalrm()) {
            print_error(__func__, "block_sigalrm() failed\n");
            close_server_socket(server_sock);
            return false;
        }

        if (recv_bytes < 0) {
            if (recv_errno == EINTR && is_sigalrm_handled == 1) {
                /* シグナルSIGALRMの処理 */
                handle_alrm(server_sock);
                is_sigalrm_handled = 0;
            } else {
                /* その他のエラーの場合 */
                print_error(__func__, "recvfrom() failed: %s\n", strerror(recv_errno));
                close_server_socket(server_sock);
                return false;
            }
        } else {
            /* 受信したDHCPヘッダの処理 */
            handle_dhcp_header(&client_addr, recv_bytes, &recv_header, server_sock);
        }
    }

    return true;
}

int main(int argc, char** argv)
{
    bool exit_status;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <config file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    /* サーバの設定の初期化 */
    initialize_dhcp_server_config(&dhcp_server_config);
    
    if (!load_config(argv[1], &dhcp_server_config)) {
        print_error(__func__, "load_config() failed\n");
        free_dhcp_server_config(&dhcp_server_config);
        return EXIT_FAILURE;
    }

    print_message(__func__, "load_config() succeeded\n");
    
    /* 割り当て可能なIPアドレスのリストを表示 */
    available_ip_addr_list_head = &dhcp_server_config.available_ip_addr_list_head;
    dump_ip_addr_list(stderr, available_ip_addr_list_head);

    /* 接続されたクライアントのリストを初期化 */
    initialize_client_list(&dhcp_client_list_head);
    print_message(__func__, "initialize_client_list() called\n");

    /* サーバの実行 */
    exit_status = run_dhcp_server();

    /* 接続されたクライアントのリストを破棄 */
    free_client_list(&dhcp_client_list_head);
    print_message(__func__, "free_client_list() called\n");
    
    /* サーバの設定の破棄 */
    free_dhcp_server_config(&dhcp_server_config);
    print_message(__func__, "free_dhcp_server_config() called\n");

    return exit_status ? EXIT_SUCCESS : EXIT_FAILURE;
}

