#define PORT 6969
#define BUFFER_SIZE 2048
#define WEB_ROOT ""

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct sockaddr_in host_addr;

void serve_html(int client_sock, char uri[1000], const char *web_root) {
  char header[] = "HTTP/1.0 200 OK\r\n"
                  "Server: webserver-c\r\n"
                  "Content-Type: text/html\r\n"
                  "\r\n"; // End of headers

  char filepath[BUFFER_SIZE];
  snprintf(filepath, sizeof(filepath), "%s%s", web_root, uri);
  printf("%s\n", filepath);
  int template = open(filepath, O_RDONLY);
  if (write(client_sock, header, strlen(header)) < 0) {
    perror("Failed to send headers");
    close(template);
    return;
  }

  if (template <0) {
    perror("Failed to open file");
    return;
  }

  char file_buffer[BUFFER_SIZE];
  ssize_t bytes_read, bytes_sent;

  while ((bytes_read = read(template, file_buffer, sizeof(file_buffer))) > 0) {
    bytes_sent = write(client_sock, file_buffer, bytes_read);
    if (bytes_sent < 0) {
      perror("Failed to send data");
      close(template);
      return;
    }
  }
  close(template);
}

int main(int argc, char *argv[]) {
  const char *web_root;
  if (argc > 1) {
    web_root = argv[1];
  } else {
    web_root = WEB_ROOT;
  }
  char buffer[BUFFER_SIZE];
  // Create a socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("webserver (socket)");
    return 1;
  }
  printf("socket created successfully\n");

  int optval = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) <
      0) {
    perror("webserver (setsockopt)");
    close(sockfd);
    return 1;
  }

  // Create the address to bind the socket to
  struct sockaddr_in host_addr;
  int host_addrlen = sizeof(host_addr);

  host_addr.sin_family = AF_INET;
  host_addr.sin_port = htons(PORT);
  host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // Create client address
  struct sockaddr_in client_addr;
  int client_addrlen = sizeof(client_addr);
  // Bind the socket to the address
  if (bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0) {
    perror("webserver (bind)");
    return 1;
  }
  printf("socket successfully bound to address\n");

  // Listen for incoming connections
  if (listen(sockfd, SOMAXCONN) != 0) {
    perror("webserver (listen)");
    return 1;
  }
  printf("server listening for connections\n");

  for (;;) {
    // Accept incoming connections
    int newsockfd = accept(sockfd, (struct sockaddr *)&host_addr,
                           (socklen_t *)&host_addrlen);
    if (newsockfd < 0) {
      perror("webserver (accept)");
      continue;
    }
    printf("connection accepted\n");
    // Get client address
    char request[BUFFER_SIZE];
    int sockn = getsockname(newsockfd, (struct sockaddr *)&client_addr,
                            (socklen_t *)&client_addrlen);
    int valread = read(newsockfd, buffer, BUFFER_SIZE);

    if (valread < 0) {
      perror("webserver (read)");
      continue;
    }
    int client_port_number = ntohs(client_addr.sin_port);
    /* printf("Host order: 0x%xun", client_port_number); */

    // Read the request
    char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];

    sscanf(buffer, "%s %s %s", method, uri, version);
    printf("%s\n", uri);
    /* printf("[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port), method, version, uri);
 */

    serve_html(newsockfd, uri, web_root);
    close(newsockfd);
  }
  close(sockfd);
  return 0;
}
