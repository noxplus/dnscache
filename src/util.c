#include "inc.h"

uint32 random32(void)
{
    uint32 uiret = 0;
    int fd = open("/dev/urandom", O_RDONLY);
    read(fd, &uiret, sizeof(uiret));
    close(fd);

    return uiret;
}
