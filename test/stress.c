/* $Id: stress.c,v 1.1 2001/06/12 02:11:27 bsd Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>


volatile int got_sig;


void sig_usr1(int sig)
{
  got_sig = 1;
  return;
}



pid_t openconn(char * dev, pid_t parent)
{
  int fd;
  pid_t pid;
  int rc;
  int i;

  pid = fork();
  if (pid < 0) {
    fprintf(stderr, "openconn(): can't fork(): %s\n", strerror(errno));
    return -1;
  }
  else if (pid > 0) {
    return pid;
  }

  fd = -1;
  while ((fd < 0) && (i<5000)) {
    fd = open(dev, O_RDWR);
    usleep(1000);
    i++;
  }

  if (fd < 0) {
    fprintf(stderr, "pid=%d, open(): %s\n", getpid(), strerror(errno));
    rc = kill(parent, SIGUSR1);
    if (rc < 0) {
      fprintf(stderr, "openconn(): kill(): %s\n", strerror(errno));
    }
    _exit(1);
  }

  fprintf(stderr, 
          "openconn(): pid=%d, \"%s\" opened, awaiting kill\n",
          getpid(), dev, i);

  rc = kill(parent, SIGUSR1);
  if (rc < 0) {
    fprintf(stderr, "openconn(): kill(): %s\n", strerror(errno));
  }

  pause();
  _exit(0);
}


void killproc(pid_t pid)
{
  pid_t p;
  int rc;
  int status;

  p = fork();
  if (p < 0) {
    fprintf(stderr, "killproc(): can't fork(): %s\n", strerror(errno));
    return;
  }
  else if (p > 0) {
    rc = waitpid(p, &status, 0);
    if (rc < 0) {
      fprintf(stderr, "killproc(): waitpid(): %s\n", strerror(errno));
      return;
    }
    return;
  }

  rc = kill(pid, SIGKILL);
  if (rc < 0) {
    fprintf(stderr, "killproc(): kill(): %s\n", strerror(errno));
    _exit(1);
    return;
  }

  fprintf(stderr, "killproc(): pid=%d killed\n", pid);

  _exit(0);
}



int main(int argc, char * argv[])
{
  pid_t p1, p2;
  int i;

  signal(SIGUSR1, sig_usr1);

  for (i=0; i<200; i++) {
    got_sig = 0;
    p1 = openconn("/usr/local/comserv/dev/modem1", getpid());
    while (!got_sig)
      ;
    killproc(p1);
    got_sig = 0;
    p2 = openconn("/usr/local/comserv/dev/modem1", getpid());
    while (!got_sig)
      ;
    killproc(p2);
    sleep(1);
  }

  fprintf(stderr, "done.");
  return 0;
}

