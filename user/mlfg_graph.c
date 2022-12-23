
#include "kernel/types.h"
#include "user/user.h"


int number_of_processes = 5;
int a=0;
int b=1;
int c;
int main(int argc, char *argv[])
{
 
  int j;
  for (j = 0; j < number_of_processes; j++)
  {
    int pid = fork();
    if (pid < 0)
    {
      printf("Fork failed\n");
      continue;
    }
    if (pid == 0)
    {
      sleep(j*10+200); //io time
      for (int i = 0; i < 100000000; i++)
      {
        c=a;
        a=b;
        b=c;
      }
      
      printf("Process: PID %d :%d Finished\n", getpid(),j);
      exit(0);
    }
  }
  for (j = 0; j < number_of_processes+5; j++)
  {
    wait(0);
  }
  
  //getps();
  
  exit(0);
}