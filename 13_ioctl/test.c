#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "ioctl.h"

int main(int argc, char **argv){
    int ans, status;
    struct mystruct s;
    int fd= open("/dev/hello0", O_RDWR);
    if(fd < 0){
        perror("Open");
        return fd;
    }

    switch(argc){
        case 1:
            status = ioctl(fd, RD_VAL, &ans);
            if(status)
                perror("ioctl");
            printf("the answer is %d\n", ans);
            break;
        case 2:
            ans = atoi(argv[1]);
            status = ioctl(fd, WR_VAL, &ans);
            if(status)
                perror("ioctl");
            break;
        case 3:
            s.repeat = atoi(argv[1]);
            strcpy(s.name, argv[2]);
            status = ioctl(fd, GREET, &s);
            if(status)
                perror("ioctl");
            break;
        default:
            status = ioctl(fd, 123, &ans);
            if(status)
                perror("ioctl");
            break;
    }
    
    close(fd);
    return 0;
}