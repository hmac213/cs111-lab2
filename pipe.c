#include <errno.h>
#include <stdio.h>
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
	int ncmds = argc - 1;
	pid_t pids[argc - 1];

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
			perror(argv[i]);
			_exit(EINVAL);
		}

		if (prev_read_fd >= 0)
			close(prev_read_fd);
		if (!is_last) {
			close(newpipe[1]);
			prev_read_fd = newpipe[0];
		}

		pids[i - 1] = pid;
	}

	int exit_code = 0;

	for (int j = 0; j < ncmds; j++) {
		int status;

		if (waitpid(pids[j], &status, 0) == -1)
			exit(errno);
		if (j == ncmds - 1) {
			if (WIFEXITED(status))
				exit_code = WEXITSTATUS(status);
			else if (WIFSIGNALED(status))
				exit_code = 128 + WTERMSIG(status);
		}
	}

	return exit_code;
}
