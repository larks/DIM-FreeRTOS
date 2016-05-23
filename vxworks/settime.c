#include <time.h>

dim_settime()
{
  struct timespec tp;

  tp.tv_sec = 1000;
  tp.tv_nsec = 0;
  clock_settime(CLOCK_REALTIME, &tp);
}

dim_gettime()
{
  struct timespec tp;
  int ret;

  ret = clock_settime(CLOCK_REALTIME, &tp);
    printf("sec = %d, nsec = %d\n",tp.tv_sec, tp.tv_nsec);
  return(ret);
}
