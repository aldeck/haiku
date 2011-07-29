/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <unistd.h>

#include <errno.h>
#include <pthread.h>

#include <syscall_utils.h>

#include <syscalls.h>


int
fsync(int fd)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_fsync(fd));
}


int
sync(void)
{
	int status = _kern_sync();
	if (status < 0) {
		errno = status;
		status = -1;
	}

	return status;
}
