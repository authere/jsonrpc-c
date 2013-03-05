/*
 * jrpc_server.h
 *
 * Created on: Feb 20, 2013
 *	Author: mathben
 */

#ifndef JRPC_SERVER_H_
#define JRPC_SERVER_H_

// if libev not define, "select" will be supported
#define LIBEV

#ifdef LIBEV
#include <ev.h>
#endif
#include <signal.h>
#include "jsonrpc-c.h"
#include "jrpc_select.h"

typedef struct {
#ifdef LIBEV
	struct ev_io io;
#endif
	int fd;
	int pos;
	unsigned int buffer_size;
	char *buffer;
	int debug_level;
} jrpc_conn_t;

typedef struct {
	int port_number;
#ifdef LIBEV
	struct ev_loop *loop;
	ev_io listen_watcher;
#else
	volatile int is_running;
	jrpc_select_t jrpc_select;
#endif
	procedure_list_t procedure_list;
	int debug_level;
} jrpc_server_t;

typedef struct {
#ifdef LIBEV
	struct ev_loop *loop;
	struct ev_io *io;
#else
	jrpc_server_t *server;
	jrpc_conn_t *conn;
#endif
} jrpc_loop_t;

void add_signal(jrpc_server_t *server, int signo, struct sigaction *action);
int jrpc_server_init(jrpc_server_t *server, int port_number);
#ifdef LIBEV
int jrpc_server_init_with_ev_loop(jrpc_server_t *server, int port_number,
		struct ev_loop *loop);
#else
int jrpc_server_init_with_select_loop(jrpc_server_t *server, int port_number);
#endif
void jrpc_server_run(jrpc_server_t *server);
int jrpc_server_stop(jrpc_server_t *server);
void jrpc_server_destroy(jrpc_server_t *server);

#endif
