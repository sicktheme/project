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

    if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr))) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <action> <arg1> <arg2>\n", argv[0]);
        fprintf(stderr, "Действия: add, sub, mul\n");
        return EXIT_FAILURE;
    }

    const char* action = argv[1];
    int arg1 = atoi(argv[2]);
    int arg2 = atoi(argv[3]);

    json_t* request = json_object();
    json_object_set_new(request, "action", json_string(action));
    json_object_set_new(request, "argument_1", json_integer(arg1));
    json_object_set_new(request, "argument_2", json_integer(arg2));

    char* request_str = json_dumps(request, 0);
    json_decref(request);

    int sock_fd = create_nl_socket();
    if (sock_fd < 0) {
        free(request_str);
        return EXIT_FAILURE;
    }

    struct sockaddr_nl dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; 
    dest_addr.nl_groups = 0;

    struct nl_msg msg;
    memset(&msg, 0, sizeof(msg));
    msg.hdr.nlmsg_len = NLMSG_LENGTH(strlen(request_str) + 1);
    msg.hdr.nlmsg_pid = getpid();
    msg.hdr.nlmsg_flags = 0;
    strcpy(msg.payload, request_str);

    printf("Отправка запроса: %s\n", request_str);
    free(request_str);

    if (sendto(sock_fd, &msg, msg.hdr.nlmsg_len, 0,
              (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Отправлено в");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    struct nl_msg reply;
    socklen_t addr_len = sizeof(dest_addr);
    ssize_t len = recvfrom(sock_fd, &reply, sizeof(reply), 0,
                          (struct sockaddr*)&dest_addr, &addr_len);
    if (len < 0) {
        perror("recvfrom");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    printf("Ответ: %s\n", reply.payload);

    json_error_t error;
    json_t* root = json_loads(reply.payload, 0, &error);
    if (root) {
        json_t* result = json_object_get(root, "Результат");
        if (json_is_integer(result)) {
            printf("Результат: %d\n", json_integer_value(result));
        }
        json_decref(root);
    }

    close(sock_fd);
    return EXIT_SUCCESS;
}
