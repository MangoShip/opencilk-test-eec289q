//
// Created by yt on 10/30/21.
//

#ifndef SCREENSAVER_QUADTREE_H
#define SCREENSAVER_QUADTREE_H

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>

#include "line.h"
#include "collision_world.h"
#include "intersection_event_list.h"
#include "intersection_detection.h"

typedef struct _quad_tree{
    Line ** lines;
    Line ** sublines;
    int num_lines;
    int num_sublines;
} QuadTree;

typedef struct _node{
    Line * line;
    struct _node* next;
} Node;

typedef IntersectionEventNode * IEN;

int  quadtree_intersections(CollisionWorld* collisionWorld, IntersectionEventListReducer* events);
void update_rectangles(CollisionWorld* collisionWorld);
void sort_event_list(IEN list);
#endif //SCREENSAVER_QUADTREE_H
