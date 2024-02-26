/*1-Standard C Library*/
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

/*2-File Descriptor Management*/
#include <fcntl.h>

/*3-Shared Memory Management*/
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

/*4-Socket and Networking Management*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

int *ptr;
#define  SHM_SIZE 100
char *sharedMemoryName =  "blackboard";  

int sockfd, newsockfd, clilent;
struct sockaddr_in serv_addr, cli_addr;

int* createSharedMemory(const char *sharedMemoryName);

void sig_killhandler(int signo);
void logData(char * fileName, char * mode);
void createLogFile(char * fileName, char * mode);

int main(int argc, char *argv[])
{   
  /*Creating a 'Log File' to record the process information like 'PID' and 'Last Activity Time'*/
  createLogFile("./server.txt","w+");
  /*Logging the data*/
  logData("server.txt","w+");
  //a 'SIGINT' signal from the 'watchdog process' to the 'motorx process'.
  signal(SIGINT, sig_killhandler);

    //Creates a shared memory
    ptr = createSharedMemory(sharedMemoryName);

    //Creates a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //Error Handling
    if (sockfd < 0) 
        perror("ERROR opening socket");

    //Config the server address
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(3500);

    //Bind the socket to the IP address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        perror("ERROR on binding");
            
    //Server is ready! Waits for new client to request
    printf ("Server is ready! Waits for new client to request...\n");
    listen(sockfd,5);

    //Accepts the request
    clilent = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilent);
    printf ("Accepts the request\n");

    //Error Handling
    if (newsockfd < 0)
        perror("ERROR on accept");
}

/*A function to create a named shared memory. */
int* createSharedMemory(const char *sharedMemoryName){
  //Creates a named shared memory object with 'O_CREAT | O_RDWR' flags and '0666' permission
  int shm_fd = shm_open(sharedMemoryName, O_CREAT | O_RDWR, 0666);

  //Error Handling
  if(shm_fd == -1 ){  
    printf("Shared memory creation failed!");
    exit(1);
  }

  //Configures the size of the shared memory object
  if(ftruncate(shm_fd, SHM_SIZE) == -1)
    printf("Trancate failed\n");

  //Maps the memory address of the calling process to the address of the shared memory 
  int *ptr = (int *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

   //Error Handling
  if (ptr == MAP_FAILED) {
    printf("Map failed\n");
  }

    //Returns the 'ptr' that points to the address of the shared memory
    return ptr;
}

/*
Defining the signal handlers function
*/
void sig_killhandler(int signo){
    if (signo == SIGINT){
        printf("I Received SIGINT Signal!\n");

        /*Close the resource*/
      // Unmap the shared memory
      if (munmap(ptr, SHM_SIZE) == -1) {
          perror("munmap");
          exit(EXIT_FAILURE);
      }

      // Close the shared memory descriptor
      if (close(*ptr) == -1) {
          perror("close");
          exit(EXIT_FAILURE);
      }

      // Unlink (delete) the shared memory object
      if (shm_unlink(sharedMemoryName) == -1) {
          perror("shm_unlink");
          exit(EXIT_FAILURE);
      }

         // Close the socket
      if (close(sockfd) == -1) {
          perror("Error closing socket");
          exit(EXIT_FAILURE);
      }

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

