#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"  // 서버 주소
#define SERVER_PORT 12345       // 서버 포트
#define BUFFER_SIZE 1024
#define TIMEOUT 5000            // 5초 타임아웃

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
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // 소켓 생성
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket() 실패");
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

    // 논블로킹 소켓 설정
    set_nonblocking(sockfd);

    // 서버에 연결 시도
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        if (errno != EINPROGRESS) {
            perror("connect() 실패");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }

    // poll()을 사용하여 stdin과 소켓을 감시
    struct pollfd poll_fds[2];
    poll_fds[0].fd = STDIN_FILENO; // 표준 입력 (키보드)
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = sockfd;       // 소켓
    poll_fds[1].events = POLLIN | POLLOUT;

    while (1) {
        int poll_count = poll(poll_fds, 2, TIMEOUT);

        if (poll_count == -1) {
            if (errno == EINTR) {
                continue; // 시그널로 인한 인터럽트 -> 다시 실행
            }
            perror("poll() 실패");
            break;
        } else if (poll_count == 0) {
            fprintf(stderr, "poll() 타임아웃\n");
            continue;
        }

        // 키보드 입력 감지
        if (poll_fds[0].revents & POLLIN) {
            ssize_t bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                perror("read(STDIN) 실패");
                break;
            }

            // 서버로 데이터 전송
            ssize_t bytes_sent = send(sockfd, buffer, bytes_read, 0);
            if (bytes_sent == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                perror("send() 실패");
                break;
            }
        }

        // 서버 응답 감지
        if (poll_fds[1].revents & POLLIN) {
            ssize_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                perror("recv() 실패");
                break;
            } else if (bytes_received == 0) {
                printf("서버가 연결을 종료했습니다.\n");
                break;
            }

            buffer[bytes_received] = '\0';
            printf("서버: %s", buffer);
        }

        // 서버 연결 종료 감지
        if (poll_fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fprintf(stderr, "서버 연결 종료됨\n");
            break;
        }
    }

    close(sockfd);
    return 0;
}