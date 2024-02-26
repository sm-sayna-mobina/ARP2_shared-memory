// blackboard.h
#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#define MAX_TARGETS 5
#define MAX_OBSTACLES 5

typedef struct {
    double x;
    double y;
} Point;

typedef struct {
    Point drone;
    Point targets[MAX_TARGETS];
    Point obstacles[MAX_OBSTACLES];
    char key_pressed;
} Blackboard;

typedef struct {
    double row;
    double col;
    char symbol;
    short color_pair;
    int active;
} Character;


#endif // BLACKBOARD_H
