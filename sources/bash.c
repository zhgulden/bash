#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <err.h>

pid_t pid = 1;

char *get_word(char *end);
char **get_list();
int is_exit(char **list);
void free_list(char **list);
void run(char **list);
void make_exec(char **list, int fd_input, int fd_output, int pipe_pos,
               int and_pos, int background);
void prep_for_conv(char **list, char ***cmd_A, char ***cmd_B, int pipe_pos);
void pipe_for_two(char **list, char **cmd_A, char **cmd_B, int fd_input, int fd_output);

char *get_word(char *end) {
    char ch, delimiter = ' ', finish = '\n', tabulation = '\t';
    int pos = 0;
    char *word = NULL;
    if (*end == finish) {
        return NULL;
    }
    do {
        ch = getchar();
        if (pos == 0 && ch == finish) {
            return NULL;
        }
        while (pos == 0 && (ch == delimiter || ch == tabulation )) {
            ch = getchar();
            if (ch == finish) {
                return NULL;
             }
        }
        word = (char *)realloc(word, (pos + 1) * sizeof(char));
        if (word == NULL) {
            err(1, NULL);
        }
        word[pos] = ch;
        pos++;
    } while (ch != delimiter && ch != tabulation && ch != finish);
    word[pos - 1] = '\0';
    *end = ch;
    return word;
}

char **get_list(void)
{
	char end = 0, **list = NULL, **ch = NULL;
	int i = 0;
	do {
		ch = (char **)realloc(list, (i + 1) * sizeof(char *));
		if (ch == NULL) {
			err(1, NULL);
		}
		list = ch;
		list[i] = get_word(&end);
		i++;
	} while (list[i - 1] != NULL);
	return list;
}

int is_exit(char **list) {
	if ((strcmp(list[0], "exit") == 0) || (strcmp(list[0], "quit") == 0)) {
		return 1;
	} else {
		return 0;
	}
}

void free_list(char **list) {
	int i = 0;
	while (list[i] != NULL) {
		free(list[i]);
		i++;
	}
	free(list);
}

void run(char **list) {
	int i = 0, fd_input = 0, fd_output = 1,
            and_pos = 0, pipe_pos = 0, background = 0;
	while (list[i] != NULL) {
		if (strcmp(list[i], "<") == 0) {
			fd_input = open(list[i + 1], O_RDONLY);
			if (fd_input < 0) {
				perror("Open failed");
				exit(1);
			}
			break;
		}
		else if (strcmp(list[i], ">") == 0) {
			fd_output = open(list[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
			if (fd_output < 0) {
				perror("Open failed");
				exit(1);
			}
			break;
		}
		else if (strcmp(list[i], "|") == 0) {
			pipe_pos = i;
		} else if (strcmp(list[i], "&") == 0) {
			background = 1;
			return;;
		} else if (strcmp(list[i], "&&") == 0) {
			and_pos = i;
			break;
		}
		i++;
	}
	list = realloc(list, (i + 1) * sizeof(char*));
	list[i] = NULL;
	make_exec(list, fd_input, fd_output, pipe_pos, and_pos, background);
        return;
}

void exec_and(char **list, int fd_input, int fd_output) {
	char **cmd_cur;
	int i = 0, length = 0, beg_of_cmd = 0, j;
	while (1) {
		if (list[i] == NULL || strcmp(list[i], "&&") == 0) {
			length = i - beg_of_cmd + 1;
			cmd_cur = (char **)malloc((length) * sizeof(char*));
			for (j = 0; j < length - 1; j++, beg_of_cmd++) {
				cmd_cur[j] = list[beg_of_cmd];
			}
			cmd_cur[j] = NULL;
			int wstatus;
			if ((pid = fork()) > 0) {
				wait(&wstatus);
			} else {
				if (fd_output != 1) {
					dup2(fd_output, 1);
				}
				if (fd_input != 0) {
					dup2(fd_input, 0);
				}
				if (execvp(cmd_cur[0], cmd_cur) < 0) {
					perror("exec failed");
					_exit(1);
				}
			}
			fprintf(stderr, "%d\n", WEXITSTATUS(wstatus));
			free(cmd_cur);
			if (list[i] == NULL) {
				break;
			}
			beg_of_cmd = i + 1;
		}
		i++;
	}
}

void make_exec(char **list, int fd_input, int fd_output, int pipe_pos,
               int and_pos, int background) {
	if (pipe_pos != 0) {
		char **cmd_A, **cmd_B;
		prep_for_conv(list, &cmd_A, &cmd_B, pipe_pos);
		pipe_for_two(list, cmd_A, cmd_B, fd_input, fd_output);
	} else if (and_pos != 0) {
		exec_and(list, fd_input, fd_output);
	} else {
		if (fork() > 0) {
			if (background == 0) {
				wait(NULL);
			}
		} else {
			if (fd_output != 1) {
				dup2(fd_output, 1);
			}
			if (fd_input != 0) {
				dup2(fd_input, 0);
			}
			if (execvp(list[0], list) < 0) {
				perror("exec failed");
				_exit(1);
			}
		}
	}
	if (fd_output != 1) {
		close(fd_output);
	}
	if (fd_input != 0) {
		close(fd_input);
	}
}

void prep_for_conv(char **list, char ***cmd_A, char ***cmd_B, int pipe_pos) {
	int i, k, j;
	(*cmd_A) = (char **)malloc((pipe_pos + 1) * sizeof(char*));
	for (i = 0; i < pipe_pos; i++) {
		(*cmd_A)[i] = list[i];
	}
	(*cmd_A)[i] = NULL;
	i = pipe_pos + 1;
	while (list[i] != NULL) {
		i++;
	}
	(*cmd_B) = (char **)malloc((i - pipe_pos) * sizeof(char*));
	for (k = 0, j = pipe_pos + 1; j < i; k++, j++) {
		(*cmd_B)[k] = list[j];
	}
	(*cmd_B)[k] = NULL;
}

void pipe_for_two(char **list, char **cmd_A, char **cmd_B, int fd_input, int fd_output) {
	int fd[2];
	pipe(fd);
	if (fork() == 0) {
		if (fd_input != 0) {
			dup2(fd_input, 0);
		}
		close(fd[0]);
		dup2(fd[1], 1);
		execvp(cmd_A[0], cmd_A);
		_exit(1);
	}
	if (fork() == 0) {
		if (fd_output != 1) {
			dup2(fd_output, 1);
		}
		close(fd[1]);
		dup2(fd[0], 0);
		execvp(cmd_B[0], cmd_B);
		_exit(1);
	}
	close(fd[0]);
	close(fd[1]);
	wait(NULL);
	wait(NULL);
	return;
}

void handler(int signo) {
	puts("\nRecieved signal SIGINT");
	if (pid != 1) {
		kill(pid, SIGINT);
	}
}

int main(void) {
	char **list;
        signal(SIGINT, handler);
	while(1) {
		printf(">>>>>>>");
		list = get_list();
		if (is_exit(list) == 1) {
			free_list(list);
			break;
		}
		run(list);
		free_list(list);
	}
	return 0;
}

