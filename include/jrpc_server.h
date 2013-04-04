/*
 * jrpc_server.h
 *
 * Created on: Feb 20, 2013
 *	Author: mathben
 */

#ifndef JRPC_SERVER_H_
#define JRPC_SERVER_H_

#include <signal.h>
#include "jsonrpc-c.h"
#include "jrpc_select.h"

typedef struct {
	int fd;
	int pos;
	unsigned int buffer_size;
	char *buffer;
	int debug_level;
} jrpc_conn_t;

typedef struct {
	int port_number;
	int is_running;
	jrpc_select_t jrpc_select;
	procedure_list_t procedure_list;
	int debug_level;
} jrpc_server_t;

typedef struct {
	jrpc_server_t *server;
	jrpc_conn_t *conn;
} jrpc_loop_t;

int jrpc_server_init(jrpc_server_t **server, int port_number);
int jrpc_server_init_with_select_loop(jrpc_server_t **server, int port_number);
void jrpc_server_run(jrpc_server_t *server);
int jrpc_server_stop(jrpc_server_t *server);
void jrpc_server_destroy(jrpc_server_t *server);

#endif
