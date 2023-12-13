#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

volatile long long int numReps = 0;
volatile int alarmStop = 0;

void notifyLoopReps() {
    printf("That's it, been 10 seconds.\n");
    alarmStop = 1;
    printf("Number of reps of loop executed: %lld\n", numReps);
}

int main() {

    printf("Current PID is %d.\n", getpid());

    struct sigaction act;
    act.sa_handler = notifyLoopReps;
    act.sa_flags = 0;
    //sigaddset(&act.sa_mask, SIGINT);
    sigaction(SIGALRM, &act, NULL);

    alarm(10);

    // 20 second loop
    while (numReps<9888800000000000 && !alarmStop) {
        numReps++;
    }

    
}