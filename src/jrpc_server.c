/*
 * jrpc_server.c
 *
 * Created on: Feb 20, 2013
 *	Author: mathben
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "jrpc_server.h"

static int jrpc_server_start(jrpc_server_t *server);

void add_signal(jrpc_server_t *server, int signo, struct sigaction *action) {
	sigaction(signo, action, NULL);
}

// get sockaddr, IPv4 or IPv6:
static void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

static void close_connection(jrpc_loop_t *jrpc_loop) {
	jrpc_conn_t *conn;
	int res;
	jrpc_server_t *server = jrpc_loop->server;
	conn = jrpc_loop->conn;
	res = remove_select_fds(&server->jrpc_select.fds_read, conn->fd, 1);
	if (res == -1) {
		fprintf(stderr, "Internal error, cannot remove fd %d\n", conn->fd);
		exit(EXIT_FAILURE);
	}
	close(conn->fd);
	free(conn->buffer);
	free(conn);
}

static void connection_cb(int fd, jrpc_loop_t *jrpc_loop) {
	jrpc_conn_t *conn;
	jrpc_server_t *server;
	conn = jrpc_loop->conn;
	server = jrpc_loop->server;
	jrpc_request_t request;
	size_t bytes_read = 0;
	request.fd = fd;
	request.debug_level = conn->debug_level;

	if (server->debug_level)
		printf("callback from fd %d\n", request.fd);

	if (conn->pos == (conn->buffer_size - 1)) {
		char *new_buffer = realloc(conn->buffer, conn->buffer_size *= 2);
		if (new_buffer == NULL) {
			perror("Memory error");
			return close_connection(jrpc_loop);
		}
		conn->buffer = new_buffer;
		memset(conn->buffer + conn->pos, 0, conn->buffer_size - conn->pos);
	}
	// can not fill the entire buffer, string must be NULL terminated
	int max_read_size = conn->buffer_size - conn->pos - 1;
	if ((bytes_read = read(fd, conn->buffer + conn->pos, max_read_size)) == -1) {
		perror("read");
		return close_connection(jrpc_loop);
	}
	if (!bytes_read) {
		// client closed the sending half of the connection
		if (server->debug_level)
			printf("Client closed connection.\n");
		return close_connection(jrpc_loop);
	}

	char *end_ptr;
	char *err_msg = "Parse error. Invalid JSON was received by the server.";
	conn->pos += bytes_read;
	cJSON *root = cJSON_Parse_Stream(conn->buffer, &end_ptr);

	if (root == NULL) {
		/* did we parse the all buffer? If so, just wait for more.
		* else there was an error before the buffer's end
		*/
		if (cJSON_GetErrorPtr() != (conn->buffer + conn->pos)) {
			if (server->debug_level) {
				printf("INVALID JSON Received:\n---\n%s\n---\nClose fd %d\n",
						conn->buffer, fd);
			}
			request.id = NULL;
			send_error(&request, JRPC_PARSE_ERROR, strdup(err_msg));
			return close_connection(jrpc_loop);
		}
		/* receive nothing */
		return;
	}
	if (server->debug_level) {
		err_msg = cJSON_Print(root);
		printf("Valid JSON Received:\n%s\n", err_msg);
		free(err_msg);
	}

	if (root->type == cJSON_Object) {
		eval_request(&request, root, &server->procedure_list);
	} else {
		request.id = NULL;
		err_msg = "The JSON sent is not a valid Request object.";
		send_error(&request, JRPC_INVALID_REQUEST, strdup(err_msg));
	}
	//shift processed request, discarding it
	memmove(conn->buffer, end_ptr, strlen(end_ptr) + 2);

	conn->pos = strlen(end_ptr);
	memset(conn->buffer + conn->pos, 0, conn->buffer_size - conn->pos - 1);
	if (root)
		cJSON_Delete(root);
}

static void accept_cb(int fd, jrpc_server_t *server) {
	jrpc_loop_t *jrpc_loop = malloc(sizeof(jrpc_loop));
	if (jrpc_loop <= 0) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	int limit_connection = get_limit_fd_number();
	char s[INET6_ADDRSTRLEN];
	jrpc_conn_t *conn;
	struct sockaddr_storage their_addr; // connector's address information

	conn = malloc(sizeof(jrpc_conn_t));
	socklen_t sin_size = sizeof their_addr;

	conn->fd = accept(fd, (struct sockaddr *) &their_addr, &sin_size);
	if (conn->fd == -1) {
		perror("accept");
		free(conn);
		return;
	}
	/* Limitation of select */
	if (limit_connection <= conn->fd) {
		close(conn->fd);
		free(conn);
		fprintf(stderr, "Reach max connection, limit %d.",
				limit_connection, s);
		return;
	}

	if (server->debug_level) {
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)
				&their_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);
	}
	//copy pointer to jrpc_server_t
	jrpc_loop->conn = conn;
	jrpc_loop->server = server;
	conn->buffer_size = 1500;
	conn->buffer = malloc(1500);
	memset(conn->buffer, 0, 1500);
	conn->pos = 0;
	conn->debug_level = server->debug_level;
	add_select_fds(&server->jrpc_select.fds_read, conn->fd, connection_cb,
			(void *)jrpc_loop);
}

int jrpc_server_init(jrpc_server_t *server, int port_number) {
	return jrpc_server_init_with_select_loop(server, port_number);
}

int jrpc_server_init_with_select_loop(jrpc_server_t *server, int port_number) {
	memset(server, 0, sizeof(jrpc_server_t));
	server->is_running = 0;
	server->port_number = port_number;
	char *debug_level_env = getenv("JRPC_DEBUG");
	if (debug_level_env == NULL)
		server->debug_level = 0;
	else {
		server->debug_level = strtol(debug_level_env, NULL, 10);
		printf("JSONRPC-C Debug level %d\n", server->debug_level);
	}
	return jrpc_server_start(server);
}

static int jrpc_server_start(jrpc_server_t *server) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int yes = 1;
	int rv;
	char PORT[6];
	sprintf(PORT, "%d", server->port_number);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	/* loop through all the results and bind to the first we can */
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(1);
	}
	if (server->debug_level)
		printf("server: waiting for connections...\n");

	add_select_fds(&server->jrpc_select.fds_read, sockfd, accept_cb,
			(void *)server);
	return 0;
}

void jrpc_server_run(jrpc_server_t *server) {
	server->is_running = 1;
	loop_select(&server->jrpc_select, server->debug_level, &server->is_running);
}

int jrpc_server_stop(jrpc_server_t *server) {
	server->is_running = 0;
	return 0;
}

void jrpc_server_destroy(jrpc_server_t *server) {
	jrpc_procedures_destroy(&server->procedure_list);
}
