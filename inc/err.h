#ifndef ERR_H
#define ERR_H

/* wypisuje informacje o blednym zakonczeniu funkcji systemowej 
i konczy dzialanie */
extern void syserr(const char *fmt, ...);

/* wypisuje informacje o bledzie i konczy dzialanie */
extern void fatal(const char *fmt, ...);

#define SYSCALL_WITH_CHECK(call) \
  if ((call) == -1) syserr("" #call "\n");

#endif /* ERR_H */
