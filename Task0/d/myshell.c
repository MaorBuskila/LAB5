#include "LineParser.h"
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#define BUFFER_SIZE 2048


void display_propmt(){
    char path_name[PATH_MAX];
    getcwd(path_name,PATH_MAX);
    fprintf(stdout, "%s\n",path_name);
}
int CdCommand(cmdLine* pCmdLine, int debug){

    int cd = 0;
    if(strcmp(pCmdLine->arguments[0],"cd") == 0){
        cd = 1;
        int val = chdir(pCmdLine->arguments[1]);

        if(val < 0){
            perror("ERROR on cd command");

            if(debug)
                fprintf(stderr, "%s\n","ERROR on cd command");

        }

    }

    return cd;
}
void execute(cmdLine* pCmdLine, int debug){

    if(CdCommand(pCmdLine, debug) == 0){

        if(strcmp(pCmdLine->arguments[0],"quit")==0)
        exit(EXIT_SUCCESS);

    int pid = fork();
    int return_code = 0;

    if (pid == 0)
        return_code = execvp(pCmdLine->arguments[0],pCmdLine->arguments);

    if(debug){
        fprintf(stderr, "%s","PID: ");
        fprintf(stderr, "%d\n",pid);
        fprintf(stderr, "%s","Executing command: ");
        fprintf(stderr, "%s\n",pCmdLine->arguments[0]);

    }

    if(return_code<0) {
        perror("Can't execute the command");
        _exit(EXIT_FAILURE);

    }

    }
}

int main(int argc, char const *argv[]) {

    FILE* input = stdin;
    char* buffer[2048];
    int debug = 0;
    int i;

    for (i = 1; i < argc; i++){

        if (strncmp("-d", argv[i], 2) == 0)
            debug = 1;
    }

    while(1){
        display_propmt();
        fgets(buffer,BUFFER_SIZE,input);
        cmdLine* line = parseCmdLines(buffer);
        execute(line, debug);
        freeCmdLines(line);
    }
    return 0;
}