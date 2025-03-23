//
// Created by yt on 10/31/21.
//

#include "Rectangle.h"
#include "line.h"

#if defined(__GNUC__) || defined(__clang__)
#define STATIC_INLINE static inline __attribute__((always_inline))
#else
#define STATIC_INLINE static inline
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define EPSILON (1e-4)

STATIC_INLINE Vec min(const Vec v1, const Vec v2, const Vec v3, const Vec v4){
    vec_dimension min_x = MIN(v1.x,MIN(v2.x, MIN(v3.x, v4.x)));
    vec_dimension min_y = MIN(v1.y,MIN(v2.y, MIN(v3.y,v4.y)));
    return Vec_make(min_x,min_y);
}

STATIC_INLINE Vec max(const Vec v1, const Vec v2, const Vec v3, const Vec v4){
    vec_dimension  max_x = MAX(v1.x,MAX(v2.x, MAX(v3.x,v4.x)));
    vec_dimension  max_y = MAX(v1.y,MAX(v2.y,MAX(v3.y,v4.y)));
    return Vec_make(max_x,max_y);
}

STATIC_INLINE Rect new_rect(const Vec lb, const Vec ub){
    Rect rect;
    rect.lower = lb;
    rect.upper = ub;
    return rect;
}

Rect move_rect(const Line* diagonal, double step){
    Vec pt_zero = diagonal->p1;
    Vec pt_fst  = diagonal->p2;
    Vec delta = Vec_multiply(diagonal->velocity,step);
    Vec npt_zero = Vec_add(pt_zero,delta);
    Vec npt_fst = Vec_add(pt_fst,delta);
    return new_rect(min(pt_zero,pt_fst,npt_zero,npt_fst), max(pt_zero,pt_fst,npt_zero,npt_fst));
}

bool intersects(const Rect* rect_fst, const Rect* rect_snd)
{
    bool y_fst_over_snd = rect_fst->upper.y - rect_snd->lower.y < -EPSILON;
    bool y_snd_over_fst = rect_snd->upper.y - rect_fst->lower.y < -EPSILON;
    bool x_fst_left_snd = rect_fst->upper.x - rect_snd->lower.x < -EPSILON;
    bool x_snd_left_fst = rect_snd->upper.x - rect_fst->lower.x < -EPSILON;

    bool no_intersection_in_y = y_fst_over_snd || y_snd_over_fst;
    bool no_intersection_in_x = x_fst_left_snd || x_snd_left_fst;

    bool no_intersection = no_intersection_in_x && no_intersection_in_y;

    return !no_intersection;

}