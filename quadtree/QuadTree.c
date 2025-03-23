//
// Created by yt on 10/30/21.
//
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <cilk/cilk.h>

#include "QuadTree.h"
#include "collision_world.h"
#include "intersection_detection.h"
#include "intersection_event_list.h"
#include "Rectangle.h"

#define BIN_LIMIT 50
#define DEPTH_LIMIT 7
#define SIZE_LIMIT 21845
#define MID(x,y) ((x+y)/2)
//fwd decl
void handle_intersections(int index, CollisionWorld *collisionWorld,
                          IntersectionEventListReducer *intersection_events);


void ien_merge_nodes(IntersectionEventListReducer * left, IntersectionEventListReducer * right){
    if(right->size == 0){
        return;
    }

    if(left->size == 0){
        memcpy(left,right,sizeof(IntersectionEventList));
    }else{
        //connect
        left->size += right->size;
        left->tail->next = right->head;
        left->tail = right->tail;
    }

    //destroy right list
    right->size = 0;
    right->head = NULL;
    right->tail = NULL;
}

void update_rectangles(CollisionWorld* collisionWorld) {
    double t = collisionWorld->timeStep;
    for (int i = 0; i < collisionWorld->numOfLines; i++) {
        Line *line = collisionWorld->lines[i];
        line->rectangle = move_rect(line, t);
    }
}

IEN find_min_node(IEN list){
    IEN minNode = list;
    IEN curNode = list->next;

    while(curNode != NULL){
        if(IntersectionEventNode_compareData(curNode, minNode) < 0) {
            minNode = curNode;
        }
        curNode = curNode->next;
    }

    return minNode;
}

void sort_event_list(IEN node){
    IntersectionEventNode * startNode = node;

    while(startNode != NULL){
        IEN minNode = find_min_node(startNode);
        if(minNode != startNode){
            IntersectionEventNode_swapData(minNode, startNode);
        }
        startNode = startNode->next;
    }
}

static QuadTree collision_tree[SIZE_LIMIT];

/**
 * @comment update the quad tree with the lines at the position index 
 * @param index -> quad tree position index
 * @param num_lines -> number of lines in the node
 * @param lines -> pointer to the start of the lines in the child
 */
static void new_quad_tree(int index, int num_lines, Line **lines) {
    collision_tree[index].lines = lines;
    collision_tree[index].sublines = NULL;
    collision_tree[index].num_lines = num_lines;
}
enum QUAD{
    ZERO = 0,
    ONE,
    TWO,
    THREE,
    OUTSIDE
};
/**
 * 
 * @param line line
 * @param xmin line xmin
 * @param xmax line xmax
 * @param ymin line ymin
 * @param ymax line ymax
 * @return the quadrant number in which the line is contained based on QUAD
 */
static int find_quad_index(Line *line, double xmin, double xmax, double ymin,
                               double ymax) {
    // calculate the quadrant divisions
    double mid_x = MID(xmin,xmax);
    double mid_y = MID(ymin,ymax);
    
    if (line->rectangle.upper.x < mid_x) { // Leftside
        if (line->rectangle.upper.y < mid_y) // Upside
            return ZERO;
        else if (line->rectangle.lower.y > mid_y) // DownSide
            return TWO;
        else
            return OUTSIDE;
    } else if (line->rectangle.lower.x > mid_x) { // RightSide
        if (line->rectangle.upper.y < mid_y) // UpSide
            return ONE;
        else if (line->rectangle.lower.y > mid_y) // DownSide
            return THREE;
        else
            return OUTSIDE;
    } else {
        return OUTSIDE;
    }
}

/**
 * Sorts the Line array wrt quadrants
 * @param lines - lines to be sorted
 * @param assignment - quadrant assignments
 * @param children_sizes - how many children
 * @param num_lines 
 */
static void sort_lines_wrt_quad(Line **lines, int assignment[],
                                   int children_sizes[], int num_lines) {
    int line_ind = 0;
    int swap_ind = 0;
    int quad_start = 0;
    Line *temp;
    
    for (int quadrant = 0; quadrant < 4; quadrant++) {
        while (swap_ind < children_sizes[quadrant]) {
            if (assignment[line_ind] == quadrant) {
                //swap lines
                temp = lines[line_ind];
                lines[line_ind] = lines[quad_start + swap_ind];
                lines[quad_start + swap_ind] = temp;

                // swap the assignments
                assignment[line_ind] =
                        assignment[quad_start + swap_ind];
                assignment[quad_start + swap_ind] = quadrant;

                // next swap
                swap_ind++;
            }
            // next line
            line_ind++;
        }
        // next quad
        quad_start += children_sizes[quadrant];
        swap_ind = 0;
        line_ind = quad_start;
    }
}

/**
 * add lines to quadtree and create new quadtrees as needed
 * @param index - index of the tree
 * @param xmin -line
 * @param xmax -line
 * @param ymin -line
 * @param ymax -line
 * @param depth - depth of the tree
 */
static void build_quad_tree(int index, double xmin, double xmax, double ymin,
                                   double ymax, int depth) {

    // quadtree exceeds BIN_LIMIT and should be split
    int num_lines = collision_tree[index].num_lines;
    Line **lines = collision_tree[index].lines;

    double x_mid = MID(xmin,xmax);
    double y_mid = MID(ymin,ymax);

    //calculate the number of lines in each quadrant
    int assignment[collision_tree[index].num_lines];
    int children_sizes[5] = {0, 0, 0, 0, 0};

    for (int i = 0; i < num_lines; i++) {
        int quadrant = find_quad_index(lines[i], xmin, xmax, ymin, ymax);
        assignment[i] = quadrant;
        children_sizes[quadrant]++;
    }

    // sort the lines
    sort_lines_wrt_quad(lines, assignment, children_sizes, num_lines);

    // classify the lines wrt to quads
    Line **quad_zero_lines = lines;
    Line **quad_one_lines = quad_zero_lines + children_sizes[0];
    Line **quad_two_lines = quad_one_lines + children_sizes[1];
    Line **quad_three_lines = quad_two_lines + children_sizes[2];
    Line **non_quad_lines = quad_three_lines + children_sizes[3];

    //multi-quadrant lines
    collision_tree[index].sublines = lines;
    collision_tree[index].lines = non_quad_lines;

    // first child!!
    collision_tree[index].num_sublines =
            collision_tree[index].num_lines - children_sizes[4];
    collision_tree[index].num_lines = children_sizes[4];

    // new quad trees for children
    int new_depth = depth + 1;
    int child_base = 4 * index + 1;

    new_quad_tree(child_base, children_sizes[0], quad_zero_lines);
    new_quad_tree(child_base + 1, children_sizes[1], quad_one_lines);
    new_quad_tree(child_base + 2, children_sizes[2], quad_two_lines);
    new_quad_tree(child_base + 3, children_sizes[3], quad_three_lines);

    // recurse on the children as appropriate
    if (new_depth < DEPTH_LIMIT) {
        if (children_sizes[0] > BIN_LIMIT) {
            build_quad_tree(child_base, xmin, x_mid, ymin, y_mid,
                                              new_depth);
        }
        if (children_sizes[1] > BIN_LIMIT) {
            build_quad_tree(child_base + 1, x_mid, xmax, ymin,
                                              y_mid, new_depth);
        }
        if (children_sizes[2] > BIN_LIMIT) {
            build_quad_tree(child_base + 2, xmin, x_mid, y_mid,
                                              ymax, new_depth);
        }
        if (children_sizes[3] > BIN_LIMIT) {
            build_quad_tree(child_base + 3, x_mid, xmax, y_mid,
                                              ymax, new_depth);
        }
    }
    return;
}


static inline bool _intersects(Line *line1, Line *line2) {
    return intersects(&(line1->rectangle),&(line2->rectangle));
}


static void update_line_intersection(Line *line1, Line *line2,
                                 CollisionWorld *collisionWorld,
                                 IntersectionEventListReducer *intersection_events) {
    if (!_intersects(line1, line2)) {
        return;
    }

    if (compareLines(line1, line2) >= 0) {
        Line *temp = line1;
        line1 = line2;
        line2 = temp;
    }
    IntersectionType iType = intersect(line1, line2, collisionWorld->timeStep);
    if (iType != NO_INTERSECTION) {
        // De-reference IntersectionEventListReducer* to get thread-local IntersectionEventList view, then use & operator
        // This ensures IntersectionEventList_appendNode() to use local copy
        IntersectionEventList *events_ptr = &(*intersection_events);
        IntersectionEventList_appendNode(events_ptr, line1, line2, iType);
    }
}

static inline void
update_line_intersection_in_quad_tree(int index, CollisionWorld *collisionWorld,
                      IntersectionEventListReducer *intersection_events) {
    Line *line1;
    Line *line2;
    cilk_for (int i = 0; i < collision_tree[index].num_lines; i++) {
        line1 = collision_tree[index].lines[i];
        for (int j = i + 1; j < collision_tree[index].num_lines; j++) {
            line2 = collision_tree[index].lines[j];
            update_line_intersection(line1, line2, collisionWorld, intersection_events);
        }
    }

}

static void update_subline_intersections(int index, CollisionWorld *collisionWorld,
                                IntersectionEventListReducer *intersection_events) {
    Line *line1;
    Line *line2;
    Line **sublines = collision_tree[index].sublines;
    cilk_for (int i = 0; i < collision_tree[index].num_sublines; i++) {
        line1 = sublines[i];
        for (int j = 0; j < collision_tree[index].num_lines; j++) {
            line2 = collision_tree[index].lines[j];
            update_line_intersection(line1, line2, collisionWorld, intersection_events);
        }
    }
    

}

static void subline_intersections(int index, CollisionWorld *collisionWorld,
                             IntersectionEventListReducer *intersection_events) {
    int child_base = 4 * index + 1;
    cilk_for (int i = 0; i < 4; i++) {
        handle_intersections(child_base + i, collisionWorld, intersection_events);
    }
}

//handle all quad tree
void handle_intersections(int index, CollisionWorld *collisionWorld,
                          IntersectionEventListReducer *intersection_events) {

    if (collision_tree[index].sublines != NULL) {
        // handle quadtree pairs
        cilk_spawn update_line_intersection_in_quad_tree(index, collisionWorld,
                                         intersection_events);
        //handle tree at this node
        cilk_spawn update_subline_intersections(index, collisionWorld, intersection_events);

        // recurse
        subline_intersections(index, collisionWorld, intersection_events);

        cilk_sync;

    } else {
        //handle all pairs we have leafs...
        update_line_intersection_in_quad_tree(index, collisionWorld, intersection_events);
    }
}

int quadtree_intersections(
        CollisionWorld *collisionWorld,
        IntersectionEventListReducer *intersection_events) {
    new_quad_tree(0, collisionWorld->numOfLines, collisionWorld->lines);
    build_quad_tree(0, BOX_XMIN, BOX_XMAX, BOX_YMIN, BOX_YMAX, 0);
    handle_intersections(0, collisionWorld, intersection_events);
    int num_collisions = intersection_events->size;
    return num_collisions;
}
