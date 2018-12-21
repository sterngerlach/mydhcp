
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps.c */

#include <assert.h>
#include <errno.h>
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
 * インターバルタイマーのタイムアウトの処理
 */
void handle_alrm()
{
    struct dhcp_client_list_entry* iter;
    struct dhcp_client_list_entry* iter_next;
    dhcp_server_event_handler handler;

    list_for_each_entry_safe(iter, iter_next,
                             &dhcp_client_list_head.list_entry,
                             struct dhcp_client_list_entry, list_entry) {
        /* IPアドレスの使用期限を更新 */
        iter->ttl_counter--;

        /* IPアドレスの使用期限が切れた場合 */
        if (iter->ttl_counter == 0) {
        }
    }
}

/*
 * サーバが受信したDHCPヘッダの処理
 */
void handle_dhcp_header(ssize_t recv_bytes, struct dhcp_header* header)
{
    /* TODO */
}

/*
 * サーバで発生したイベントの処理
 */
void handle_event(struct dhcp_client_list_entry* client, enum dhcp_server_event event)
{
    /* TODO */
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
        } else if (recv_bytes != sizeof(recv_header)) {
            /* 受信したDHCPヘッダのサイズが小さい場合 */
            print_error(__func__, "incomplete dhcp header received\n");
            close_server_socket(server_sock);
            return false;
        } else {
            /* 受信したDHCPヘッダの処理 */
            handle_dhcp_header(recv_bytes, &recv_header);
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

