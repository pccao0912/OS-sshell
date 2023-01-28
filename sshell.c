#include <stdio.h>
#include <errno.h>
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
        int arg_nums;
        int pipe_nums;
};

int commandCheck(char *cmd) {
        // Check if there's no command before the > symbol
        int cmd_length = strlen(cmd);
        int valid_char = 0;
        for (int i = 0; i < cmd_length; i++) {
                if(valid_char == 0 && cmd[i] == '>') {
                        return 1;
                } else if (cmd[i] != '>' && cmd[i] != ' '){
                        valid_char++;
                }
        }
        return 0;
}

struct CMD parse(struct CMD command, char* cmd) {
        char *token;  // current string/token
        int index = 0;
        token = strtok(cmd, " ");
        command.arg_nums = 0;
        while (token != NULL) {
                // method to keep reading words from a string from https://www.runoob.com/cprogramming/c-function-strtok.html
                command.args[index] = token;
                token = strtok(NULL, " ");
                index++;
                command.arg_nums = index;
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


int redirection(char* cmd) {
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

        for (int j = start; j < cmd_length; j++) {
                if (cmd[j] == ' ') {
                        end = j - 1;
                        break;
                }
                end = j;
        }

        // creat a string according to the length(end - start)
        int dir_len = end - start + 1;
        char directory[dir_len];
        // copy the string content from cmd
        for (int k = 0 ; k < dir_len; k++) {
                directory[k] = cmd[k + start];
        }
        // end the string
        directory[dir_len] = '\0';

        if (dir_len == 1 && directory[0] == '>') {
                fprintf(stderr, "Error: no output file\n");
                return 1;
        }
        if (append_flag == 1) {
                fd = open(directory, O_CREAT | O_WRONLY | O_APPEND, 0600);
        } else {
                fd = open(directory, O_CREAT | O_WRONLY | O_TRUNC, 0600);
        }
        if (fd < 0) {
                fprintf(stderr, "Error: cannot open output file\n");
                return 1;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        return 0;
}

bool pipeline_check(struct CMD *CMD, char *cmd) {
        int Pip_flag = 0;
        int cmd_length = strlen(cmd);
        CMD->pipe_nums = 0;
        for (int i = 0; i <= cmd_length; i++) {
                if(cmd[i] == '|') {
                        Pip_flag = 1;
                        CMD->pipe_nums++;
                }
        }
        return Pip_flag;
}

bool background_check(char *cmd) {
        int bg_flag = 0;
        int cmd_length = strlen(cmd);
        for (int i = 0; i <= cmd_length; i++) {
                if(cmd[i] == '&') {
                        bg_flag = 1;
                        cmd[i] = ' ';
                }
        }
        return bg_flag;
}

int main(void)
{
        char cmd[CMDLINE_MAX];
        char pipeline_cmd[CMDLINE_MAX];
        char pipeline_cmd_arg2[CMDLINE_MAX];
        char Prev_cmd[CMDLINE_MAX];
        char bg_cmd[CMDLINE_MAX];
        struct CMD CMD;
        int bg_pid = 0;
        int bg_pro = 0;
        int bg_status_list[MAX_PIPE];
        int bg_pipe_count = 0;
        int bg_pipe_indicator = 0;
        int error_flag;
        // since parse would change the cmd, so creating a new string to store the origin version
        while (1) {
                
                char *nl;

                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);
                if(!(strcmp(cmd,"\n"))) {
                        continue;
                }

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
                int command_flag = commandCheck(cmd);
                if (command_flag == 1) {
                        fprintf(stderr, "Error: missing command\n");
                        continue;
                }
                if (strstr(cmd, ">") < strstr(cmd, "|") && strstr(cmd, ">") != NULL) {
                        fprintf(stderr, "Error: mislocated output redirection\n");
                        continue;
                }
                int redirection_flag = redirection_check(cmd);
                int pipline_flag = pipeline_check(&CMD, cmd);
                int bg_flag = background_check(cmd);
                if(pipline_flag == 1) {
                        // pipeline instruction
                        char *token = strtok(cmd, "|");
                        char *cmds[MAX_PIPE];
                        int count = 0;
                        while (token != NULL) {
                                
                                cmds[count] = token;
                                token = strtok(NULL, "|");
                                count++ ;
                        }
                        if (CMD.pipe_nums >= count) {
                                fprintf(stderr, "Error: missing command\n");
                                continue;;
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
                                        arg_count++;
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
                                        if (redirection_flag != 0 && j == count -1) {
                                                char cmd_for_redirect[CMDLINE_MAX];
                                                int error_flag;
                                                strcpy(cmd_for_redirect,pipeline_cmd_arg2);
                                                printf("%s\n",cmd_for_redirect);
                                                char *token_redirect = strtok(cmd_for_redirect, "|");
                                                char *cmds_redirect[MAX_PIPE];
                                                int count_redirect = 0;
                                                while (token_redirect != NULL) {
                                                        
                                                        cmds_redirect[count_redirect] = token_redirect;
                                                        token_redirect = strtok(NULL, "|");
                                                        count_redirect++ ;
                                                }
                                                error_flag = redirection(cmds_redirect[count_redirect-1]);
                                                if (error_flag != 0) {
                                                        exit(1);
                                                }
                                        }
                                        execvp(args[0],args);
                                } else if (pid > 0) {
                                        //parent
                                if (j > 0) {
                                        close(pipes[j-1][0]);
                                        close(pipes[j-1][1]);
                                }
                                if(j == count - 1) {
                                        if (bg_pro > 0){
                                                bg_pid = pid;
                                                printf("ruuning in the background %d\n",bg_pid);
                                                
                                                strcpy(bg_cmd,pipeline_cmd_arg2);
                                                
                                        } else {
                                                pid = waitpid(pid, &status, 0);
                                                if (bg_pro > 0){
                                                        int status;
                                                        int bg_result = waitpid(bg_pid,&status, WNOHANG);
                                                        if(bg_result > 0){
                                                                if (bg_pipe_indicator != 1){
                                                                        fprintf(stderr, "+ completed '%s' [%d]\n",
                                                                        bg_cmd, WEXITSTATUS(status));
                                                                } else {
                                                                        fprintf(stderr, "+ completed '%s' ",bg_cmd);
                                                                        for (int h = 0; h < bg_pipe_count; h++) {
                                                                                fprintf(stderr, "[%d]", WEXITSTATUS(bg_status_list[h]));
                                                                        }
                                                                        fprintf(stderr,"[%d]",WEXITSTATUS(status));
                                                                        fprintf(stderr, "\n");
                                                                }
                                                                bg_pro = 0 ;
                                                                bg_pid = 0 ;
                                                                bg_pipe_indicator = 0;
                                                        }
                                                }
                                                fprintf(stderr, "+ completed '%s' ", pipeline_cmd_arg2);
                                                for (int h = 0; h < count-1; h++) {
                                                        fprintf(stderr, "[%d]", WEXITSTATUS(status_list[h]));
                                                }
                                                fprintf(stderr,"[%d]",WEXITSTATUS(status));
                                                fprintf(stderr, "\n");
                                        }
                                        
                                } else {
                                        if(bg_flag == 1 ||bg_pro > 0){
                                                bg_pro += 1;
                                                bg_pipe_count += 1;
                                                bg_status_list[j] = WEXITSTATUS(status);
                                                bg_pipe_indicator = 1;
                                        }
                                        status_list[j] = WEXITSTATUS(status);
                                }
                            }
                        }    
                } else  {
                        // execution without piping
                        struct CMD CMD = parse(CMD, cmd);
                        if (CMD.arg_nums >= ARGS_MAX) {
                                fprintf(stderr, "Error: too many process arguments\n");
                                continue;
                        }
                        /* Builtin command */
                        if (!strcmp(CMD.args[0], "exit")) {
                                fprintf(stderr, "Bye...\n");
                                fprintf(stderr, "+ completed '%s' [%d]\n",
                                        Prev_cmd, 0);
                                exit(0);
                        } else if (!strcmp(CMD.args[0], "cd")) {
                                int ret = chdir(CMD.args[1]);
                                if (ret != 0) {
                                        fprintf(stderr, "Error: cannot cd into directory\n");
                                        fprintf(stderr, "+ completed '%s' [%d]\n",
                                        Prev_cmd, 1);
                                        continue;
                                }
                                fprintf(stderr, "+ completed '%s' [%d]\n",
                                Prev_cmd, WEXITSTATUS(ret));
                        } else if (!strcmp(CMD.args[0], "pwd")) {
                                char* cur_path = NULL;
                                cur_path = getcwd(cur_path, 0);
                                fprintf(stdout, "%s\n", cur_path);
                                fprintf(stderr, "+ completed '%s' [%d]\n",
                                Prev_cmd, 0);
                        } else {
                                /* Regular command */
                                pid_t pid = fork();
                                int status;
                                if (pid == 0) {
                                        if (redirection_flag != 0) {
                                                error_flag = redirection(Prev_cmd);
                                                if (error_flag != 0) {
                                                        continue;
                                                }
                                        }
                                        execvp(CMD.args[0],CMD.args);
                                        if (errno = ENOENT) {
                                                fprintf(stderr, "Error: command not found\n");
                                                fprintf(stderr, "+ completed '%s' [%d]\n",
                                                Prev_cmd, 1);
                                                continue;
                                        }
                                        exit(1);
                                }
                                if (pid > 0) {
                                        if (bg_flag == 1){
                                                bg_pid = pid;
                                                bg_pro += 1;
                                                strcpy(bg_cmd,Prev_cmd);
                                        } else {
                                                pid = waitpid(pid, &status, 0);
                                                if (bg_pro > 0){
                                                        int status;
                                                        int bg_result = waitpid(bg_pid,&status, WNOHANG);
                                                        if(bg_result > 0){
                                                                if (bg_pipe_indicator != 1){
                                                                        fprintf(stderr, "+ completed '%s' [%d]\n",
                                                                        bg_cmd, WEXITSTATUS(status));
                                                                } else {
                                                                        fprintf(stderr, "+ completed '%s' ",bg_cmd);
                                                                        for (int h = 0; h < bg_pipe_count; h++) {
                                                                                fprintf(stderr, "[%d]", WEXITSTATUS(bg_status_list[h]));
                                                                        }
                                                                        fprintf(stderr,"[%d]",WEXITSTATUS(status));
                                                                        fprintf(stderr, "\n");
                                                                }
                                                                bg_pro = 0 ;
                                                                bg_pid = 0 ;
                                                                bg_pipe_indicator = 0;
                                                        }
                                                }
                                                fprintf(stderr, "+ completed '%s' [%d]\n",
                                                Prev_cmd, WEXITSTATUS(status));
                                        } 

                                }
                        }
                }
        }
        return EXIT_SUCCESS;
}
