#include "sig_log.h"
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <dirent.h>

int switch_sig, dump_sig;
pid_t pid;
long long int timestamp_counter, dump_counter;
frequency sleep_frequency;
sem_t switch_sem, dump_sem;
int is_enabled, is_initialized = FALSE;
FILE* logs_file;
pthread_t log_switch_thread, log_dump_thread;
pthread_mutex_t switch_mutex, dump_mutex;
char* log_details[3] = {"MIN", "STANDARD", "MAX"};
char* frequencies[3] = {"FAST", "NORMAL", "SLOW"};
detail log_file_detail;

int init(detail arg_detail, int arg_is_enabled, frequency arg_sleep_frequency){
    if(is_initialized == TRUE) 
        return EXIT_FAILURE;

    int error_exit;

    timestamp_counter = 0;
    dump_counter = 0;
    switch_sig = SIGLOGS;
    dump_sig = SIGDUMP;

    log_file_detail = arg_detail;
    is_enabled = arg_is_enabled;
    sleep_frequency = arg_sleep_frequency;

    printf("LOGGING: \n");
    printf("- enabled: %s\n", (is_enabled == TRUE) ? "true" : "false");
    printf("- detail: %s\n", log_details[log_file_detail]);
    printf("- frequency: %ds [%s]\n", FREQUENCY(sleep_frequency), frequencies[sleep_frequency]);

    char date[30];
    char filename[100];
    time_t raw = time(0);
    struct tm *timeInfo = localtime(&raw);
    strftime(date, 30, DATE_FORMAT, timeInfo);
    sprintf(filename, "LOGS_%s.txt", date);
    logs_file = fopen(filename,"a+");
    if(logs_file == NULL){
        printf("date: %s\n", filename);
        return EXIT_FAILURE;
    }

    sigset_t switch_mask, dump_mask;
    struct sigaction switch_action, dump_action;

    switch_action.sa_sigaction = handle_switch;
    switch_action.sa_flags = SA_SIGINFO;
    sigfillset(&switch_mask);
    switch_action.sa_mask = switch_mask;
    error_exit = sigaction(switch_sig,&switch_action,NULL);
    if(error_exit){
        fclose(logs_file);
        return EXIT_FAILURE;
    }

    dump_action.sa_sigaction = handle_dump;
    dump_action.sa_flags = SA_SIGINFO;
    sigfillset(&dump_mask);
    dump_action.sa_mask = dump_mask;
    error_exit = sigaction(dump_sig,&dump_action,NULL);
    if(error_exit){
        fclose(logs_file);
        return EXIT_FAILURE;
    }

    error_exit = sem_init(&switch_sem,0,0);
    if(error_exit){
        fclose(logs_file);
        return EXIT_FAILURE;
    }
    error_exit = sem_init(&dump_sem,0,0);
    if(error_exit){
        fclose(logs_file);
        sem_destroy(&switch_sem);
        return EXIT_FAILURE;
    }

    error_exit = pthread_mutex_init(&switch_mutex,NULL);
    if(error_exit){
        fclose(logs_file);
        sem_destroy(&switch_sem);
        return EXIT_FAILURE;
    }

    error_exit = pthread_mutex_init(&dump_mutex,NULL);
    if(error_exit){
        fclose(logs_file);
        sem_destroy(&switch_sem);
        sem_destroy(&dump_sem);
        pthread_mutex_destroy(&dump_mutex);
        return EXIT_FAILURE;
    }

    error_exit = pthread_create(&log_dump_thread, NULL, log_dump, NULL);
    if(error_exit) {
        fclose(logs_file);
        sem_destroy(&switch_sem);
        sem_destroy(&dump_sem);
        pthread_mutex_destroy(&dump_mutex);
        pthread_mutex_destroy(&switch_mutex);
        return EXIT_FAILURE;
    }

    error_exit = pthread_create(&log_switch_thread, NULL, log_switch, NULL);
    if(error_exit) {
        fclose(logs_file);
        pthread_cancel(log_dump_thread);
        sem_destroy(&switch_sem);
        sem_destroy(&dump_sem);
        pthread_mutex_destroy(&dump_mutex);
        pthread_mutex_destroy(&switch_mutex);
        return EXIT_FAILURE;
    }

    error_exit = pthread_detach(log_switch_thread);
    if(error_exit) {
        fclose(logs_file);
        pthread_cancel(log_switch_thread);
        pthread_cancel(log_dump_thread);
        sem_destroy(&switch_sem);
        sem_destroy(&dump_sem);
        pthread_mutex_destroy(&dump_mutex);
        pthread_mutex_destroy(&switch_mutex);
        return EXIT_FAILURE;
    }

    error_exit = pthread_detach(log_dump_thread);
    if(error_exit) {
        fclose(logs_file);
        pthread_cancel(log_switch_thread);
        pthread_cancel(log_dump_thread);
        sem_destroy(&switch_sem);
        sem_destroy(&dump_sem);
        pthread_mutex_destroy(&dump_mutex);
        pthread_mutex_destroy(&switch_mutex);
        return EXIT_FAILURE;
    }

    is_initialized = TRUE;
    return EXIT_SUCCESS;
}

void my_sleep(){
    sleep(FREQUENCY(sleep_frequency));
}

void* dumps_saved(void *arg){
    while(TRUE){
        if(is_enabled == TRUE)
            timestamp_fprintf(logs_file, "Dumps saved = %d", dump_counter);
        my_sleep();
    }
    return NULL;
}

void* timestamps_saved(void *arg){
    while(TRUE){
        if(is_enabled == TRUE)
            timestamp_fprintf(logs_file, "Timestamps saved = %d", timestamp_counter);
        my_sleep();
    }
    return NULL;
}

void* frequency_currently_set(void *arg){
    while(TRUE){
        if(is_enabled == TRUE)
            timestamp_fprintf(logs_file, "Current logging frequency is %s", frequencies[sleep_frequency]);
        my_sleep();
    }
    return NULL;
}

int starts_with(const char *a, const char *b){
    return (strncmp(a, b, strlen(b)) == 0);
}

int available_dumps(){
    DIR* d;
    struct dirent *dir;
    d = opendir(".");
    int available_dumps = 0;
    if(d){
        while((dir = readdir(d)) != NULL){
            if(starts_with(dir->d_name, "dump_"))
                available_dumps++;
        }
    }
    return available_dumps;
}

void destroy(){
    if(logs_file != NULL){
        fclose(logs_file);
    }
    pthread_cancel(log_switch_thread);
    pthread_cancel(log_dump_thread);
    sem_destroy(&switch_sem);
    sem_destroy(&dump_sem);
    pthread_mutex_destroy(&dump_mutex);
    pthread_mutex_destroy(&switch_mutex);
}

void handle_switch(int signo, siginfo_t *info,void* other){
    sem_post(&switch_sem);
}

void handle_dump(int signo, siginfo_t *info,void* other){
    sem_post(&dump_sem);
}

void get_current_time(char* date, char* week_day, char* hour){
    time_t raw = time(0);
    struct tm *timeInfo = localtime(&raw);
    if(date != NULL)
        strftime(date, 30, DATE_FORMAT, timeInfo);
    if(week_day != NULL)
        strftime(week_day, 30, DAY_FORMAT, timeInfo);
    if(hour != NULL)
        strftime(hour, 30, TIME_FORMAT, timeInfo);
    return;
}

void timestamp_fprintf(FILE* f, char* string,...){
    if(is_initialized == FALSE)
        return;
    char date[30], day[30], hour[30];
    pthread_mutex_lock(&switch_mutex);
    fprintf(f, "[%s] ", log_details[log_file_detail]);
    if(log_file_detail == MIN){
        get_current_time(date, NULL, NULL);
        fprintf(f, "<%s> ", date);
    }
    else if(log_file_detail == STANDARD){
        get_current_time(date, day, NULL);
        fprintf(f,"<%s (%s)> ", date, day);
    }
    else if(log_file_detail == MAX){
        get_current_time(date, day, hour);
        fprintf(f,"<%s (%s) %s> ", date, day, hour);
    }
    va_list list;
    va_start(list,string);
    vfprintf(f, string, list);
    va_end(list);
    fprintf(f,"\n");
    fflush(f);
    pthread_mutex_unlock(&switch_mutex);
    timestamp_counter++;
}

void* log_switch(void *arg)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, switch_sig);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    while(TRUE){
        sem_wait(&switch_sem);
        is_enabled = is_enabled == TRUE ? FALSE : TRUE;
        is_enabled == TRUE ? (timestamp_fprintf(logs_file, "Logging is enabled!")) : (timestamp_fprintf(logs_file, "Logging is disabled!")); 
    }
    return NULL;
}

void* log_dump(void* arg)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, dump_sig);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    while(TRUE){
        sem_wait(&dump_sem);
        char timeBuf[30];
        char filename[100];
        time_t raw = time(0);
        struct tm *timeInfo = localtime(&raw);
        strftime(timeBuf, 30, TIME_FORMAT, timeInfo);
        sprintf(filename, "dump_%s.txt", timeBuf);
        FILE* file = fopen(filename, "w+");
        if(file == NULL)
            return NULL;
        timestamp_fprintf(file,"Timestamps saved %lld", timestamp_counter);
        timestamp_fprintf(file,"Dumps saved %lld", dump_counter);
        timestamp_fprintf(file, "Logging is %s", (is_enabled == TRUE ? "enabled" : "disabled"));
        timestamp_fprintf(file, "Available dumps: %d", available_dumps());
        fclose(file);    
        dump_counter++;
    }
    return NULL;
}


