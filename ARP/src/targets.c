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
#include <sys/mman.h>
#include "blackboard.h"

#define NUM_TARGETS 5

//Shared Mmemory
int shm_fd;
Blackboard *shared_memory;

//Initialize the target's location
void initializeTargets(Point *targets_location){
        for (int i = 0; i < NUM_TARGETS; ++i) {
            targets_location[i].x = rand() % 30;
            targets_location[i].y = rand() % 150;
        }
    }


//Update the target's location
void updateTargets(Point *targets_location, Blackboard *shared_memory) {
    // Update the status of targets based on the drone's movement
    for (int i = 0; i < NUM_TARGETS; ++i) {
        if (shared_memory->targets[i].x != -1 && shared_memory->targets[i].y != -1) {
            // Check if the drone reached the target
            if (shared_memory->targets[i].x == shared_memory->drone.x &&
                shared_memory->targets[i].y == shared_memory->drone.y) {
                shared_memory->targets[i].x = -1;
                shared_memory->targets[i].y = -1;  // Mark as reached
            }
        }
    }
}


void sig_killhandler(int signo);
void logData(char * fileName, char * mode);
void createLogFile(char * fileName, char * mode);

int main(){
  /*Creating a 'Log File' to record the process information like 'PID' and 'Last Activity Time'*/
  createLogFile("./targets.txt","w+");
  /*Logging the data*/
  logData("targets.txt","w+");
  //a 'SIGINT' signal from the 'watchdog process' to the 'motorx process'.
  signal(SIGINT, sig_killhandler);
  
    shm_fd = shm_open("/blackboard", O_RDWR, 0666);
    shared_memory = (Blackboard *)mmap(NULL, sizeof(Blackboard), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    initializeTargets(shared_memory->targets);
    srand(time(NULL));
    while (1) {
        updateTargets(shared_memory->targets, shared_memory);
        sleep(1); // Adjust this based on how frequently you want to update targets
    }

    munmap(shared_memory, sizeof(Blackboard));
    close(shm_fd);
    return 0;
}

/*
Defining the signal handlers function
*/
void sig_killhandler(int signo){
    if (signo == SIGINT){
        printf("I Received SIGINT Signal!\n");
        /*Close the resource*/
        munmap(shared_memory, sizeof(Blackboard));
        close(shm_fd);

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

