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
#include <string.h>
#include <fcntl.h>

#include "jrpc_select.h"

void fill_fd_select(fd_set *fds, jrpc_select_fds_t *jrpc_fds, int *ndfs);
void cb_fd_select(fd_set *fds, jrpc_select_fds_t *jrpc_fds, int ndfs);

void loop_select(jrpc_select_t *jrpc_select, int debug, int *is_running) {
	fd_set readfds, writefds, errfds;
	int ready, nfds, err;

	if (debug)
		printf("Loop select started.\n");

	while (*is_running) {
		err = 0;
		/* Initialize the file descriptor set. */
		nfds = 0;
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&errfds);

		fill_fd_select(&readfds, &jrpc_select->fds_read, &nfds);
		fill_fd_select(&writefds, &jrpc_select->fds_write, &nfds);
		fill_fd_select(&errfds, &jrpc_select->fds_err, &nfds);

		ready = 0;
		while (*is_running && !ready) {
			/* Call select() */
			ready = select(nfds, &readfds, &writefds, &errfds, NULL);
			if (ready == -1) {
				/* continue when receive interrupt
				 * the callback will manage the situation
				 */
				if (EINTR == errno)
					continue;
				perror("select");
				exit(EXIT_FAILURE);
			}
		}
		if (!*is_running)
			break;
		if (debug)
			printf("Select receive %d fd.\n", ready);
		if (ready > 0) {
			/* Callback time */
			cb_fd_select(&readfds, &jrpc_select->fds_read, nfds);
			cb_fd_select(&writefds, &jrpc_select->fds_write, nfds);
			cb_fd_select(&errfds, &jrpc_select->fds_err, nfds);
		}
	}
	if (debug)
		printf("Leave loop select.\n");
}

void add_select_fds(jrpc_select_fds_t *fds, int fd, void *cb, void *data,
		int free_data) {
	int i, last_size, mem_size;
	jrpc_select_fds_data_t *fds_data;

	if (fds->nb >= fds->size) {
		last_size = fds->size;
		if (fds->size <= 0)
			fds->size = 1;
		fds->size *= 2;
		
		fds_data = malloc(sizeof(jrpc_select_fds_data_t) * fds->size);
		if (!fds_data) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		/* to be sure to have only NULL */
		memset(fds_data, 0, sizeof(jrpc_select_fds_data_t) * fds->size);
		for (i = 0; i < last_size; i++) {
			fds_data[i].fd = fds->data[i].fd;
			fds_data[i].cb = fds->data[i].cb;
			fds_data[i].data = fds->data[i].data;
			fds_data[i].free_data = fds->data[i].free_data;
		}
		free(fds->data);
		fds->data = fds_data;
	} else {
		fds_data = fds->data;
	}

	/* search first fd == NULL */
	for (i = 0; i < fds->nb; i++) {
		if (!fds_data[i].fd)
			break;
	}
	fds_data[i].fd = fd;
	fds_data[i].cb = (select_cb)cb;
	fds_data[i].data = data;
	fds_data[i].free_data = free_data;
	fds->nb++;
}

int remove_select_fds(jrpc_select_fds_t *fds, int fd) {
	/* return 0 when success */
	int i;
	for (i = 0; i < fds->size; i++) {
		if (fds->data[i].fd == fd) {
			fds->data[i].fd = 0;
			fds->data[i].cb = NULL;
			if (fds->data[i].free_data && fds->data[i].data) {
				free(fds->data[i].data);
				fds->data[i].data = NULL;
			}
			fds->nb--;
			return 0;
		}
	}
	return -1;
}

int remove_select_all_fds() {
	/* TODO complete this and free all memory */
	return 1;
}

void fill_fd_select(fd_set *fds, jrpc_select_fds_t *jrpc_fds, int *ndfs) {
	/* ndfs is the max number of fd */
	int ndfs_local = *ndfs;
	int fd, i, rc;
	for (i = 0; i < jrpc_fds->size; i++) {
		fd = jrpc_fds->data[i].fd;
		if (!fd || !jrpc_fds->data[i].cb) /* empty fd or empty callback*/
			continue;
		if (fd >= FD_SETSIZE) {
			fprintf(stderr, "fd %d is upper then FD_SETSIZE\n", fd);
			continue;
		}
		/* check if fd is open */
		rc = fcntl(fd, F_GETFL);
		if (rc < 0)
			continue;
		/* set the fd */
		if (ndfs_local <= fd)
			ndfs_local = fd + 1;
		FD_SET(fd, fds);
	}
	*ndfs = ndfs_local;
}

void cb_fd_select(fd_set *fds, jrpc_select_fds_t *jrpc_fds, int ndfs) {
	int i, fd;
	for (i = 0; i < jrpc_fds->size; i++) {
		fd = jrpc_fds->data[i].fd;
		if (!fd || !jrpc_fds->data[i].cb)
			continue;
		if (FD_ISSET(fd, fds)) {
			jrpc_fds->data[i].cb(fd, jrpc_fds->data[i].data);
		}
	}
}

int get_limit_fd_number() {
	return (int)FD_SETSIZE;
}
