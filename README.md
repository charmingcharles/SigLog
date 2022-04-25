# SigLog

This program uses three signals to log current state of the application. It doesn't have a purpose itself but it can be a base for many other aplications. We can easily build an app on top of it when we need any kind of logging during runtime. I used [multithreaded programming](https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html) and [signal handling](https://linux.die.net/man/7/signal).

## Instructions
The easiest way to understand the purpose and usage cases of this project is to run it. 
Compiling:
```
gcc -o [any_name] main.c sig_log.c sig_log.h -lpthread -lrt
```
Executing
If we execute our executable file without any parameters:
```
./[any_name]
```
We will we welcomed with:
```
use command "./test help" for more info!
PID = 1832  <-- random number
AVAILABLE SIGNALS:
- dump [35]
- loggging ON/OFF [36]
- exit the program [37]
LOGGING: 
- enabled: true
- detail: STANDARD
- frequency: 2s [NORMAL]
```
Everything always will be the same, only PID will change. 
To learn about changeable paramenters we need to run our program in a different way:
```
./test help
```
Output:
```
command line args:
d - logging detail: (MIN = 0, STANDARD = 1, MAX = 2)
f - logging frequency: (FAST = 0, NORMAL = 1, SLOW = 2)
l - logging enabled: (FALSE = 0, TRUE = 1)
example: ./test -d 1 -f 0 -l 2
```
And by knowing that we can change logging detail, logging frequency and even turn on/off logging itself. Example:
```
./test -d 1 -f 2 -l 0
```
## "My original" custom signals
I use existing [real-time signals](https://chromium.googlesource.com/chromiumos/docs/+/master/constants/signals.md). I redefined them for my personal usage:
```
#define SIGDUMP SIGRTMIN+1 --> no. 35
#define SIGLOGS SIGRTMIN+2 --> no. 36
#define SIGEXIT SIGRTMIN+3 --> no. 37
```

## Signal handling
Now we can open another terminal tab and we can instruct our currently running program from outside. 

Examples:
```
kill -35 [YOUR_PID]
```
Creates a dump with a timestamp. 


```
kill -36 [YOUR_PID]
```
It turns logging on/off 

And finally... We can turn off the whole program:
```
kill -37 [YOUR_PID]
```

## Timestamp file saving

I wrote very useful function that can be helpful with writing to files with some extra information. I used existing [sprintf()](https://cpp0x.pl/dokumentacja/standard-C/sprintf/586) function and "extened it". Function that I am talking about:
```
void timestamp_fprintf(FILE* f, char* string,...)
```
With some small changes it can be used in mutliple ways. I find it very helpful with debugging. 


