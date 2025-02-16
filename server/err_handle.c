#include "err_handle.h"
#include <errno.h>
#include <stdio.h>

int handle_socket_error() {
    switch (errno) {
        case EACCES:
            perror("[Error] socket() failed: Permission denied");
            return 0;
        case EAFNOSUPPORT:
            perror("[Error] socket() failed: Address family not supported");
            return 0;
        case EINVAL:
            perror("[Error] socket() failed: Invalid domain or type");
            return 0;
        case EMFILE:
            perror("[Error] socket() failed: Process file descriptor limit reached");
            return 0;
        case ENFILE:
            perror("[Error] socket() failed: System file descriptor limit reached");
            return 0;
        case ENOBUFS:
            perror("[Error] socket() failed: Insufficient buffer space available");
            return 1; // retry
        case ENOMEM:
            perror("[Error] socket() failed: Not enough memory");
            return 1; // retry
        case EPROTONOSUPPORT:
            perror("[Error] socket() failed: Protocol not supported");
            return 1;
        default:
            perror("[Error] socket() failed");
            return 0;
    }
}

int handle_setsockopt_error() {
    switch (errno) {
        case EBADF:
            perror("[Error] setsockopt() failed: Invalid socket descriptor");
            return 0;
        case EFAULT:
            perror("[Error] setsockopt() failed: Invalid option value pointer");
            return 0;
        case EINVAL:
            perror("[Error] setsockopt() failed: Invalid option length");
            return 0;
        case ENOBUFS:
            perror("[Error] setsockopt() failed: Insufficient buffer space available");
            return 1; // retry
        case ENOMEM:
            perror("[Error] setsockopt() failed: Not enough memory");
            return 1; // retry
        case ENOPROTOOPT:
            perror("[Error] setsockopt() failed: Protocol not available");
            return 0;
        case ENOTSOCK:
            perror("[Error] setsockopt() failed: Socket descriptor is not valid");
            return 0;
        case EDOM:
            perror("[Error] setsockopt() failed: Invalid option name");
            return 0;
        case EISCONN:
            perror("[Error] setsockopt() failed: Socket is already connected");
            return 0;
        case EOPNOTSUPP:
            perror("[Error] setsockopt() failed: Socket is not of type SOCK_STREAM");
            return 0;
        default:
            perror("[Error] setsockopt() failed");
            return 0;
    }
}

int handle_bind_error() {
    switch (errno) {
        case EACCES:
            perror("[Error] bind() failed: Permission denied");
            return 0;
        case EADDRINUSE:
            perror("[Error] bind() failed: Address already in use");
            return 1; // retry
        case EBADF:
            perror("[Error] bind() failed: Invalid socket descriptor");
            return 0;
        case EINVAL:
            perror("[Error] bind() failed: Invalid address length");
            return 0;
        case ENOTSOCK:
            perror("[Error] bind() failed: Socket descriptor is not valid");
            return 0;
        case EADDRNOTAVAIL:
            perror("[Error] bind() failed: Address not available");
            return 0;
        case EFAULT:
            perror("[Error] bind() failed: Invalid address pointer");
            return 0;
        case ELOOP:
            perror("[Error] bind() failed: Too many symbolic links in resolving address");
            return 0;
        case ENAMETOOLONG:
            perror("[Error] bind() failed: Address too long");
            return 0;
        case ENOENT:
            perror("[Error] bind() failed: Address does not exist");
            return 0;
        case ENOMEM:
            perror("[Error] bind() failed: Not enough memory");
            return 1; // retry
        case ENOTDIR:
            perror("[Error] bind() failed: Not a valid directory");
            return 0;
        case EROFS:
            perror("[Error] bind() failed: Read-only file system");
            return 0;
        default:
            perror("[Error] bind() failed");
            return 0;
    }
}

int handle_listen_error() {
    switch (errno) {
        case EBADF:
            perror("[Error] listen() failed: Invalid socket descriptor");
            return 0;
        case EDESTADDRREQ:
            perror("[Error] listen() failed: Destination address required");
            return 1; // retry
        case EINVAL:
            perror("[Error] listen() failed: Socket is not bound");
            return 0;
        case ENOTSOCK:
            perror("[Error] listen() failed: Socket descriptor is not valid");
            return 0;
        case EOPNOTSUPP:
            perror("[Error] listen() failed: Socket is not of type SOCK_STREAM");
            return 0;
        default:
            perror("[Error] listen() failed");
            return 0;
    }
}

int handle_poll_error() {
    switch (errno) {
        case EAGAIN:
            printf("[Error] poll() failed: Temporary resource shortage, retrying...\n");
            return 1; // retry
        case EFAULT:
            perror("[Error] poll() failed: Invalid fds pointer");
            return 0;
        case EINTR:
            printf("[Warning] poll() interrupted by signal, retrying...\n");
            return 1; // retry
        case EINVAL:
            perror("[Error] poll() failed: Invalid nfds or timeout value");
            return 0;
        default:
            perror("[Error] poll() failed");
            return 0;
    }
}

int handle_accept_error() {
    switch (errno) {
        case EBADF:
            perror("[Error] accept() failed: Invalid socket descriptor");
            return 0;
        case ECONNABORTED:
            printf("[Warning] accept() failed: Connection aborted, retrying...\n");
            return 1; // retry
        case EFAULT:
            perror("[Error] accept() failed: Invalid address pointer");
            return 0;
        case EINTR:
            printf("[Warning] accept() interrupted by signal, retrying...\n");
            return 1; // retry
        case EINVAL:
            perror("[Error] accept() failed: Socket is not listening");
            return 0;
        case EMFILE:
            perror("[Error] accept() failed: Process file descriptor limit reached");
            return 0;
        case ENFILE:
            perror("[Error] accept() failed: System file descriptor limit reached");
            return 0;
        case ENOMEM:
            perror("[Error] accept() failed: Not enough memory");
            return 0;
        case ENOTSOCK:
            perror("[Error] accept() failed: Socket descriptor is not valid");
            return 0;
        case EOPNOTSUPP:
            perror("[Error] accept() failed: Socket is not of type SOCK_STREAM");
            return 0;
        case EWOULDBLOCK:
            printf("[Error] accept() failed: No pending connections, retrying...\n");
            return 1; // retry
        default:
            perror("[Error] accept() failed");
            return 0;
    }
}

int handle_receive_error() {
    switch (errno) {
        case EAGAIN: // EWOULDBLOCK
            return 1; // retry
        case EBADF:
            perror("[Error] recv() failed: Invalid socket descriptor");
            return 0;
        case ECONNREFUSED:
            printf("[Warning] recv() failed: Connection refused, closing...\n");
            return 0;
        case EFAULT:
            perror("[Error] recv() failed: Invalid buffer pointer");
            return 0;
        case EINTR:
            printf("[Warning] recv() interrupted by signal, retrying...\n");
            return 1; // retry
        case EINVAL:
            perror("[Error] recv() failed: Invalid buffer length");
            return 0;
        case ENOMEM:
            perror("[Error] recv() failed: Not enough memory");
            return 0;
        case ENOTCONN:
            perror("[Error] recv() failed: Socket is not connected");
            return 0;
        case ENOTSOCK:
            perror("[Error] recv() failed: Socket descriptor is not valid");
            return 0;
        default:
            perror("[Error] recv() failed");
            return 0;
    }
}

int handle_send_error() {
    switch (errno) {
        case EACCES:
            perror("[Error] send() failed: Permission denied");
            return 0;
        case EAGAIN: // EWOULDBLOCK
            return 1; // retry
        case EALREADY:
            perror("[Error] send() failed: Operation already in progress");
            return 0;
        case EBADF:
            perror("[Error] send() failed: Invalid socket descriptor");
            return 0;
        case ECONNRESET:
            perror("[Error] send() failed: Connection reset by peer");
            return 0;
        case EDESTADDRREQ:
            perror("[Error] send() failed: Destination address required");
            return 0;
        case EFAULT:
            perror("[Error] send() failed: Invalid buffer pointer");
            return 0;
        case EINTR:
            printf("[Warning] send() interrupted by signal, retrying...\n");
            return 1; // retry
        case EINVAL:
            perror("[Error] send() failed: Invalid buffer length");
            return 0;
        case EISCONN:
            perror("[Error] send() failed: Socket is already connected");
            return 0;
        case EMSGSIZE:
            perror("[Error] send() failed: Message too long");
            return 0;
        case ENOBUFS:
            perror("[Error] send() failed: Insufficient buffer space available");
            return 1; // retry
        case ENOMEM:
            perror("[Error] send() failed: Not enough memory");
            return 0;
        case ENOTCONN:
            perror("[Error] send() failed: Socket is not connected");
            return 0;
        case ENOTSOCK:
            perror("[Error] send() failed: Socket descriptor is not valid");
            return 0;
        case EOPNOTSUPP:
            perror("[Error] send() failed: Operation not supported on socket");
            return 0;
        case EPIPE:
            perror("[Error] send() failed: Broken pipe");
            return 0;
        default:
            perror("[Error] send() failed");
            return 0;
    }
}