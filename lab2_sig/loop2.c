#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void printSigset(const sigset_t *sigset) {
    int sig = 0;

    // NSIG is the maximum amount of signals defined by Linux so
    // hopefully this catches any type of signal that shows up?
    // double check this
    for(int sig = 0; sig < NSIG; sig++) {
        if(sigismember(sigset, sig)){
            printf("%s%d (%s)\n", "\t", sig, strsignal(sig));
        }
    }
}

int main() {

    sigset_t pending_signals, all_signals;

    sigfillset(&all_signals);
    sigprocmask(SIG_BLOCK, &all_signals, NULL);

    printf("Current PID is %d.\n", getpid());

    // 10 second loop, Nearly all signals blocked.
    long long int d = 0;
    while (d<9800000000) {
        d++;
    }

    printf("Pending Signals are: \n");
    // Print all signals received during the loop
    sigpending(&pending_signals);
    printSigset(&pending_signals);
    // unblock signals
    sigprocmask(SIG_UNBLOCK, &all_signals, NULL);
}