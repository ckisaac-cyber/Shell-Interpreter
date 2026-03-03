#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

// use construct to save the background information
typedef struct bg_process {
    int pid;
    char command[256];
    struct bg_process* next;
} bg_process;
//the signal will send when the subprocess is terminated and start, so there should be a value to determine when the subprocess is terminate
//if not the prompt will print twice.
volatile sig_atomic_t child_terminated = 0;
bg_process* bg_processes = NULL; 
// use linkedlist struct to save multi background information
int bg_process_count = 0;
volatile sig_atomic_t error_bg = 0;
bg_process* add_bg_process(int pid, char* command);
void sigquit();
void remove_bg_process(bg_process* p);
void handle_sigchld(int sig);
void execute_command(char* args[]);
void change_directory(char* path);
void execute_in_background(char* args[]);
void display_bg_processes();
void check_bg_processes();
void display_prompt();

int main(int argc, char *argv[]) {
    // set SIGCHLD to get a signal when the background process is done
    struct sigaction sa;
    sa.sa_handler = &handle_sigchld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        perror("sigaction");
        exit(1);
    }
    while(1) {
        display_prompt();
        char input[256];
        char* args[100] = {0};
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
        int i = 0;
        char* token = strtok(input, " ");
        while(token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        // use token to seprate the input

        
        if (args[0]!=NULL) {
            if (strcmp(args[0], "cd") == 0) {
                change_directory(args[1]);
            } else if (strcmp(args[0], "bg") == 0) {
                execute_in_background(args + 1);
            } else if (strcmp(args[0], "bglist") == 0) {
                display_bg_processes();
            }else if (strcmp(args[0], "exit") == 0) {
                exit(0);
            }
            
            else{
                execute_command(args);
            }
        }
    

    }
    return 0;
}

void execute_command(char* args[]) {
    child_terminated = 1;
    int pid = fork();
    if(pid == 0) {
        if(execvp(args[0], args) == -1) {
            printf("Invliad Command\n");
            exit(0);
        }
        exit(0);
    } else {
        waitpid(pid, NULL, 0);
        child_terminated = 1;
    }
    
}
//function to deal with cd
void change_directory(char* path) {
    if (!path || strcmp(path, "~") == 0) {
        chdir(getenv("HOME"));
    } else if (chdir(path) == -1) {
        printf("Invalid Directory\n");
    }
}

void execute_in_background(char* args[]) {
    int pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            //in subprocess, when user typed wrong command, send a signal to the father process and deal it seprately
            perror("\nError executing command");
             kill(getppid(),SIGUSR1);
            exit(0);
        }
        exit(0);
    } else {
//show the new added background process information
        char command[256] = "";
        for (int i = 0; args[i]; i++) {
            strcat(command, args[i]);
            if (args[i + 1]) {
                strcat(command, " ");
            }
        }
        
        add_bg_process(pid, command);
        printf("Added background process: PID: %d, Command: %s\n", pid, command);
//when using bg cat, the terminal will print prompt twice, and the txt file will be print after the prompt, so the bg cat should wait
//print the txt after prompt is printed, not print with prompt, and do not print prompt twice
        if(!strcmp(args[0],"cat")){
                child_terminated = 1;
                waitpid(pid,NULL,0);
                check_bg_processes();
                
        }else{
//this signal is only use for when execvp returns -1
            signal(SIGUSR1,sigquit);
             if(error_bg==1){
                child_terminated = 1;
                waitpid(pid,NULL,0);
                check_bg_processes();
                error_bg =0;
                
            }
            
            child_terminated = 0;
            
        }
        
    }
}
//add the background information to the struct
bg_process* add_bg_process(int pid, char* command) {
    bg_process* new_process = malloc(sizeof(bg_process));
    new_process->pid = pid;
    strcpy(new_process->command, command);
    new_process->next = bg_processes;
    bg_processes = new_process;
    bg_process_count++;
    return new_process;
}
//delete the information from the list struct
void remove_bg_process(bg_process* p) {
    if (!p) return;
    if (bg_processes == p) {
        bg_processes = p->next;
    } else {
        bg_process* tmp = bg_processes;
        while (tmp->next && tmp->next != p) {
            tmp = tmp->next;
        }
        if (tmp->next) {
            tmp->next = p->next;
        }
    }
    free(p);
    bg_process_count--;
}
//get information from the struct list, and print when adding process
void display_bg_processes() {
    int display_order = bg_process_count;
    printf("\nBackground processes:\n");
    for (bg_process* p = bg_processes; p; p = p->next) {
        printf("%d: %s JOBNUMBER: %d\n", p->pid, p->command,display_order);
        display_order--;
    }
    printf("Total Background jobs: %d\n\n", bg_process_count);
}
//use signal to check the background process when the process is terminated
void check_bg_processes() {
    int status;
    bg_process* p = bg_processes;
    while (p) {
        if (waitpid(p->pid, &status, WNOHANG) != 0) {
            printf("\n%d: %s has terminated.\n", p->pid, p->command);
            if(child_terminated==0){
                display_prompt();
            }
            
            bg_process* tmp = p;
            p = p->next;
            remove_bg_process(tmp);
        } else {
            p = p->next;
        }
    }
}
//just display the prompt, very easy function
void display_prompt() {
    char cwd[256];
    char hostname[50];
    gethostname(hostname, sizeof(hostname));
    printf("%s@%s: %s > ", getlogin(), hostname, getcwd(cwd, sizeof(cwd)));
    fflush(stdout);
}
//signal function
void handle_sigchld(int sig) {
    (void)sig;
    // check_bg_processes();
    if(child_terminated == 0){
        check_bg_processes();
        // child_terminated = 0;
    }else{
      
   
    }
    
}
//only use for when the user type invalied command in background
void sigquit(){
    
    error_bg = 1;

    
}