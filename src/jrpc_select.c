/*
 * jrpc_select.c
 *
 * Created on: Feb 28, 2013
 *	Author: mathben
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "jrpc_select.h"

void add_select_fds(jrpc_select_fds_t *fds, int fd, void *cb, void *data) {
	int *fd_l;
	select_cb *cb_l;
	void **data_l;
	int i, last_size, mem_size;

	if (fds->nb >= fds->size) {
		last_size = fds->size;
		if (fds->size <= 0)
			fds->size = 1;
		fds->size *= 2;
		
		fd_l = calloc(sizeof(int), sizeof(int) * fds->size);
		cb_l = calloc(sizeof(select_cb), sizeof(select_cb) * fds->size);
		data_l = calloc(sizeof(void *), sizeof(void *) * fds->size);
		/* to be sure to have only NULL */
		memset(fd_l, 0, sizeof(int) * fds->size);
		memset(cb_l, 0, fds->size);
		memset(data_l, 0, fds->size);
		for (i = 0; i < last_size; i++) {
			fd_l[i] = fds->fd[i];
			cb_l[i] = fds->cb[i];
			data_l[i] = fds->data[i];
		}
		free(fds->fd);
		free(fds->cb);
		free(fds->data);
		fds->fd = fd_l;
		fds->cb = cb_l;
		fds->data = data_l;
	} else {
		fd_l = fds->fd;
		cb_l = fds->cb;
		data_l = fds->data;
	}

	/* search first fd == NULL */
	for (i = 0; i < fds->nb; i++) {
		if (!fd_l[i])
			break;
	}
	fd_l[i] = fd;
	cb_l[i] = (select_cb)cb;
	data_l[i] = data;
	fds->nb++;
}

int remove_select_fds(jrpc_select_fds_t *fds, int fd) {
	/* return 0 when success */
	int i;
	for (i = 0; i < fds->size; i++) {
		if (fds->fd[i] == fd) {
			fds->fd[i] = NULL;
			fds->cb[i] = NULL;
			free(fds->data[i]);
			fds->data[i] = NULL;
			fds->nb--;
			return 0;
		}
	}
	return -1;
}

void fill_fd_select(fd_set *fds, jrpc_select_fds_t *jrpc_fds, int *ndfs) {
	/* ndfs is the max number of fd */
	int ndfs_local = *ndfs;
	int fd, i;
	for (i = 0; i < jrpc_fds->size; i++) {
		fd = jrpc_fds->fd[i];
		if (!fd) /* empty fd */
			continue;

		if (fd >= FD_SETSIZE) {
			fprintf(stderr, "fd %d is upper then FD_SETSIZE\n", fd);
			continue;
		}
		if (ndfs_local <= fd)
			ndfs_local = fd + 1;
		FD_SET(fd, fds);
	}
	*ndfs = ndfs_local;
}

void cb_fd_select(fd_set *fds, jrpc_select_fds_t *jrpc_fds, int ndfs) {
	int i, fd;
	for (i = 0; i < jrpc_fds->size; i++) {
		fd = jrpc_fds->fd[i];
		if (!fd)
			continue;
		if (FD_ISSET(fd, fds)) {
			jrpc_fds->cb[i](fd, jrpc_fds->data[i]);
		}
	}
}

