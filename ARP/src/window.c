#include <ncurses.h>
#include <stdlib.h>
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
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <errno.h>
#include "blackboard.h"


int shm_fd;
Blackboard *shared_memory;

// Define the border dimensions
#define BORDER_WIDTH 140
#define BORDER_HEIGHT 32


void sig_killhandler(int signo);
void logData(char * fileName, char * mode);
void createLogFile(char * fileName, char * mode);

//Draw border to show the working area of the drone
void drawBorder(){
      // Draw horizontal borders
        attron(COLOR_PAIR(1)); // Use blue color for the border
        for (int i = 0; i < BORDER_WIDTH; ++i) {
            mvaddch(0, i, '-');                           // Top border
            mvaddch(BORDER_HEIGHT - 1, i, '-');           // Bottom border
        }
        attroff(COLOR_PAIR(1));

        // Draw vertical borders
        attron(COLOR_PAIR(1)); // Use blue color for the border
        for (int i = 0; i < BORDER_HEIGHT; ++i) {
            mvaddch(i, 0, '|');                          // Left border
            mvaddch(i, BORDER_WIDTH - 1, '|');          // Right border
        }
        attroff(COLOR_PAIR(1));

}


// Function to initialize colors in NCurses
void initialize_colors() {
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_BLUE, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    }
}

// Function to draw a character on the screen based on the Character structure
void draw_character(Character *character) {
    wattron(stdscr, COLOR_PAIR(character->color_pair));
    mvaddch(character->row, character->col, character->symbol);
    wattroff(stdscr, COLOR_PAIR(character->color_pair));
}

// Function to update targets and obstacles based on shared memory
void readUserKeyPressed(Blackboard *shared_memory) {
    // Move drone based on user input
    char ch = getch();
    shared_memory->key_pressed = ch;
}

int main() {

  /*Creating a 'Log File' to record the process information like 'PID' and 'Last Activity Time'*/
  createLogFile("./window.txt","w+");
  /*Logging the data*/
  logData("window.txt","w+");
  //a 'SIGINT' signal from the 'watchdog process' to the 'motorx process'.
  signal(SIGINT, sig_killhandler);
  
  // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

   initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0); // Hide cursor
    timeout(200); // Set a timeout for getch() to simulate real-time behavior

    initialize_colors();

    shm_fd = shm_open("/blackboard", O_RDWR, 0666);
    shared_memory = (Blackboard *)mmap(NULL, sizeof(Blackboard), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    while (1) {
        clear(); // Clear the screen

    //Draw border
    drawBorder();

    // Draw drone
  Character drone_character = {
    .row = shared_memory->drone.x,
    .col = shared_memory->drone.y,
    .symbol = '+',
    .color_pair = 1,
    .active = 1
};
draw_character(&drone_character);

        // Draw targets
        for (int i = 0; i < MAX_TARGETS; ++i) {
            if (shared_memory->targets[i].x != -1 && shared_memory->targets[i].y != -1) {
                Character target = {
                    .row = shared_memory->targets[i].x,
                    .col = shared_memory->targets[i].y,
                    .symbol = '0' + i,
                    .color_pair = 2,
                    .active = 1
                };
                draw_character(&target);
            }
        }

        // Draw obstacles
        for (int i = 0; i < MAX_OBSTACLES; ++i) {
            Character obstacle = {
                .row = shared_memory->obstacles[i].x,
                .col = shared_memory->obstacles[i].y,
                .symbol = 'O',
                .color_pair = 3,
                .active = 1
            };
            draw_character(&obstacle);
        }

        // Refresh the screen
        refresh();

        // Move drone based on user input
        readUserKeyPressed(shared_memory);
    }

    munmap(shared_memory, sizeof(Blackboard));
    close(shm_fd);
    endwin();
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


