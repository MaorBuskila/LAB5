#include "LineParser.h"
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


#define BUFFER_SIZE 2048

#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0

typedef struct process {
    cmdLine *cmd; /* the parsed command line*/
    pid_t pid; /* the process id that is running the command*/
    int status; /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next; /* next process in chain */
} process;


/*Fucntion Declaretion*/
void display_propmt();

void print_debug(pid_t, cmdLine *);

process *list_append(process *, cmdLine *, pid_t);

int exeCustomCommand(cmdLine *);

void execute(cmdLine *pCmdLine, int debug);

void list_print(process *);

void printProcess(process *process);

void addProcess(process **, cmdLine *, pid_t);

void printProcessList(process **);


void updateProcessList(process **);

void updateProcessStatus(process *, int, int);

int deleteTerminatedProcesses(process **process_list);

int delete_single_process(process *);


process *global_process_list;
process *global_process_list_begin = NULL;


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

/* Add a new link with the given data to the end of the list */
process *list_append(process *process_list, cmdLine *cmd, pid_t pid) {

    if (process_list == NULL) {
        process *new_process = malloc(sizeof(process));
        new_process->cmd = cmd;
        new_process->pid = pid;
        new_process->status = RUNNING;
        new_process->next = NULL;
        return new_process;
    } else
        process_list->next = list_append(process_list->next, cmd, pid);
    return process_list;
}

/*
void *list_append_begin(process* process_list, cmdLine* cmd, pid_t pid){
        process *new_process = malloc(sizeof(process));
        new_process->cmd = cmd;
        new_process->pid = pid;
        new_process->status = RUNNING;
        new_process->next = process_list;

        *process_list = *new_process;
}
*/

void updateProcessStatus(process *process_list, int pid, int status) {
    int new_status = RUNNING;

    if (WIFEXITED(status) || WIFSIGNALED(status))
        new_status = TERMINATED;

    else if (WIFSTOPPED(status))
        new_status = SUSPENDED;

    else if (WIFCONTINUED(status))
        new_status = RUNNING;

    process_list->status = new_status;

}

/*iterate all over the process list and update each process*/
void updateProcessList(process **process_list) {
    process *curr_process = *process_list;
    while (curr_process != NULL) {
        int status;
        int w_status=RUNNING;
        pid_t pid = waitpid(curr_process->pid, &status, WNOHANG); //check if process is done

        if (pid != 0) {       /*pid argument has changed state*/
            updateProcessStatus(curr_process, curr_process->pid, status);
        }
            curr_process=curr_process->next;
    }
}

char *getStatusString(int status) {

    if (status == TERMINATED)
        return "Terminated";

    else if (status == RUNNING)
        return "Running";

    else
        return "Suspended";
}

void printProcess(process *process) {

    char command[100] = "";
    int i;

    if (process->cmd->argCount > 0)
        for (i = 0; i < process->cmd->argCount; i++) {
            strcat(command, process->cmd->arguments[i]);
            strcat(command, " ");
        }
    printf("%d\t\t%s\t%s\n", process->pid, command, getStatusString(process->status));

}

void list_print(process *process_list) {
    process *curr_process = process_list;
    while (curr_process != NULL) {
        printProcess(curr_process);
        curr_process = curr_process->next;
    }
}

void delete_process(process *toDelete) {

    freeCmdLines(toDelete->cmd);
    toDelete->cmd = NULL;
    toDelete->next = NULL;
    free(toDelete);
    toDelete = NULL;
}
/* free all memory allocated for the process list , iterate on the linked list with recursion*/
void freeProcessList(process* process_list){
    process* curr_process=process_list;
    if(curr_process != NULL) {
        freeProcessList(curr_process->next);
        freeCmdLines(curr_process->cmd);
        free(curr_process->cmd);
        free(curr_process);
    }
    return;
}

int deleteTerminatedProcesses(process **process_list) {

    process *curr_process = *process_list;
    process *prev_process;

    /*deleting the head*/
    if (curr_process != NULL && curr_process->status == TERMINATED) {
        *process_list = curr_process->next;
        delete_process(curr_process);
        return 1;
    }

    /*iterate to the next terminated process*/
    while (curr_process != NULL && curr_process->status != TERMINATED) {
        prev_process = curr_process;
        curr_process = curr_process->next;
    }

    /*none terminated found*/
    if (curr_process == NULL)
        return 0;

    else {
        prev_process->next = curr_process->next;
        delete_process(curr_process);
        return 1;
    }
}

void printProcessList(process **process_list) {
    updateProcessList(process_list);
    printf("PID\t\tCommand\t\tSTATUS\n");
    list_print(*process_list);
    while (deleteTerminatedProcesses(process_list)) {};

}

void addProcess(process **process_list, cmdLine *cmd, pid_t pid) {
    *process_list = list_append(*process_list, cmd, pid);

}


int exeCustomCommand(cmdLine *command) {
    int isCustom = 0;
    if (strcmp(command->arguments[0], "quit") == 0) {
        exit(EXIT_SUCCESS);
    } else if (strcmp(command->arguments[0], "cd") == 0) {
        isCustom = 1;
        if (chdir(command->arguments[1]) < 0) {
            perror("Could not execute CD command");
        }
    } else if (strcmp(command->arguments[0], "showprocs") == 0) {
        isCustom = 1;
        printProcessList(&global_process_list);
    } else if (strcmp(command->arguments[0], "nap") == 0) {

        isCustom = 1;

        int nap_time = atoi(command->arguments[1]);
        int nap_pid = atoi(command->arguments[2]);

        int suspend_fork = fork();
        int kill_status;

        if (suspend_fork == 0) {
            kill_status = kill(nap_pid, SIGTSTP);


            if (kill_status == -1)
                perror("kill SIGTSTP failed");

            else {

                printf("%d handling SIGTSTP:\n", nap_pid);
                sleep(nap_time);
                kill_status = kill(nap_pid, SIGCONT);

                if (kill_status == -1)
                    perror("kill SIGCONT failed");

                else
                    printf("%d handling SIGCONT\n", nap_pid);
            }
            _exit(1);

        }

    } else if (strcmp(command->arguments[0], "stop") == 0) {

        isCustom = 1;

        int stop_pid = atoi(command->arguments[1]);
        if (kill(stop_pid, SIGINT) == -1)    /*terminated*/
            perror("kill SIGINT failed");
        else
            printf("%d handling SIGINT\n", stop_pid);
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
        if (cpid != -1)  //child success
            addProcess(&global_process_list, pCmdLine, cpid);
        if (return_code < 0) {
            perror("Can't execute the command");
            _exit(EXIT_FAILURE);
        }
        if (debug) {
            print_debug(cpid, pCmdLine);
        }
        if (pCmdLine->blocking) {
            waitpid(cpid, NULL, 0); //
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
    }
    return 0;
}