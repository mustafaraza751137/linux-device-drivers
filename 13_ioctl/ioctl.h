#ifndef MY_IOCTL_H
#define MY_IOCTL_H

struct mystruct{
    int repeat;
    char name[64];
};

#define WR_VAL _IOW('a', 'b', int *)
#define RD_VAL _IOR('a', 'b', int *)
#define GREET _IOW('a', 'd', struct mystruct *)

#endif  