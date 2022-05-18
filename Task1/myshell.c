#include "LineParser.h"
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


#define BUFFER_SIZE 2048


void display_propmt() {
    char path_name[PATH_MAX];
    getcwd(path_name, PATH_MAX);
    fprintf(stdout, "%s\n", path_name);
}

void print_debug(int pid, cmdLine *pCmdLine) {
    fprintf(stderr, "%s", "PID: ");
    fprintf(stderr, "%d\n", pid);
    fprintf(stderr, "%s", "Executing command: ");
    fprintf(stderr, "%s\n", pCmdLine->arguments[0]);
}


int exeCustomCommand(cmdLine *command) {
    int isCustom = 0;
    if (strcmp(command->arguments[0], "quit") == 0) {
        exit(EXIT_SUCCESS);
    }
    if (strcmp(command->arguments[0], "cd") == 0) {
        isCustom = 1;
        if (chdir(command->arguments[1]) < 0) {
            perror("Could not execute CD command");
        }
    }
    return isCustom;
}

void execute(cmdLine *pCmdLine, int debug) {
    if (!exeCustomCommand(pCmdLine)) {
        int cpid = fork();
        int return_code = 0;
        if (cpid == 0) {
            return_code = execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        }
        if (return_code < 0) {
            perror("Can't execute the command");
            _exit(EXIT_FAILURE);
        }
        if (debug) {
            print_debug(cpid, pCmdLine);
        }
        if(pCmdLine->blocking){
            waitpid(cpid,NULL,0); //
        }
    }
}


        int main(int argc, char const *argv[]) {

            FILE *input = stdin;
            char *buffer[2048];
            int debug = 0;
            int i;

            for (i = 1; i < argc; i++) {
                if (strncmp("-d", argv[i], 2) == 0)
                    debug = 1;
            }

            while (1) {
                display_propmt();
                fgets(buffer, BUFFER_SIZE, input);
                cmdLine *line = parseCmdLines(buffer);
                execute(line, debug);
                freeCmdLines(line);
            }
            return 0;
        }