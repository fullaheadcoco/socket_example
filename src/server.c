#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include "../include/err_handle.h"

#define PORT 12345
#define BUFFER_SIZE 4
#define MAX_CLIENTS 1024

struct client_buffer {
    char *data;
    size_t length;
    size_t capacity;
};

struct client_buffer client_buffers[MAX_CLIENTS] = {0};
int server_listen_ok = 0;

// 소켓을 논블로킹 모드로 설정
void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

void set_cloexec(int sockfd) {
    int flags = fcntl(sockfd, F_GETFD, 0);
    fcntl(sockfd, F_SETFD, flags | FD_CLOEXEC);
}

// 클라이언트 버퍼 초기화
int init_client_buffer(int client_fd) {
    client_buffers[client_fd].data = (char *) malloc(BUFFER_SIZE);
    if (!client_buffers[client_fd].data) {
        return 0;
    }
    client_buffers[client_fd].length = 0;
    client_buffers[client_fd].capacity = BUFFER_SIZE;
    return 1;
}

// 클라이언트 버퍼 해제
void free_client_buffer(int client_fd) {
    free(client_buffers[client_fd].data);
    client_buffers[client_fd].data = NULL;
    client_buffers[client_fd].length = 0;
    client_buffers[client_fd].capacity = 0;
}

// 데이터를 버퍼에 추가
int append_to_buffer(int client_fd, const char *data, size_t len, struct pollfd *fds, int nfds) {
    struct client_buffer *buf = &client_buffers[client_fd];

    if (buf->length + len > buf->capacity) {
        size_t new_capacity = buf->capacity * 2;
        char *new_data = (char *) realloc(buf->data, new_capacity);
        if (!new_data) {
            return 0;
        }
        buf->data = new_data;
        buf->capacity = new_capacity;
    }

    // 기존 데이터 뒤에 새로운 데이터 추가
    memcpy(buf->data + buf->length, data, len);
    buf->length += len;

    // 버퍼에 데이터가 존재하므로 POLLOUT 활성화
    for (int i = 0; i < nfds; i++) {
        if (fds[i].fd == client_fd) {
            fds[i].events |= POLLOUT;
            break;
        }
    }

    return 1;
}

// 클라이언트에서 데이터를 수신하고 버퍼에 저장
int receive_data(int client_fd, struct pollfd *fds, int nfds) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);

    if (bytes > 0) {
        if (!append_to_buffer(client_fd, buffer, bytes, fds, nfds)) {
            return 0; // 메모리 부족
        }
        return 1;
    } else if (bytes == 0) {
        // 클라이언트가 연결 종료
        // 클라이언트 소켓을 닫고 버퍼를 해제
        return 0;
    } else {
        if (!handle_receive_error()) {
            return 0; // 오류 발생
        } else {
            return 1; // 재시도
        }
    }
}

// 클라이언트에게 데이터 전송
int send_data(int client_fd, struct pollfd *fds, int nfds) {
    struct client_buffer *buf = &client_buffers[client_fd];

    if (buf->length == 0) return 1; // 보낼 데이터 없음

    ssize_t bytes_sent = send(client_fd, buf->data, buf->length, 0);

    if (bytes_sent > 0) {
        memmove(buf->data, buf->data + bytes_sent, buf->length - bytes_sent);
        buf->length -= bytes_sent;

        // 데이터가 모두 전송되었으면 POLLOUT 비활성화
        if (buf->length == 0) {
            for (int i = 0; i < nfds; i++) {
                if (fds[i].fd == client_fd) {
                    fds[i].events &= ~POLLOUT;
                    // TODO: buf->data 초기화
                    break;
                }
            }
        }
        return 1;
    } else if (bytes_sent < 0) {
        if (!handle_send_error()) {
            return 0; // 오류 발생
        } else {
            return 1; // 재시도
        }
    }

    return 0;
}

int main() {
    struct pollfd fds[MAX_CLIENTS] = {0};
    int nfds = -1;
    int server_fd = -1;

    do {
        server_fd = socket(AF_INET, SOCK_STREAM | O_NONBLOCK | O_CLOEXEC, 0);
        if (server_fd == -1) {
            if (handle_socket_error()) {
                continue;
            }
            exit(EXIT_FAILURE);
        }

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            if (handle_setsockopt_error()) {
                close(server_fd);
                continue;
            }
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in server_addr = {0};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(PORT);

        if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
            if (handle_bind_error()) {
                close(server_fd);
                continue;
            }
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 10) == -1) {
            if (handle_listen_error()) {
                close(server_fd);
                continue;
            }
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        // init fds for server_fd
        nfds = 1;
        fds[0].fd = server_fd;
        fds[0].events = POLLIN;

        // server listen success
        server_listen_ok = 1;
    } while (!server_listen_ok);

    printf("Server listening on port %d\n", PORT);

    while (1) {
        printf("-------------------------------\n");
        const int rc = poll(fds, nfds, 5000);
        if (rc < 0) {
            if (handle_poll_error()) {
                continue;
            }
            break;
        }
        if (rc == 0) {
            printf("poll timeout\n");
            continue;
        }

        printf("poll event: %d fds\n", rc);

        for (int i = 0; i < nfds; i++) {
            printf("fd=%d, events=%d, revents=%d\n", fds[i].fd, fds[i].events, fds[i].revents);

            if (fds[i].revents == 0) {
                // 이벤트 없음
                continue;
            }

            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == server_fd) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len);
                    if (client_fd < 0) {
                        if (handle_accept_error()) {
                            continue;
                        }
                        break;
                    }

                    set_nonblocking(client_fd);
                    set_cloexec(client_fd);

                    if (!init_client_buffer(client_fd)) {
                        perror("Memory allocation failed");
                        close(client_fd);
                        continue;
                    }
                    fds[nfds].fd = client_fd;
                    fds[nfds].events = POLLIN;
                    nfds++;

                    printf("New client connected: %d\n", client_fd);
                } else {
                    int client_fd = fds[i].fd;
                    if (!receive_data(client_fd, fds, nfds)) {
                        close(client_fd);
                        free_client_buffer(client_fd);
                        fds[i] = fds[nfds - 1];
                        nfds--;
                        printf("Client disconnected: %d\n", client_fd);
                    } else {
                        printf("Received data from client: %d\n", client_fd);
                        // continue;
                    }
                }
            } else if (fds[i].revents & POLLOUT) {
                int client_fd = fds[i].fd;
                if (!send_data(client_fd, fds, nfds)) {
                    close(client_fd);
                    free_client_buffer(client_fd);
                    fds[i] = fds[nfds - 1];
                    nfds--;
                    printf("Client disconnected (send error): %d\n", client_fd);
                } else {
                    printf("Sent data to client: %d\n", client_fd);
                }
            } else if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                int client_fd = fds[i].fd;
                close(client_fd);
                free_client_buffer(client_fd);
                fds[i] = fds[nfds - 1];
                nfds--;
                printf("Client disconnected (error): %d\n", client_fd);
            } else {
                printf("Unexpected poll event: %d\n", fds[i].revents);
                // continue;
            }
        }
    }

    for (int i = 0; i < nfds; i++) {
        close(fds[i].fd);
        free_client_buffer(fds[i].fd);
    }

    return 0;
}
