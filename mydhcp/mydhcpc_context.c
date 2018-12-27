
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcpc_context.c */

#include <assert.h>
#include <stdlib.h>

#include "mydhcpc_context.h"
#include "util.h"

/*
 * DHCPクライアントの情報を初期化
 */
bool initialize_dhcp_client_context(
    struct dhcp_client_context* context, const char* server_addr_str)
{
    struct in_addr server_addr;

    assert(context != NULL);
    assert(server_addr_str != NULL);
    
    if (!inet_aton(server_addr_str, &server_addr)) {
        print_error(__func__, "inet_aton() failed\n");
        return false;
    }

    context->state = DHCP_CLIENT_STATE_INIT;
    context->ttl_counter = UINT16_C(0);
    context->client_sock = -1;
    context->server_addr = server_addr;
    context->addr.s_addr = htonl(0);
    context->mask.s_addr = htonl(0);
    context->ttl = htons(0);

    return true;
}

/*
 * DHCPクライアントの情報を破棄
 */
void free_dhcp_client_context(struct dhcp_client_context* context)
{
    assert(context != NULL);

    context->state = DHCP_CLIENT_STATE_NONE;
    context->ttl_counter = UINT16_C(0);
    context->client_sock = -1;
    context->server_addr.s_addr = htonl(0);
    context->addr.s_addr = htonl(0);
    context->mask.s_addr = htonl(0);
    context->ttl = htons(0);
}

