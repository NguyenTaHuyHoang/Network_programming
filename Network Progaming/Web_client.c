#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#define MAX_RESPONSE_SIZE 1024

int is_content_length(const char *response)
{
    return strstr(response, "Content-Length:") != NULL;
}
int is_transfer_chunked(const char *response)
{
    return strstr(response, "Transfer-Encoding: chunked") != NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Sử dụng: %s <URL> <output_file>\n", argv[0]);
        return 1;
    }

    char *url = argv[1];
    char *output_filename = argv[2];
    char hostname[256];
    char path[256];

    if (sscanf(url, "http://%255[^/]", hostname) == 1)
    {
        // Trường hợp có path
        if (strchr(url, '/'))
        {
            int offset = strlen(hostname) + 7; // 7 là độ dài của "http://"
            strncpy(path, url + offset, 255);
            path[255] = '\0'; // Đảm bảo kết thúc chuỗi path
        }
        else
        {
            // Trường hợp không có path
            path[0] = '\0';
        }
    }
    else
    {
        fprintf(stderr, "URL không hợp lệ.\n");
        return 1;
    }

    if (strlen(hostname) >= 256 || strlen(path) >= 256)
    {
        fprintf(stderr, "Hostname hoặc path quá dài.\n");
        return 1;
    }

    struct hostent *host = gethostbyname(hostname);
    if (!host)
    {
        fprintf(stderr, "Không thể tìm thấy thông tin host.\n");
        return 1;
    }

    // Lay dia chi host
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(80);
    memcpy(&server_address.sin_addr, host->h_addr_list[0], host->h_length);
    // Khoi tao mot socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        fprintf(stderr, "Không thể tạo socket.\n");
        return 1;
    }
    // ket noi toi may chu
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        fprintf(stderr, "Không thể kết nối tới máy chủ.\n");
        return 1;
    }

    char request[512];
    snprintf(request, sizeof(request), "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, hostname);

    if (send(client_socket, request, strlen(request), 0) < 0)
    {
        fprintf(stderr, "Không thể gửi yêu cầu.\n");
        return 1;
    }

    FILE *output_file = fopen(output_filename, "w");
    if (!output_file)
    {
        fprintf(stderr, "Không thể mở tệp để ghi nội dung.\n");
        return 1;
    }

    char response[MAX_RESPONSE_SIZE];
    ssize_t bytes_received;
    int body_started = 0;    // Dùng để xác định khi phần body bắt đầu
    int content_length = -1; // Kích thước của phần body
    while ((bytes_received = recv(client_socket, response, sizeof(response) - 1, 0)) > 0)
    {
        response[bytes_received] = '\0';
        if (!body_started)
        {
            // Tìm chuỗi "\r\n\r\n" để xác định phần body bắt đầu
            char *body_start = strstr(response, "\r\n\r\n");
            if (body_start)
            {
                body_start += 4; // Bỏ qua "\r\n\r\n" và chuyển đến phần body
                body_started = 1;
                // Kiểm tra nếu đây là truyền dữ liệu theo chế độ Content-Length"
                if (is_content_length(response))
                {
                    const char *content_length_str = strstr(response, "Content-Length:") + 15;
                    content_length = atoi(content_length_str);
                    // Kiểm tra nếu đây là truyền dữ liệu theo chế độ "Transfer-Encoding: chunked"
                    if (is_transfer_chunked(response))
                    {
                        char *chunk_start = body_start;
                        while (1)
                        {
                            // Tìm phần cuối của kích thước chunk hiện tại
                            char *chunk_size_end = strstr(chunk_start, "\r\n");
                            if (!chunk_size_end)
                                break;                                      // Không còn đoạn hợp lệ nào nữa
                            int chunk_size = strtol(chunk_start, NULL, 16); // Lấy kích thước chunk
                            if (chunk_size == 0)
                            {
                                break; // Kết thúc dữ liệu chunked
                            }
                            // Di chuyển về đầu chunk data
                            chunk_start = chunk_size_end + 2;
                            // Ghi dữ liệu chunk vào file đầu ra
                            fwrite(chunk_start, 1, chunk_size, output_file);
                            //fwrite(chunk_start, 1, bytes_received - (chunk_start - response), output_file);
                            // Di chuyển tới đoạn tiếp theo
                            if (chunk_start[0] == '\r' && chunk_start[1] == '\n')
                            {
                                chunk_start += 2; // Bỏ qua "\r\n" sau dữ liệu chunk
                            }
                        }
                    }
                    else
                    {
                        // Ghi phần body vào tệp theo kích thước của Content-Length
                        int bytes_to_write = bytes_received - (body_start - response);
                        if (bytes_to_write > content_length)
                        {
                            bytes_to_write = content_length;
                        }
                        fprintf(output_file, "%.*s", bytes_to_write, body_start);
                        content_length -= bytes_to_write;
                    }
                }
                else
                {
                    // Ghi phần body vào tệp (không theo chế độ chunked và không có Content-Length)
                    fprintf(output_file, "%s", body_start);
                }
            }
        }
        else
        {
            // Ghi phần body vào tệp
            fprintf(output_file, "%s", response);

            // Kiểm tra nếu đã nhận đủ kích thước của phần body (nếu có Content-Length)
            if (content_length >= 0)
            {
                content_length -= bytes_received;
                if (content_length <= 0)
                {
                    break; // Đã nhận đủ dữ liệu
                }
            }
        }
    }

    fclose(output_file);
    close(client_socket);

    return 0;
}