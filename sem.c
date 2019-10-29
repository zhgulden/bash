#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <err.h>
#include <sys/stat.h>
#include <fcntl.h>

char *get_word(char *end){
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

char **free_list(char **list){
    if(list == NULL) {
        return list;
    }
    for (int i = 0; list[i] != NULL; i++){
        free(list[i]);
    }
    free(list);
    return list;
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

int make_exec(char **arr) {
    if (arr != NULL) {
        char *cmd = arr[0];
        if (arr != NULL && ((strcmp(cmd, "exit") == 0) ||
                            (strcmp(cmd, "quit") == 0))) {
            free_list(arr);
            return 1;
        }
        if (fork() > 0) {
            wait(NULL);
        } else {
            if (execvp(cmd, arr) < 0) {
                perror("Exec failed:");
                return 1;
            }
        }
    }
    return 0;
}



int main(int argc, char **argv) {
    char **arr = NULL;
    while (1) {
        printf(">>>>>>>");
        arr = get_list();
        if (arr != NULL) {
            if (make_exec(arr) == 1) {
                return 1;
            }
        }
    }
    return 0;
}
