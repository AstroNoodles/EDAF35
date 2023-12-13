#include <stdio.h>
#include <signal.h>
#include <unistd.h>

/*
To run this code, run the executable and then try to stop it with Ctrl + C.
Then, run htop to get the PID of this process and use
kill -SIGINT <pid>, kill -SIGUSR1 <pid> and kill -SIGUSR2 <pid>
the code should only respond to SIGUSR2.
*/


void onForcedExit() {
    printf("YOU'VE EXITED THE INFINITE LOOP! \n");
}

void signalUSR2(int signum){
    if(signum == SIGUSR2){
        printf("USR2 Handled!\n");
    } else if(signum == SIGUSR1) {
        // But effectively ignored!
        //printf("USR1 Handled!\n");
    }
}

void infiniteSignalLoop(int signnum) {
    
    struct sigaction userSignal;
    userSignal.sa_handler = signalUSR2;
    userSignal.sa_flags = 0;

    sigaction(SIGUSR1, &userSignal, NULL);
    sigaction(SIGUSR2, &userSignal, NULL);

    // infinite loop
    long long int d = 0;
    while(d < (1 << 32)){
        d++;
    }
}

int main(){
    long long int c = 0;

    struct sigaction act;
    //act.sa_handler = onForcedExit;
    act.sa_handler = infiniteSignalLoop;
    act.sa_flags = 0;
    //sigaddset(&act.sa_mask, SIGINT);
    sigaction(SIGINT, &act, NULL);


    // infinite loop
    while(c < (100000000000000)) {
        //puts("hi");
        c++;
    }
}

