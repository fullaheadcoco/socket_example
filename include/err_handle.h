#ifndef __ERR_HANDLE_H__
#define __ERR_HANDLE_H__
#include <stdio.h>
#include <errno.h>

int handle_socket_error();
int handle_setsockopt_error();
int handle_bind_error();
int handle_listen_error();
int handle_poll_error();
int handle_accept_error();
int handle_receive_error();
int handle_send_error();
int handle_connect_error();

#endif // __ERR_HANDLE_H__