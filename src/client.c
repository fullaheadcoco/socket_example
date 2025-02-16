#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../include/err_handle.h"

#define SERVER_IP "127.0.0.1"  // 서버 주소
#define SERVER_PORT 12345       // 서버 포트
#define BUFFER_SIZE 1024
#define TIMEOUT 5000            // 5초 타임아웃
#define MAX_RETRIES 5           // 최대 재시도 횟수

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL) 실패");
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL) 실패");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int sockfd = -1;
    struct sockaddr_in server_addr = {0};
    char buffer[BUFFER_SIZE] = {0};
    int socket_create_ok = 0;

    do {
        // 소켓 생성
        sockfd = socket(AF_INET, SOCK_STREAM | O_NONBLOCK | O_CLOEXEC, 0);
        if (sockfd == -1) {
            if (handle_socket_error()) {
                continue;
            }
            exit(EXIT_FAILURE);
        }

        // 서버 주소 설정
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
            perror("inet_pton() 실패");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // 소켓을 논블로킹 모드로 설정
        // set_nonblocking(sockfd);
        set_nonblocking(STDIN_FILENO);

        socket_create_ok = 1;
    } while (!socket_create_ok);

    int retries = 0;
    while (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
//        if (handle_connect_error()) {
//            continue;
//        }

        if (errno == EINPROGRESS) {
            struct pollfd poll_fd;
            poll_fd.fd = sockfd;
            //poll()을 이용하여 소켓이 연결 가능(POLLOUT) 상태가 되는지 감시
            poll_fd.events = POLLOUT;

            int ret = poll(&poll_fd, 1, TIMEOUT);
            if (ret == -1) {
                perror("poll() 실패");
                close(sockfd);
                exit(EXIT_FAILURE);
                //poll() 함수가 타임아웃 되거나, poll()이 반환되었지만, POLLOUT 이벤트가 발생하지 않은 경우
            } else {
                fprintf(stderr, "서버 연결 시간 초과 또는 %s, 재시도 중 (%d/%d)\n", strerror(errno),retries + 1, MAX_RETRIES);
                if (++retries >= MAX_RETRIES) {
                    fprintf(stderr, "최대 재시도 횟수 초과, 종료합니다.\n");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
                continue;
            }
        } else {
            perror("connect() 실패");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }

    printf("서버에 성공적으로 연결되었습니다.\n");

    struct pollfd poll_fds[2];
    poll_fds[0].fd = STDIN_FILENO;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = sockfd;
    poll_fds[1].events = POLLIN | POLLOUT;

    while (1) {
        int poll_count = poll(poll_fds, 2, TIMEOUT);

        if (poll_count == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll() 실패");
            break;
        } else if (poll_count == 0) {
            fprintf(stderr, "poll() 타임아웃\n");
            continue;
        }

        if (poll_fds[0].revents & POLLIN) {
            ssize_t bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            if (bytes_read > 0) {
                send(sockfd, buffer, bytes_read, 0);
            }
        }

        if (poll_fds[1].revents & POLLIN) {
            ssize_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                printf("서버: %s", buffer);
            } else if (bytes_received == 0) {
                printf("서버가 연결을 종료했습니다.\n");
                break;
            }
        }

        if (poll_fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fprintf(stderr, "서버 연결이 끊어졌습니다.\n");
            break;
        }
    }

    close(sockfd);
    return 0;
}
