#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <signal.h>
#include <time.h>
#include <fcntl.h> 
#include <unistd.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <errno.h>

void sig_killhandler(int signo);
void logData(char * fileName, char * mode);
void createLogFile(char * fileName, char * mode);

/*Creating the parent-child process*/
int spawn(const char * program, char * arg_list[]) {

  pid_t child_pid = fork();

  if(child_pid < 0) {
    perror("Error while forking...");
    return 1;
  }

  else if(child_pid != 0) {
    return child_pid;
  }

  else {
    if(execvp (program, arg_list) == 0);
    perror("Exec failed");
    return 1;
  }
}

int main() {
  
  /*Creating a 'Log File' to record the process information like 'PID' and 'Last Activity Time'*/
  createLogFile("./master.txt","w+");
  /*Logging the data*/
  logData("master.txt","w+");
  //a 'SIGINT' signal from the 'watchdog process' to the 'motorx process'.
  signal(SIGINT, sig_killhandler);

  /*Defining the command line's arguments for different process*/
    char * arg_list_window[] = { "/usr/bin/konsole", "-e", "./bin/window", NULL };
  /*Creating different processes*/
  pid_t pid_s = spawn("./bin/server", NULL);
  pid_t pid_d = spawn("./bin/drone", NULL);
  pid_t pid_ts = spawn("./bin/targets", NULL);
  pid_t pid_os = spawn("./bin/obstacles", NULL);
  pid_t pid_wd = spawn("./bin/watch_dog", NULL);
  pid_t pid_w = spawn("/usr/bin/konsole", arg_list_window);
    
 /*Waiting for all the child processes*/
  int status;
  waitpid(pid_s, &status, 0);
  waitpid(pid_d, &status, 0);
  waitpid(pid_ts, &status, 0);
  waitpid(pid_wd, &status, 0);
  waitpid(pid_w, &status, 0);

  printf ("Main program exiting with status %d\n", status);
  return 0;
}

/*
Defining the signal handlers function
*/
void sig_killhandler(int signo){
    if (signo == SIGINT){
        printf("I Received SIGINT Signal!\n");
        /*Close the resource*/

        /*Kill yourself!*/
        kill(getpid(), SIGKILL);
    }
}

/*a function to create a log file for the target process*/
void createLogFile(char * fileName, char * mode){
    /*In a new run of the programe, if the log file exist, remove the old log file and create a new one*/
    FILE *fp;
    if(!remove(fileName)){
        printf("The old log file is deleted successfully!\n");
    }
    if(fp = fopen(fileName, mode)){
        printf("The new log file is created successfully!\n");
        logData(fileName, mode);
    }else{
        printf("Could not create a log file!\n");
    }
}

/*a function to record the essential information of a process into a logfile.*/
void logData(char * fileName, char * mode){
   FILE *fp;
   int pid = getpid();

   time_t t = time(NULL);
   struct tm tm = *localtime(&t);
   
   /*Opening the logfile and record the current activity time of the process*/
   fp = fopen(fileName, mode);
   /*Error Checking*/ 
   if (fp<0){
      printf("Could not open the %s file; errno=%d\n", fileName, errno);
      exit(1); 
    }
    /*Writing into the file*/ 
    fprintf(fp, "%d,%d,%d,%d\n", pid, tm.tm_hour, tm.tm_min, tm.tm_sec);
    /*Closing the file*/
    fclose(fp);
}



