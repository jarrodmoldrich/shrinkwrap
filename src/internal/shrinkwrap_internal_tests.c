#include "minunit.h"
#include "shrinkwrap_triangle_internal.h"
#include "shrinkwrap_curve_internal.h"

typedef struct {
        curvep * l;
        curvep * r;
} intersect_curve;

void context_create_intersect_curve()
{

}

char * test_self_intersection() {
        return NULL;
}

char * test_shrinkwrap_internal() {
        mu_run_test(test_self_intersection);
        return NULL;
}

