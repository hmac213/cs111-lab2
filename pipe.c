#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if (argc < 2) {
		errno = EINVAL;
		exit(EINVAL);
	}

	int prev_read_fd = -1;

	for (int i = 1; i < argc; i++) {
		int is_last = (i == argc - 1);
		int newpipe[2];

		if (!is_last) {
			if (pipe(newpipe) == -1) {
				if (prev_read_fd >= 0)
					close(prev_read_fd);
				exit(errno);
			}
		}

		pid_t pid = fork();
		// fork failed
		if (pid < 0) {
			if (!is_last) {
				close(newpipe[0]);
				close(newpipe[1]);
			}
			if (prev_read_fd >= 0)
				close(prev_read_fd);
			exit(errno);
		}

		// fork succeeded; in child process
		if (pid == 0) {
			if (prev_read_fd >= 0) {
				if (dup2(prev_read_fd, STDIN_FILENO) == -1)
					_exit(errno);
				close(prev_read_fd);
			}
			if (!is_last) {
				if (dup2(newpipe[1], STDOUT_FILENO) == -1) {
					close(newpipe[0]);
					close(newpipe[1]);
					_exit(errno);
				}
				close(newpipe[0]);
				close(newpipe[1]);
			}
			execlp(argv[i], argv[i], (char *)NULL);
			errno = EINVAL;
			_exit(EINVAL);
		}

		if (prev_read_fd >= 0)
			close(prev_read_fd);
		if (!is_last) {
			close(newpipe[1]);
			prev_read_fd = newpipe[0];
		}
	}

	for (int i = 1; i < argc; i++) {
		if (wait(NULL) == -1)
			exit(errno);
	}

	return 0;
}
