#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 6969
#define BUFFER_SIZE 2048
#define WEB_ROOT "."

struct thread_args {
  int client_sock;
  char uri[1000];
  const char *web_root;
};

const char *get_mime_type(const char *filename) {
  const char *ext = strrchr(filename, '.');
  if (!ext)
    return "application/octet-stream";

  if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
    return "text/html";
  if (strcmp(ext, ".css") == 0)
    return "text/css";
  if (strcmp(ext, ".js") == 0)
    return "application/javascript";
  if (strcmp(ext, ".json") == 0)
    return "application/json";
  if (strcmp(ext, ".png") == 0)
    return "image/png";
  if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
    return "image/jpeg";
  if (strcmp(ext, ".gif") == 0)
    return "image/gif";
  if (strcmp(ext, ".svg") == 0)
    return "image/svg+xml";
  if (strcmp(ext, ".txt") == 0)
    return "text/plain";
  if (strcmp(ext, ".ico") == 0)
    return "image/x-icon";
  return "application/octet-stream";
}

void serve_html(int client_sock, char uri[1000], const char *web_root) {
  if (strcmp(uri, "/") == 0) {
    strcpy(uri, "/index.html");
  }

  char filepath[BUFFER_SIZE];
  snprintf(filepath, sizeof(filepath), "%s%s", web_root, uri);
  printf("Serving file: %s\n", filepath);

  int template = open(filepath, O_RDONLY);
  if (template <0) {
    perror("Failed to open file");
    return;
  }

  const char *mime_type = get_mime_type(filepath);

  char header[BUFFER_SIZE];
  snprintf(header, sizeof(header),
           "HTTP/1.0 200 OK\r\n"
           "Server: webserver-c\r\n"
           "Content-Type: %s\r\n"
           "\r\n",
           mime_type);

  if (write(client_sock, header, strlen(header)) < 0) {
    perror("Failed to send headers");
    close(template);
    return;
  }

  char file_buffer[BUFFER_SIZE];
  ssize_t bytes_read, bytes_sent;

  while ((bytes_read = read(template, file_buffer, sizeof(file_buffer))) > 0) {
    bytes_sent = write(client_sock, file_buffer, bytes_read);
    if (bytes_sent < 0) {
      perror("Failed to send data");
      break;
    }
  }

  close(template);
}

void *handle_client(void *args_ptr) {
  struct thread_args *args = (struct thread_args *)args_ptr;

  serve_html(args->client_sock, args->uri, args->web_root);
  close(args->client_sock);
  free(args);
  return NULL;
}

int main(int argc, char *argv[]) {
  const char *web_root;
  if (argc > 1) {
    web_root = argv[1];
  } else {
    web_root = WEB_ROOT;
  }

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

  struct sockaddr_in host_addr;
  socklen_t host_addrlen = sizeof(host_addr);
  host_addr.sin_family = AF_INET;
  host_addr.sin_port = htons(PORT);
  host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0) {
    perror("webserver (bind)");
    close(sockfd);
    return 1;
  }
  printf("socket successfully bound to address\n");

  if (listen(sockfd, SOMAXCONN) != 0) {
    perror("webserver (listen)");
    close(sockfd);
    return 1;
  }
  printf("server listening for connections on port %d\n", PORT);

  for (;;) {
    int newsockfd =
        accept(sockfd, (struct sockaddr *)&host_addr, &host_addrlen);
    if (newsockfd < 0) {
      perror("webserver (accept)");
      continue;
    }

    char buffer[BUFFER_SIZE];
    int valread = read(newsockfd, buffer, BUFFER_SIZE);
    if (valread < 0) {
      perror("webserver (read)");
      close(newsockfd);
      continue;
    }

    char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];
    sscanf(buffer, "%s %s %s", method, uri, version);
    printf("Incoming request for URI: %s\n", uri);

    struct thread_args *args = malloc(sizeof(struct thread_args));
    args->client_sock = newsockfd;
    strncpy(args->uri, uri, sizeof(args->uri) - 1);
    args->uri[sizeof(args->uri) - 1] = '\0';
    args->web_root = web_root;

    pthread_t tid;
    if (pthread_create(&tid, NULL, handle_client, args) != 0) {
      perror("Failed to create thread");
      close(newsockfd);
      free(args);
    } else {
      pthread_detach(tid);
    }
  }

  close(sockfd);
  return 0;
}
