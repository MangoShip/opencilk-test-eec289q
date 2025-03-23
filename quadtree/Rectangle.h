//
// Created by yt on 10/30/21.
//

#ifndef SCREENSAVER_RECTANGLE_H
#define SCREENSAVER_RECTANGLE_H

#include <math.h>
#include <stdbool.h>
#include "vec.h"

typedef struct _rect{
    Vec lower;
    Vec upper;
}Rect;

Rect move_rect(const struct Line* diagonal, double step);
bool intersects(const Rect* rect_fst, const Rect* rect_snd);
#endif //SCREENSAVER_RECTANGLE_H
