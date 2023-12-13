#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main(int argc, char* vargs[]){
    char* program = vargs[0];
    int wstatus;
    pid_t cpid = fork();

    //printf("%s \n", vargs[1]);

    if(cpid == 0){
        // we are in the child
        printf("Child PID is %d \n", getpid());
        int ecode = execvp(vargs[1], &vargs[2]);

    } else {
        // we are in the parent
        pid_t w;

        do {
            w = waitpid(cpid, &wstatus, 0);
            if (w == -1) {
                perror("waitpid");
            }

            if (WIFEXITED(wstatus)) {
                printf("exited, status=%d\n", WEXITSTATUS(wstatus));
            } if (WIFSIGNALED(wstatus)) {
                printf("killed by signal %d\n", WTERMSIG(wstatus));
            } if (WIFSTOPPED(wstatus)) {
                printf("stopped by signal %d\n", WSTOPSIG(wstatus));
            }
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));



    }


}