#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <sys/socket.h>
#include <jansson.h>

#define NETLINK_USER 31
#define MAX_PAYLOAD 1024

struct nl_msg {
    struct nlmsghdr hdr;
    struct genlmsghdr genl;
    char payload[MAX_PAYLOAD];
};

int create_nl_socket(void) {
    int sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_nl src_addr;
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();
    src_addr.nl_groups = 0;

    if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

json_t* process_request(const char* json_str) {
    json_error_t error;
    json_t* root = json_loads(json_str, 0, &error);
    if (!root) {
        fprintf(stderr, "JSON error: %s\n", error.text);
        return NULL;
    }

    json_t* action = json_object_get(root, "action");
    json_t* arg1 = json_object_get(root, "argument_1");
    json_t* arg2 = json_object_get(root, "argument_2");

    if (!json_is_string(action) || !json_is_integer(arg1) || !json_is_integer(arg2)) {
        json_decref(root);
        return NULL;
    }

    const char* action_str = json_string_value(action);
    int a = json_integer_value(arg1);
    int b = json_integer_value(arg2);
    int result = 0;

    if (strcmp(action_str, "add") == 0) {
        result = a + b;
    } else if (strcmp(action_str, "sub") == 0) {
        result = a - b;
    } else if (strcmp(action_str, "mul") == 0) {
        result = a * b;
    } else {
        json_decref(root);
        return NULL;
    }

    json_decref(root);

    json_t* response = json_object();
    json_object_set_new(response, "результат", json_integer(result));
    return response;
}

int main() {
    int sock_fd = create_nl_socket();
    if (sock_fd < 0) {
        return EXIT_FAILURE;
    }

    printf("Сервер запустился, ожидание сообщения...\n");

    while (1) {
        struct nl_msg msg;
        struct sockaddr_nl dest_addr;
        socklen_t addr_len = sizeof(dest_addr);

        ssize_t len = recvfrom(sock_fd, &msg, sizeof(msg), 0,
                             (struct sockaddr*)&dest_addr, &addr_len);
        if (len < 0) {
            perror("recvfrom");
            continue;
        }

        printf("Получено сообщение: %s\n", msg.payload);

        json_t* response = process_request(msg.payload);
        if (response) {
            char* response_str = json_dumps(response, 0);
            json_decref(response);

            struct nl_msg reply;
            memset(&reply, 0, sizeof(reply));
            reply.hdr.nlmsg_len = NLMSG_LENGTH(strlen(response_str) + 1);
            reply.hdr.nlmsg_pid = getpid();
            reply.hdr.nlmsg_flags = 0;
            strcpy(reply.payload, response_str);

            sendto(sock_fd, &reply, reply.hdr.nlmsg_len, 0,
                  (struct sockaddr*)&dest_addr, addr_len);

            free(response_str);
        }
    }

    close(sock_fd);
    return EXIT_SUCCESS;
}
