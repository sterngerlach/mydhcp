
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* mydhcps.c */

#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "mydhcp.h"
#include "util.h"

struct dhcp_server_config dhcp_server_config;
struct ip_addr_list_entry* available_ip_addr_list_head;

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <config file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    initialize_dhcp_server_config(&dhcp_server_config);
    
    if (!load_config(argv[1], &dhcp_server_config)) {
        print_error(__func__, "load_config() failed\n");
        free_dhcp_server_config(&dhcp_server_config);
        return EXIT_FAILURE;
    }

    available_ip_addr_list_head = &dhcp_server_config.available_ip_addr_list_head;
    dump_ip_addr_list(stderr, available_ip_addr_list_head);

    free_dhcp_server_config(&dhcp_server_config);

    return EXIT_SUCCESS;
}

