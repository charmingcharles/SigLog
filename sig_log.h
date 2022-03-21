#ifndef SCR_1_SIG_LOG_H
#define SCR_1_SIG_LOG_H

#include <stdio.h>
#include <signal.h>
#include <semaphore.h>

#define TRUE 1
#define FALSE 0

#define DATE_FORMAT "%m-%d-%y"
#define DAY_FORMAT "%A"
#define TIME_FORMAT "%H:%M:%S"

#define SIGDUMP SIGRTMIN+1
#define SIGLOGS SIGRTMIN+2
#define SIGEXIT SIGRTMIN+3

enum detail {MIN,STANDARD,MAX};
typedef enum detail detail;

enum frequency {FAST,NORMAL,SLOW};
typedef enum frequency frequency;

#define FREQUENCY(value) (value+1)

extern int dump_sig, switch_sig;
extern pid_t pid;

int init(detail arg_detail, int arg_is_enabled, frequency arg_sleep_frequency);

void my_sleep();
void* dumps_saved(void *arg);
void* timestamps_saved(void *arg);
void* frequency_currently_set(void *arg);

int starts_with(const char *a, const char *b);
int available_dumps();

void get_current_time(char* date, char* week_day, char* hour);
void timestamp_fprintf(FILE* f, char* string,...);

void handle_switch(int signo, siginfo_t *info, void* other);
void handle_dump(int signo, siginfo_t *info, void* other);

void* log_switch(void *arg);
void* log_dump(void* arg);

void destroy();

#endif
