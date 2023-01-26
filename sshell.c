#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 16
#define TOKEN_MAX 32

struct CMD {
        char *args[ARGS_MAX];
};

struct CMD parse(struct CMD command, char* cmd) {
        char *token;  // current string/token
        int index = 0;
        token = strtok(cmd, " ");
        while (token != NULL) {
                // method to keep reading words from a string from https://www.runoob.com/cprogramming/c-function-strtok.html
                command.args[index] = token;
                token = strtok(NULL, " ");
                index++;
        }
        command.args[index] = NULL;
        return command;
}

bool redirection_check(char* cmd) {
        int length = strlen(cmd);
        int flag = 0;
        for (int i = 0; i < length; i++) {
                if (cmd[i] == '>') {
                        flag = 1;
                }
                if (flag == 1) {
                        cmd[i] = ' ';
                }
        }
        return flag;
}

void redirection(char* cmd) {
        int cmd_length = strlen(cmd);
        int start = 0;
        int end = 0;
        int fd;
        // use two pointers start and end to track the text that we want to use
        for (int i = 0; i < cmd_length; i++) {
                if (start == 0 && cmd[i] == '>') {
                        start = i;
                } else if (start != 0 && cmd[i] != ' ') {
                        start = i;
                        break;
                }
        }

        // printf("start is: %d\n", start);

        for (int j = start; j < cmd_length; j++) {
                if (cmd[j] == ' ') {
                        end = j - 1;
                        break;
                }
                end = j;
        }

        // printf("end is: %d\n", end);

        // creat a string according to the length(end - start)
        int dir_len = end - start + 1;
        char directory[dir_len];
        // copy the string content from cmd
        for (int k = 0 ; k < dir_len; k++) {
                directory[k] = cmd[k + start];
        }
        // end the string
        directory[dir_len] = '\0';
        // printf("text is: %s\n", directory);
        // printf("text length is: %d\n", dir_len);

        // find the path or place that we want to use for fd
        // printf("Directory is %s\n", directory);
        fd = open(directory, O_CREAT | O_WRONLY | O_TRUNC);
        if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
}

int main(void)
{
        char cmd[CMDLINE_MAX];
        char Prev_cmd[CMDLINE_MAX];  
        // since parse would change the cmd, so creating a new string to store the origin version
        while (1) {
                char *nl;
                //int retval;

                /* Print prompt */
                printf("sshell$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);
                strcpy(Prev_cmd, cmd);
                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';
                nl = strchr(Prev_cmd, '\n');
                if (nl)
                        *nl = '\0';

                int redirection_flag = redirection_check(cmd);
                // printf("redirection_flag: %d\n", redirection_flag);
                struct CMD CMD = parse(CMD, cmd);
                /* Builtin command */
                if (!strcmp(CMD.args[0], "exit")) {
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr, "+ completed '%s' [%d]\n",
                                Prev_cmd, 0);
                        break;
                } else if (!strcmp(CMD.args[0], "cd")) {
                        int ret = chdir(CMD.args[1]);
                        if (ret != 0) {
                                fprintf(stderr, "Error: cannot cd into directory");
                                fprintf(stderr, "+ completed '%s' [%d]\n",
                                Prev_cmd, WEXITSTATUS(ret));
                                break;
                        }
                        fprintf(stderr, "+ completed '%s' [%d]\n",
                        Prev_cmd, WEXITSTATUS(ret));
                } else if (!strcmp(CMD.args[0], "pwd")) {
                        char* cur_path = NULL;
                        cur_path = getcwd(cur_path, 0);
                        fprintf(stderr, "%s\n", cur_path);
                        fprintf(stderr, "+ completed '%s' [%d]\n",
                        Prev_cmd, 0);
                } else {

                        /* Regular command */
                        // retval = system(cmd);
                        // fprintf(stdout, "Return status value for '%s': %d\n",
                        //         cmd, retval);
                        pid_t pid = 0;
                        int status;
                        //char *args[] = {cmd,"-u",NULL};
                        pid = fork();
                        if (pid == 0) {
                                if (redirection_flag == 1) {
                                        redirection(Prev_cmd);
                                }
                                execvp(CMD.args[0],CMD.args);
                                perror("execvp");
                                exit(1);
                        }
                        if (pid > 0) {
                                pid = wait(&status);
                                fprintf(stdout, "+ completed '%s' [%d]\n",
                                Prev_cmd, WEXITSTATUS(status));
                        }
                }
        }
        return EXIT_SUCCESS;
}
