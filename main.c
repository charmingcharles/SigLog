#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "sig_log.h"

pthread_t levelLogThread, dumpLogThread, frequencyLogThread;
sem_t exit_sem;

void handle_exit(int signo, siginfo_t *info,void* other);
void* exitLog(void *arg);

int main(int argc, char *argv[]) {
    if(argc > 1 && strcmp(argv[1], "help") == 0){
        printf("command line args:\n");
        printf("d - logging detail: (MIN = 0, STANDARD = 1, MAX = 2)\n");
        printf("f - logging frequency: (FAST = 0, NORMAL = 1, SLOW = 2)\n");
        printf("l - logging enabled: (FALSE = 0, TRUE = 1)\n");
        printf("example: %s -d 1 -f 0 -l 2\n", argv[0]);
        return EXIT_SUCCESS;
    }
    srand(time(NULL)); 
    detail detailing = STANDARD;
    int enabled = TRUE;
    frequency freq = NORMAL;
    for(int i = 1; i < argc; i += 2){
        if(argc > i && strcmp(argv[i], "-d") == 0){
        if(strcmp(argv[i + 1], "0") == 0)
            detailing = MIN;
        else if(strcmp(argv[i + 1], "1") == 0)
            detailing = STANDARD;
        else if(strcmp(argv[i + 1], "2") == 0)
            detailing = MAX;    
        }
        if(argc > i && strcmp(argv[i], "-l") == 0){
            if(strcmp(argv[i + 1], "0") == 0)
                enabled = FALSE;
            else
                enabled = TRUE;
        }
        if(argc > i && strcmp(argv[i], "-f") == 0){
        if(strcmp(argv[i + 1], "0") == 0)
            freq = SLOW;
        else if(strcmp(argv[i + 1], "1") == 0)
            freq = NORMAL;
        else if(strcmp(argv[i + 1], "2") == 0)
            freq = FAST;    
        }        
    }

    int exit_sig = SIGEXIT;
    int error_exit = sem_init(&exit_sem,0,0);
    if(error_exit)
        return EXIT_FAILURE;

    sigset_t exit_mask;
    struct sigaction exit_action;

    exit_action.sa_sigaction = handle_exit;
    exit_action.sa_flags = SA_SIGINFO;
    sigfillset(&exit_mask);
    exit_action.sa_mask = exit_mask;
    error_exit = sigaction(exit_sig,&exit_action,NULL);
    if(error_exit)
        return EXIT_FAILURE;

    pthread_t exitLogThread;    

    pid_t currentPid = getpid();
    printf("use command \"%s help\" for more info!\n", argv[0]);
    printf("PID = %d\n", currentPid);
    printf("AVAILABLE SIGNALS:\n");
    printf("- dump [%d]\n", SIGDUMP);
    printf("- loggging ON/OFF [%d]\n", SIGLOGS);
    printf("- exit the program [%d]\n", SIGEXIT);
    if(init(detailing, enabled, freq) == EXIT_SUCCESS){
        pid = currentPid;
        pthread_create(&levelLogThread,NULL,dumps_saved, NULL); 
        pthread_create(&dumpLogThread,NULL,timestamps_saved, NULL); 
        pthread_create(&frequencyLogThread,NULL,frequency_currently_set, NULL); 
        pthread_create(&exitLogThread,NULL,exitLog, NULL); 
        pthread_join(levelLogThread,NULL);
        pthread_join(dumpLogThread,NULL);
        pthread_join(frequencyLogThread,NULL);
        pthread_join(exitLogThread,NULL);
        destroy();
        sem_destroy(&exit_sem);
        printf("Program successfully ended!\n");
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

void handle_exit(int signo, siginfo_t *info,void* other){
    sem_post(&exit_sem);
}

void* exitLog(void *arg){
        sem_wait(&exit_sem);
        pthread_cancel(levelLogThread);
        pthread_cancel(dumpLogThread);
        pthread_cancel(frequencyLogThread);
        pthread_exit(NULL);
}
