/*
 * jrpc_select.h
 *
 * Created on: Feb 28, 2013
 *	Author: mathben
 */

#ifndef JRPC_SELECT_H_
#define JRPC_SELECT_H_

typedef void (*select_cb) (int, void *);
typedef struct {
	int *fd; /* table of fd */
	select_cb *cb; /* callback */
	void **data; /* table of data */
	int nb; /* number of fd */
	int size; /* size of table */
} jrpc_select_fds_t;

typedef struct {
	jrpc_select_fds_t fds_read;
	jrpc_select_fds_t fds_write;
	jrpc_select_fds_t fds_err;
	/* Quick fonctionality to cb when interrupt signal */
	void *cb_int_signal;
} jrpc_select_t;

void add_select_fds(jrpc_select_fds_t *fds, int fd, void *cb, void *data);
int remove_select_fds(jrpc_select_fds_t *fds, int fd);
void fill_fd_select(fd_set *fds, jrpc_select_fds_t *jrpc_fds, int *ndfs);
void cb_fd_select(fd_set *fds, jrpc_select_fds_t *jrpc_fds, int ndfs);

#endif
