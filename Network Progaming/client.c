#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

// A function to parse the URL and get the host name, path and port number
void parse_url(char *url, char **host, char **path, int *port) {
    char *p;
    p = strstr(url, "://");
    if (p == NULL) {
        fprintf(stderr, "Invalid URL format\n");
        exit(1);
    }
    *p = '\0';
    if (strcmp(url, "http") != 0) {
        fprintf(stderr, "Only http protocol is supported\n");
        exit(1);
    }
    *p = ':';
    p += 3;
    *host = p;
    p = strchr(*host, ':');
    if (p != NULL) {
        *p = '\0';
        p++;
        *port = atoi(p);
        if (*port <= 0) {
            fprintf(stderr, "Invalid port number\n");
            exit(1);
        }
    } else {
        *port = 80;
    }
    p = strchr(*host, '/');
    if (p != NULL) {
        *p = '\0';
        p++;
        *path = p;
    } else {
        *path = "";
    }
}

// A function to create a socket and connect to the server
int create_socket(char *host, int port) {
    int sock;
    struct sockaddr_in server_addr;
    struct hostent *server;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "Host not found\n");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list, server->h_length);
    server_addr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }
    
    return sock;
}

// A function to send a GET request to the server
void send_request(int sock, char *host, char *path) {
    char buffer[BUFFER_SIZE];
    
    sprintf(buffer, "GET /%s HTTP/1.1\r\n", path);
    send(sock, buffer, strlen(buffer), 0);

    sprintf(buffer, "Host: %s\r\n", host);
    send(sock, buffer, strlen(buffer), 0);

    sprintf(buffer, "Connection: close\r\n\r\n");
    send(sock, buffer, strlen(buffer), 0);
}

// A function to receive the response from the server and write it to a file
void receive_response(int sock, char *filename) {
    char buffer[BUFFER_SIZE];
    int n;
    
    FILE *fp = fopen(filename, "w");
    
     // Read the status line
     n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
     if (n <= 0) {
         perror("recv");
         exit(1);
     }
     buffer[n] = '\0';
     printf("%s", buffer);

     // Check if the status code is 200 OK
     if (strstr(buffer, "200 OK") == NULL) {
         fprintf(stderr, "The request failed\n");
         exit(1);
     }

     // Read and ignore the headers until an empty line
     while (1) {
         n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
         if (n <= 0) {
             perror("recv");
             exit(1);
         }
         buffer[n] = '\0';
         printf("%s", buffer);
         if (strcmp(buffer, "\r\n") == 0 || strcmp(buffer, "\n") == 0) {
             break;
         }
     }

     // Check if the response has Content-Length header
     int content_length = -1;
     char *p = strstr(buffer, "Content-Length:");
     if (p != NULL) {
         p += strlen("Content-Length:");
         content_length = atoi(p);
     }

     // Check if the response has Transfer-Encoding: chunked header
     int chunked = 0;
     p = strstr(buffer, "Transfer-Encoding: chunked");
     if (p != NULL) {
         chunked = 1;
     }

     // Read the body of the response
     int total = 0;
     while (1) {
         if (content_length != -1) {
             // If Content-Length is specified, read until the specified number of bytes
             if (total >= content_length) {
                 break;
             }
             n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
             if (n <= 0) {
                 perror("recv");
                 exit(1);
             }
             buffer[n] = '\0';
             fwrite(buffer, 1, n, fp);
             total += n;
         } else if (chunked) {
             // If Transfer-Encoding is chunked, read the chunk size and then the chunk data
             n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
             if (n <= 0) {
                 perror("recv");
                 exit(1);
             }
             buffer[n] = '\0';
             int chunk_size = strtol(buffer, NULL, 16); // Convert hex string to integer
             if (chunk_size == 0) {
                 break; // Last chunk has size zero
             }
             while (chunk_size > 0) {
                 n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
                 if (n <= 0) {
                     perror("recv");
                     exit(1);
                 }
                 buffer[n] = '\0';
                 if (n > chunk_size) {
                     n = chunk_size; // Ignore the trailing \r\n after the chunk data
                 }
                 fwrite(buffer, 1, n, fp);
                 chunk_size -= n;
             }
         } else {
             // If neither Content-Length nor Transfer-Encoding is specified, read until the connection is closed
             n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
             if (n < 0) {
                 perror("recv");
                 exit(1);
             }
             if (n == 0) {
                 break; // Connection closed
             }
             buffer[n] = '\0';
             fwrite(buffer, 1, n, fp);
         }
     }

     fclose(fp);
}

int main(int argc, char *argv[]) {
    char *url;
    char *host;
    char *path;
    int port;
    int sock;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <url> <filename>\n", argv[0]);
        exit(1);
    }

    url = argv[1];
    parse_url(url, &host, &path, &port);

    sock = create_socket(host, port);

    send_request(sock, host, path);

    receive_response(sock, argv[2]);

    close(sock);

    return 0;
}
