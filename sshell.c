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
#define MAX_PIPE 4

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

int redirection_check(char* cmd) {
        int length = strlen(cmd);
        int flag = 0;
        for (int i = 0; i < length; i++) {
                if (cmd[i] == '>') {
                        if (cmd[i + 1] == '>') {
                                flag = 2;
                        } else {
                                flag = 1;
                        }
                }
                if (flag == 1) {
                        cmd[i] = ' ';
                } else if (flag == 2) {
                        cmd[i] = ' ';
                        cmd[i + 1] = cmd[i];
                }
        }
        return flag;
}


void redirection(char* cmd) {
        int cmd_length = strlen(cmd);
        int start = 0;
        int end = 0;
        int fd;
        int append_flag = 0;
        // use two pointers start and end to track the text that we want to use
        for (int i = 0; i < cmd_length; i++) {
                if (start == 0 && cmd[i] == '>' && cmd[i+1] == '>') {
                        append_flag = 1;
                        start = i + 1;
                        i++;
                } else if (start == 0 && cmd[i] == '>' && cmd[i+1] != '>') {
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
        // printf("Append flag is: %d\n", append_flag);
        if (append_flag == 1) {
                fd = open(directory, O_WRONLY | O_APPEND);
        } else {
                fd = open(directory, O_CREAT | O_WRONLY | O_TRUNC);
        }
        if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
}

bool pipeline_check(char *cmd) {
        int Pip_flag = 0;
        int cmd_length = strlen(cmd);
        for (int i = 0; i <= cmd_length; i++) {
                if(cmd[i] == '|') {
                        Pip_flag = 1;
                }
        }
        return Pip_flag;
}

void pipeline(char *cmd, char* cmd_duplicate, int redirection_flag) {
        // char *redirectionfile_cmd[MAX_PIPE];
        // char cmd_duplicate_output[CMDLINE_MAX];
        // strcpy(cmd_duplicate_output,cmd_duplicate);
        char *token = strtok(cmd, "|");
        // char *token_redirection = strtok(cmd_duplicate_output,"|");
        char *cmds[MAX_PIPE];
        int count = 0;
        while (token != NULL) {
                
                cmds[count] = token;
                token = strtok(NULL, "|");
                count++ ;
        }
        int pipes[count][2];
        int status_list[count+1];
        for (int j = 0; j < count; j++) {
                pipe(pipes[j]);
        }

        for (int j = 0; j < count; j++) {
                char *args[ARGS_MAX];
                int arg_count = 0;
                char *separate_arg = strtok(cmds[j]," ");
                while (separate_arg != NULL) {
                        args[arg_count] = separate_arg;
                        //printf("%s\n",args[arg_count]);
                        arg_count ++ ;
                        separate_arg = strtok(NULL, " ");
                }
                args[arg_count] = NULL;
                int status;
                pid_t pid = fork();
                if (pid == 0) {
                        //children
                        if (j > 0) {
                                close(pipes[j-1][1]);
                                dup2(pipes[j-1][0], STDIN_FILENO);
                                close(pipes[j-1][0]);
                        }
                        if (j < count - 1) {
                                close(pipes[j][0]);
                                dup2(pipes[j][1], STDOUT_FILENO);
                                close(pipes[j][1]);
                        }
                        if (redirection_flag == 1 && j == count -1) {
                                char cmd_for_redirect[CMDLINE_MAX];
                                strcpy(cmd_for_redirect,cmd_duplicate);
                                printf("%s\n",cmd_for_redirect);
                                char *token_redirect = strtok(cmd_for_redirect, "|");
                                char *cmds_redirect[MAX_PIPE];
                                int count_redirect = 0;
                                while (token_redirect != NULL) {
                                        
                                        cmds_redirect[count_redirect] = token_redirect;
                                        token_redirect = strtok(NULL, "|");
                                        count_redirect++ ;
                                }
                                redirection(cmds_redirect[count_redirect-1]);
                        }
                        execvp(args[0],args);
                } else if (pid > 0) {
                        //parent
                        if (j > 0) {
                                close(pipes[j-1][0]);
                                close(pipes[j-1][1]);
                        }
                        if(j == count - 1) {

                                // printf("redirection flag is: %d\n", redirection_flag);
                                // printf("current cmd is: %s\n", cmd_duplicate);
                                pid = waitpid(pid, &status, 0);
                                fprintf(stderr, "+ completed '%s' ", cmd_duplicate);
                                for (int h = 0; h < count-1; h++) {
                                         fprintf(stderr, "[%d]", WEXITSTATUS(status_list[h]));
                                }
                                fprintf(stderr,"[%d]",WEXITSTATUS(status));
                                fprintf(stderr, "\n");
                        } else {
                                status_list[j] = WEXITSTATUS(status);
                        }
                        // pid = wait(&status);
                }

        }    
}

void execution(char Prev_cmd[], char cmd[], int redirection_flag) {
        struct CMD CMD = parse(CMD, cmd);
        /* Builtin command */
        if (!strcmp(CMD.args[0], "exit")) {
                fprintf(stderr, "Bye...\n");
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        Prev_cmd, 0);
                return;
        } else if (!strcmp(CMD.args[0], "cd")) {
                int ret = chdir(CMD.args[1]);
                if (ret != 0) {
                        fprintf(stderr, "Error: cannot cd into directory");
                        fprintf(stderr, "+ completed '%s' [%d]\n",
                        Prev_cmd, WEXITSTATUS(ret));
                        return;
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
                        if (redirection_flag != 0) {
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

int main(void)
{
        char cmd[CMDLINE_MAX];
        char pipeline_cmd[CMDLINE_MAX];
        char pipeline_cmd_arg2[CMDLINE_MAX];
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
                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';
                strcpy(Prev_cmd, cmd);
                strcpy(pipeline_cmd, cmd);
                strcpy(pipeline_cmd_arg2, cmd);   
                int redirection_flag = redirection_check(cmd);
                int pipline_flag = pipeline_check(cmd);
                if(pipline_flag == 1) {
                        // pipeline instruction
                        pipeline(cmd, pipeline_cmd_arg2, redirection_flag);
                } else  {
                        // execution without piping
                        execution(Prev_cmd, cmd, redirection_flag);
                }
        }
        return EXIT_SUCCESS;
}
