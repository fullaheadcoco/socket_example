#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>

#define PORT 12345
#define MAX_CLIENTS 10

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL) 실패");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL) 실패");
        return -1;
    }
    return 0;
}

int main() {
    int server_fd, new_socket, addrlen;
    struct sockaddr_in address;
    struct pollfd poll_fds[MAX_CLIENTS + 1]; // 서버 소켓 포함
    char buffer[1024];

    // 1. 소켓 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("소켓 생성 실패");
        exit(EXIT_FAILURE);
    }

    // 2. 소켓 옵션 설정 (SO_REUSEADDR)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt 실패");
        exit(EXIT_FAILURE);
    }

    // 3. 서버 주소 구조체 설정
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 4. 소켓 바인딩
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("바인딩 실패");
        exit(EXIT_FAILURE);
    }

    // 5. 리스닝 설정
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen 실패");
        exit(EXIT_FAILURE);
    }

    printf("서버가 포트 %d에서 대기 중...\n", PORT);

    // 6. 논블로킹 모드 설정
    if (set_nonblocking(server_fd) == -1) {
        exit(EXIT_FAILURE);
    }

    // 7. poll()을 위한 poll_fds 배열 초기화
    poll_fds[0].fd = server_fd;
    poll_fds[0].events = POLLIN;  // 서버 소켓은 새로운 연결을 감지해야 함

    for (int i = 1; i <= MAX_CLIENTS; i++) {
        poll_fds[i].fd = -1; // 비어있는 슬롯
    }

    while (1) {
        int poll_count = poll(poll_fds, MAX_CLIENTS + 1, -1);
        if (poll_count == -1) {
            if (errno == EINTR) {
                printf("poll()이 시그널에 의해 인터럽트됨, 계속 실행\n");
                continue;
            }
            perror("poll() 실패");
            exit(EXIT_FAILURE);
        }

        // 8. 새로운 연결 요청 처리
        if (poll_fds[0].revents & POLLIN) {
            addrlen = sizeof(address);
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (new_socket == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    printf("비동기 accept() 호출, 처리할 연결 없음\n");
                } else {
                    perror("accept() 실패");
                }
            } else {
                printf("새로운 클라이언트 연결: %d\n", new_socket);
                set_nonblocking(new_socket);

                // 빈 슬롯 찾기
                for (int i = 1; i <= MAX_CLIENTS; i++) {
                    if (poll_fds[i].fd == -1) {
                        poll_fds[i].fd = new_socket;
                        poll_fds[i].events = POLLIN;
                        break;
                    }
                }
            }
        }

        // 9. 클라이언트 데이터 처리
        for (int i = 1; i <= MAX_CLIENTS; i++) {
            if (poll_fds[i].fd != -1 && (poll_fds[i].revents & POLLIN)) {
                int bytes_read = read(poll_fds[i].fd, buffer, sizeof(buffer));
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    printf("클라이언트 %d로부터 데이터 수신: %s\n", poll_fds[i].fd, buffer);
                } else if (bytes_read == 0 || (bytes_read == -1 && (errno == ECONNRESET || errno == EPIPE))) {
                    printf("클라이언트 %d 연결 종료\n", poll_fds[i].fd);
                    close(poll_fds[i].fd);
                    poll_fds[i].fd = -1;
                } else if (bytes_read == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                    }
                    perror("read() 실패");
                    close(poll_fds[i].fd);
                    poll_fds[i].fd = -1;
                }
            }
        }

        // 10. 클라이언트 비정상 종료 감지
        for (int i = 1; i <= MAX_CLIENTS; i++) {
            if (poll_fds[i].fd != -1) {
                if (poll_fds[i].revents & (POLLHUP | POLLERR)) {
                    printf("클라이언트 %d 비정상 종료 감지\n", poll_fds[i].fd);
                    close(poll_fds[i].fd);
                    poll_fds[i].fd = -1;
                }
            }
        }
    }

    close(server_fd);
    return 0;
}