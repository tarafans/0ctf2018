#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <unistd.h>

int main(int argc, char *argv[])
{
    setresuid(0, 0, 0);

    int pid = fork();
    int status;

    if (!pid) {
        char *args[8];
        args[0] = "mount";
        args[1] = "-o";
        args[2] = "loop";
        args[3] = "-t";
        args[4] = "zerofs";
        args[5] = "/tmp/zerofs.img";
        args[6] = "/mnt";
        args[7] = NULL;
        execvp("/bin/mount", args);
    }
    waitpid(pid, &status, 0);

    pid = fork();
    if (!pid) {
        char *args[5];
        args[0] = "chown";
        args[1] = "-R";
        args[2] = "1000.1000";
        args[3] = "/mnt";
        args[4] = NULL;
        execvp("/bin/chown", args);
    }
    waitpid(pid, &status, 0);

    pid = fork();
    if (!pid) {
        char *args[5];
        args[0] = "chmod";
        args[1] = "-R";
        args[2] = "a+rwx";
        args[3] = "/mnt";
        args[4] = NULL;
        execvp("/bin/chmod", args);
    }
    waitpid(pid, &status, 0);

    return 0;
}
