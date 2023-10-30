#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
  // Kiểm tra số lượng tham số đầu vào
  if (argc != 2) {
    printf("Sử dụng: %s <địa chỉ URL>\n", argv[0]);
    return 1;
  }

  // Tạo socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket");
    return 1;
  }

  // Cấu hình địa chỉ IP và cổng của máy chủ
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(80);
  inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

  // Kết nối với máy chủ
  int connect_status = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (connect_status < 0) {
    perror("connect");
    return 1;
  }

  // Gửi yêu cầu GET
  char request[] = "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n";
  int send_status = send(sockfd, request, strlen(request), 0);
  if (send_status < 0) {
    perror("send");
    return 1;
  }

  // Nhận phản hồi từ máy chủ
  char buffer[BUFFER_SIZE];
  int recv_status;
  int content_length = 0;
  int chunk_size = 0;
  FILE *fp = fopen("output.txt", "wb");
  while (1) {
    recv_status = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (recv_status < 0) {
      perror("recv");
      return 1;
    }

    if (content_length > 0) {
      fwrite(buffer, 1, recv_status, fp);
      content_length -= recv_status;
      if (content_length <= 0) {
        break;
      }
    } else {
      // Kiểm tra header
      if (strstr(buffer, "Content-Length:") != NULL) {
        // Lấy giá trị Content-Length
        sscanf(buffer, "Content-Length: %d\r\n", &content_length);
      } else if (strstr(buffer, "Transfer-Encoding: chunked") != NULL) {
        // Lấy giá trị chunk size
        sscanf(buffer, "Transfer-Encoding: chunked\r\n\r\n%d\r\n", &chunk_size);
        while (chunk_size > 0) {
          fwrite(buffer, 1, chunk_size, fp);
          recv_status = recv(sockfd, buffer, BUFFER_SIZE, 0);
          if (recv_status < 0) {
            perror("recv");
            return 1;
          }
          if (buffer[0] == '\r' && buffer[1] == '\n') {
            chunk_size = 0;
          } else {
            sscanf(buffer, "%x\r\n", &chunk_size);
          }
        }
      }
    }
  }

  // Đóng file
  fclose(fp);

  // Đóng socket
  close(sockfd);

  return 0;
}
