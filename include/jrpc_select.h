/*
 * jrpc_select.h
 *
 * Created on: Feb 28, 2013
 *	Author: mathben
 */

#ifndef JRPC_SELECT_H_
#define JRPC_SELECT_H_
#include <sys/select.h>

typedef void (*select_cb) (int, void *);
typedef void (*destructor_cb) (void *);
typedef struct {
	int fd; /* table of fd */
	select_cb cb; /* callback */
	void *data; /* table of data */
	int free_data; /* apply free when delete fds on data */
	/* function to call when not null when delete fds */
	destructor_cb destructor;
} jrpc_select_fds_data_t;

typedef struct {
	jrpc_select_fds_data_t *data; /* tbl of data */
	int nb; /* number of fd */
	int size; /* size of table */
} jrpc_select_fds_t;

typedef struct {
	jrpc_select_fds_t fds_read;
	jrpc_select_fds_t fds_write;
	jrpc_select_fds_t fds_err;
} jrpc_select_t;

void loop_select(jrpc_select_t *jrpc_select, int debug, int *is_running);
void add_select_fds(jrpc_select_fds_t *fds, int fd, void *cb, void *data,
		int free_data, void *destructor);
/* cleanup_data set 1 to free data, else set 0 to do nothing. */
int remove_select_fds(jrpc_select_fds_t *fds, int fd);
int get_limit_fd_number();
void destroy_jrpc_select_fds(jrpc_select_t *jrpc_select);


#endif
