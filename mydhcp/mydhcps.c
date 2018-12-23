
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
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mydhcp.h"
#include "mydhcps.h"
#include "mydhcps_alarm.h"
#include "mydhcps_config.h"
#include "mydhcps_client_list.h"
#include "mydhcps_state_machine.h"
#include "util.h"

/*
 * 疑似DHCPサーバの設定
 */
struct dhcp_server_config dhcp_server_config;

/*
 * 疑似DHCPサーバが割り当て可能なIPアドレスのリスト
 */
struct ip_addr_list_entry* available_ip_addr_list_head;

/*
 * 接続されたクライアントのリスト
 */
struct dhcp_client_list_entry dhcp_client_list_head;

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
        print_error(__func__, "socket() failed: %s\n", strerror(errno));
        return false;
    }
    
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

    return true;
}

/*
 * サーバのソケットの後始末
 */
void close_server_socket(int server_sock)
{
    if (close(server_sock) < 0)
        print_error(__func__, "close() failed: %s\n", strerror(errno));
}

/*
 * サーバで発生したイベントの処理
 */
void handle_event(
    const struct dhcp_header* header,
    struct dhcp_client_list_entry* client,
    enum dhcp_server_event event)
{
    dhcp_server_event_handler handler;

    assert(client != NULL);
    assert(event != DHCP_SERVER_EVENT_NONE);

    /* 現在の状態とイベントに対応する遷移関数を取得 */
    handler = lookup_server_state_transition_table(client->state, event);

    /* 状態とイベントに対応する遷移関数がない場合 */
    /* 該当するクライアントとの処理を終了 */
    if (handler == NULL) {
        print_error(__func__,
                    "lookup_server_state_transition_table() failed: "
                    "corresponding event handler not found, therefore "
                    "connection to client %s will be shut down\n",
                    inet_ntoa(client->addr));
        /* TODO */
    } else {
        /* 対応する遷移関数を実行 */
        (*handler)(header, client);
    }
}

/*
 * インターバルタイマーのタイムアウトの処理
 */
void handle_alrm()
{
    struct dhcp_client_list_entry* iter;
    struct dhcp_client_list_entry* iter_next;

    list_for_each_entry_safe(iter, iter_next,
                             &dhcp_client_list_head.list_entry,
                             struct dhcp_client_list_entry, list_entry) {
        /* IPアドレスの使用期限を更新 */
        if (iter->ttl_counter > 0)
            iter->ttl_counter--;

        /* IPアドレスの使用期限が切れた場合 */
        /* タイムアウトのイベントを処理 */
        if (iter->ttl_counter == 0)
            handle_event(NULL, iter, DHCP_SERVER_EVENT_TTL_TIMEOUT);
    }
}

/*
 * サーバが受信したDHCPヘッダの処理
 */
void handle_dhcp_header(
    const struct sockaddr_in* client_addr,
    ssize_t recv_bytes, const struct dhcp_header* header)
{
    struct dhcp_client_list_entry* client;

    print_error(__func__, "message received from %s (port: %d)\n",
                inet_ntoa(client_addr->sin_addr),
                ntohs(client_addr->sin_port));
    
    /* クライアントの情報を取得 */
    /* 新規のクライアントである場合はNULL */
    client = lookup_dhcp_client(&dhcp_client_list_head,
                                client_addr->sin_addr);

    /* 新規のクライアントである場合 */
    if (client == NULL) {
        /* クライアントのリストに追加 */
        if (!append_dhcp_client(&dhcp_client_list_head, client_addr->sin_addr,
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
                    "invalid dhcp header: "
                    "expected length was %zd bytes, but %zd were received\n",
                    sizeof(struct dhcp_header), recv_bytes);
        /* プロトコルに整合しないDHCPヘッダの処理 */
        handle_event(header, client, DHCP_SERVER_EVENT_INVALID_HEADER);
        return;
    }

    /* メッセージを出力 */
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
                            "invalid dhcp header: "
                            "dhcp header fields except 'type' should be set to zero\n");
                /* プロトコルに整合しないDHCPヘッダの処理 */
                handle_event(header, client, DHCP_SERVER_EVENT_INVALID_HEADER);
                return;
            }
            
            /* DISCOVERメッセージの処理 */
            handle_event(header, client, DHCP_SERVER_EVENT_DISCOVER);
            return;

        case DHCP_HEADER_TYPE_REQUEST:
            /* REQUESTメッセージである場合 */
            /* Codeフィールドのチェック */
            if (header->code != DHCP_HEADER_CODE_REQUEST_ALLOC ||
                header->code != DHCP_HEADER_CODE_REQUEST_TIME_EXT) {
                print_error(__func__,
                            "invalid 'type' field value %" PRIu8 ", "
                            "%" PRIu8 " (%s) or %" PRIu8 " (%s) expected",
                            header->code,
                            DHCP_HEADER_CODE_REQUEST_ALLOC,
                            dhcp_header_code_to_string(DHCP_HEADER_TYPE_REQUEST,
                                                       DHCP_HEADER_CODE_REQUEST_ALLOC),
                            DHCP_HEADER_CODE_REQUEST_TIME_EXT,
                            dhcp_header_code_to_string(DHCP_HEADER_TYPE_REQUEST,
                                                       DHCP_HEADER_CODE_REQUEST_TIME_EXT));
                /* プロトコルに整合しないDHCPヘッダの処理 */
                handle_event(header, client, DHCP_SERVER_EVENT_INVALID_HEADER);
                return;
            }

            /* 要求されたIPアドレスが割り当てたものと異なる場合はエラー */
            if (client->addr.s_addr != header->addr ||
                client->mask.s_addr != header->mask) {
                print_error(__func__,
                            "invalid 'addr' field value %s (mask: %s), "
                            "%s (mask: %s) expected",
                            inet_ntoa(client->addr), inet_ntoa(client->mask),
                            inet_ntoa(*(struct in_addr*)&header->addr),
                            inet_ntoa(*(struct in_addr*)&header->mask));
                
                /* 不正なREQUESTメッセージの処理 */
                if (header->code == DHCP_HEADER_CODE_REQUEST_ALLOC)
                    handle_event(header, client, DHCP_SERVER_EVENT_INVALID_ALLOC_REQUEST);     
                else
                    handle_event(header, client, DHCP_SERVER_EVENT_INVALID_TIME_EXT_REQUEST);
                return;
            }

            /* TTLフィールドが大き過ぎる場合はエラー */
            if (ntohs(header->ttl) > ntohs(client->ttl)) {
                print_error(__func__,
                            "invalid 'ttl' field value %" PRIu16 ", "
                            "less than or equal to %" PRIu16 " expected",
                            ntohs(header->ttl), ntohs(client->ttl));
                
                /* 不正なREQUESTメッセージの処理 */
                if (header->code == DHCP_HEADER_CODE_REQUEST_ALLOC)
                    handle_event(header, client, DHCP_SERVER_EVENT_INVALID_ALLOC_REQUEST);
                else
                    handle_event(header, client, DHCP_SERVER_EVENT_INVALID_TIME_EXT_REQUEST);
                return;
            }
            
            /* REQUESTメッセージの処理 */
            if (header->code == DHCP_HEADER_CODE_REQUEST_ALLOC)
                handle_event(header, client, DHCP_SERVER_EVENT_ALLOC_REQUEST);
            else
                handle_event(header, client, DHCP_SERVER_EVENT_TIME_EXT_REQUEST);
            return;

        case DHCP_HEADER_TYPE_RELEASE:
            /* RELEASEメッセージである場合 */
            /* Type, Addr以外のフィールドが0でなければエラー */
            if (header->code != 0 ||
                ntohs(header->ttl) != 0 ||
                ntohl(header->mask) != 0) {
                print_error(__func__,
                            "invalid dhcp header: "
                            "dhcp header fields except 'ttl' and 'mask' should be set to zero\n");
                /* プロトコルに整合しないDHCPヘッダの処理 */
                handle_event(header, client, DHCP_SERVER_EVENT_INVALID_HEADER);
                return;
            }
            
            /* RELEASEメッセージの処理 */
            handle_event(header, client, DHCP_SERVER_EVENT_RELEASE);
            return;
    }
    
    /* それ以外のタイプである場合はエラー */
    handle_event(header, client, DHCP_SERVER_EVENT_INVALID_HEADER);
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

    /* シグナルSIGALRMのハンドラを設定 */
    if (!setup_sigalrm_handler()) {
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

    while (true) {
        /* シグナルSIGALRMのブロックを解除 */
        if (!unblock_sigalrm()) {
            print_error(__func__, "unblock_sigalrm() failed\n");
            close_server_socket(server_sock);
            return false;
        }
        
        /* ソケットからDHCPヘッダを受信 */
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
                handle_alrm();
                is_sigalrm_handled = 0;
            } else {
                /* その他のエラーの場合 */
                print_error(__func__, "recvfrom() failed: %s\n", strerror(recv_errno));
                close_server_socket(server_sock);
                return false;
            }
        } else {
            /* 受信したDHCPヘッダの処理 */
            handle_dhcp_header(&client_addr, recv_bytes, &recv_header);
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
    
    /* 割り当て可能なIPアドレスのリストを表示 */
    available_ip_addr_list_head = &dhcp_server_config.available_ip_addr_list_head;
    dump_ip_addr_list(stderr, available_ip_addr_list_head);

    /* 接続されたクライアントのリストを初期化 */
    initialize_client_list(&dhcp_client_list_head);

    /* サーバの実行 */
    exit_status = run_dhcp_server();

    /* 接続されたクライアントのリストを破棄 */
    free_client_list(&dhcp_client_list_head);
    
    /* サーバの設定の破棄 */
    free_dhcp_server_config(&dhcp_server_config);

    return exit_status ? EXIT_SUCCESS : EXIT_FAILURE;
}

