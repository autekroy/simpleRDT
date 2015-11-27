/*
 * setitimer.c - simple use of the interval timer
 */
#include <stdio.h>
#include <sys/time.h>		/* for setitimer */
#include <unistd.h>		/* for pause */
#include <signal.h>		/* for signal */
#include <stdlib.h>


#define INTERVAL 500		/* number of milliseconds to go off */

/*
 * DoStuff
 */
int now;
time_t rawtime;
struct tm * timeinfo;


void DoStuff(void) {
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  printf("Now: %d\t current: %s", now,   asctime (timeinfo) );

}

double simulate(double probability) {
    double random_value = (double) rand() / (double) RAND_MAX;
    if (random_value <  probability)
        return random_value;
    return random_value;
}


void alarm_handler(int sig)
{
	//retransmit everything in my window
	printf("Timer expired");
}

int main(int argc, char *argv[]) {
  now = 0;
  struct itimerval it_val;	/* for setting itimer */
  
  

  /* Upon SIGALRM, call DoStuff().
   * Set interval timer.  We want frequency in ms, 
   * but the setitimer call needs seconds and useconds. */
  if (signal(SIGALRM, (void (*)(int)) DoStuff) == SIG_ERR) {
    perror("Unable to catch SIGALRM");
    exit(1);
  }
  it_val.it_value.tv_sec =     (INTERVAL+now)/1000;
  it_val.it_value.tv_usec =    (INTERVAL*1000) % 1000000;	
  it_val.it_interval = it_val.it_value;  
  // if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
  //   perror("error calling setitimer()");
  //   exit(1);
  // }
  int inwhile = 0;
  // set random seed
  srand(time(NULL));

  
  if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
	perror("error calling setitimer()");
	exit(1);
  }
  pause();

 //  while (1) {
  	  
 //  	  if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
	//     perror("error calling setitimer()");
	//     exit(1);
 //  	  }
 //  	  else{
 //  	  	  now += 1000;
 //  	  	  it_val.it_value.tv_sec =     (INTERVAL+now)/1000;
	// 	  it_val.it_value.tv_usec =    (INTERVAL*1000) % 1000000;	
	// 	  it_val.it_interval = it_val.it_value;
 //  	  }
 //    	printf("%lf\n", simulate(0.6));
 //    	pause();
	// }
}

