#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#define MAX_RESPONSE_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Sử dụng: %s <URL>\n", argv[0]);
        return 1;
    }

    char *url = argv[1];
    char hostname[256];
    char path[256];

   if (sscanf(url, "http://%255[^/]%255[^\n]", hostname, path) != 2) {
    fprintf(stderr, "URL không hợp lệ.\n");
    return 1;
}


    struct hostent *host = gethostbyname(hostname);
    if (!host) {
        fprintf(stderr, "Không thể tìm thấy thông tin host.\n");
        return 1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(80);
    memcpy(&server_address.sin_addr, host->h_addr_list[0], host->h_length);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        fprintf(stderr, "Không thể tạo socket.\n");
        return 1;
    }

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        fprintf(stderr, "Không thể kết nối tới máy chủ.\n");
        return 1;
    }

    char request[512];
    snprintf(request, sizeof(request), "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, hostname);

    if (send(client_socket, request, strlen(request), 0) < 0) {
        fprintf(stderr, "Không thể gửi yêu cầu.\n");
        return 1;
    }

    char response[MAX_RESPONSE_SIZE];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_socket, response, sizeof(response) - 1, 0)) > 0) {
        response[bytes_received] = '\0';
        printf("%s", response);
    }
    close(client_socket);

    return 0;
}
