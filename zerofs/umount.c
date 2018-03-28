#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <unistd.h>

int main(int argc, char *argv[])
{
    setresuid(0, 0, 0);

    int pid = fork();
    int status;

    if (!pid) {
        char *args[3];
        args[0] = "umount";
        args[1] = "/mnt";
        args[2] = NULL;
        execvp("/bin/umount", args);
    }
    waitpid(pid, &status, 0);

    return 0;
}
