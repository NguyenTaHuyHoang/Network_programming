#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#define MAX_RESPONSE_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Sử dụng: %s <URL> <output_file>\n", argv[0]);
        return 1;
    }

    char *url = argv[1];
    char *output_filename = argv[2];
    char hostname[256];
    char path[256];

    if (sscanf(url, "http://%255[^/]", hostname) == 1) {
    // Trường hợp có path
    if (strchr(url, '/')) {
        int offset = strlen(hostname) + 7;  // 7 là độ dài của "http://"
        strncpy(path, url + offset, 255);
        path[255] = '\0';  // Đảm bảo kết thúc chuỗi path
    } else {
        // Trường hợp không có path
        path[0] = '\0';
    }
    } else {
    fprintf(stderr, "URL không hợp lệ.\n");
    return 1;
    }

    if (strlen(hostname) >= 256 || strlen(path) >= 256) {
        fprintf(stderr, "Hostname hoặc path quá dài.\n");
        return 1;
    }

    struct hostent *host = gethostbyname(hostname);
    if (!host) {
        fprintf(stderr, "Không thể tìm thấy thông tin host.\n");
        return 1;
    }

    //Lay dia chi host
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(80);
    memcpy(&server_address.sin_addr, host->h_addr_list[0], host->h_length);
    // Khoi tao mot socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        fprintf(stderr, "Không thể tạo socket.\n");
        return 1;
    }
    // ket noi toi may chu
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

    FILE *output_file = fopen(output_filename, "w");
    if (!output_file) {
        fprintf(stderr, "Không thể mở tệp để ghi nội dung.\n");
        return 1;
    }

    char response[MAX_RESPONSE_SIZE];
    ssize_t bytes_received;
    int body_started = 0;  // Dùng để xác định khi phần body bắt đầu

    while ((bytes_received = recv(client_socket, response, sizeof(response) - 1, 0)) > 0) {
        response[bytes_received] = '\0';

        if (!body_started) {
            // Tìm chuỗi "\r\n\r\n" để xác định phần body bắt đầu
            char *body_start = strstr(response, "\r\n\r\n");
            if (body_start) {
                body_start += 4;  // Bỏ qua "\r\n\r\n" và chuyển đến phần body
                body_started = 1;

                // Ghi phần body vào tệp
                fprintf(output_file, "%s", body_start);
            }
        } else {
            // Ghi phần body vào tệp
            fprintf(output_file, "%s", response);
        }
    }

    fclose(output_file);
    close(client_socket);

    return 0;
}
