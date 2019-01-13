#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include "minunit.h"
#include "shrinkwrap_triangle_internal.h"
#include "shrinkwrap_curve_internal.h"

typedef struct {
        CP * l;
        CP * r;
} intersect_curve;

typedef struct _test_point {
        float x;
        float y;
} test_point;

typedef struct _test_point_list {
        const test_point * points;
        const size_t count;
        const alpha a;
        C * curveresult;
} test_point_list;

test_point_list make_point_list(test_point points[], alpha a, size_t count) {
        test_point_list list = { points, count, a, NULL };
        return list;
}

#define create_test_curve_list_static_array(curves, height) \
        create_test_curve_list(curves, sizeof(curves) / sizeof(test_point_list), height)

C * create_test_curve(curve_list * list, const test_point * const points, size_t pointcount, alpha a)
{
        const test_point * point = points;
        C * c = (C *)malloc(c_size);
        CP * p = init_curve(c, point->x, point->y, list, a);
        add_curve(list, c, p);
        add_curve_to_scanline(list, (size_t)point->y, c, p);
        while (--pointcount) {
                point++;
                p = append_point_to_curve(p, list, point->x, point->y);
                add_curve_to_scanline(list, (size_t)point->y, c, p);
        }
        return c;
}

C * create_test_curve_from_list(curve_list * list, const test_point_list * points)
{
        return create_test_curve(list, points->points, points->count, points->a);
}


curve_list * create_test_curve_list(test_point_list * curves, size_t curvecount, size_t height)
{
        assert(height > 0);
        curve_list * list = create_curve_list(height);
        test_point_list * curve = curves;
        while (curvecount--) {
                C * c = create_test_curve(list, curve->points, curve->count, curve->a);
                curve->curveresult = c;
                curve++;
        }
        return list;
}

CP * get(CP * p, int idx) {
        while (idx-- > 0) { p = p->next; }
        assert(p != NULL);
        return p;
}

const size_t SLOPE_HEIGHT = 4;

typedef struct {
        test_point * leftedge;
        test_point * increaseslope;
        test_point * decreaseslope;
        test_point * rightedge;
} slopes_fixture;

test_point * malloc_point_list(test_point list[], size_t height) {
        size_t size = height * sizeof(test_point);
        test_point * memory = (test_point *)malloc(size);
        memcpy(memory, list, size);
        return memory;
}

slopes_fixture create_slopes_fixture() {
        slopes_fixture fixture;
        test_point leftedge[] = {
                {0.f, 0.f},
                {0.f, 1.f},
                {0.f, 2.f},
                {0.f, 3.f}
        };
        test_point increaseslope[] = {
                {2.f, 0.f},
                {1.f, 1.f},
                {1.f, 2.f},
                {2.f, 3.f}
        };
        test_point decreaseslope[] = {
                {1.f, 0.f},
                {2.f, 1.f},
                {2.f, 2.f},
                {1.f, 3.f}
        };
        test_point rightedge[] = {
                {3.f, 0.f},
                {3.f, 1.f},
                {3.f, 2.f},
                {3.f, 3.f}
        };
        fixture.leftedge = malloc_point_list(leftedge, SLOPE_HEIGHT);
        fixture.increaseslope = malloc_point_list(increaseslope, SLOPE_HEIGHT);
        fixture.decreaseslope = malloc_point_list(decreaseslope, SLOPE_HEIGHT);
        fixture.rightedge = malloc_point_list(rightedge, SLOPE_HEIGHT);
        return fixture;
}

void destroy_slopes_fixture(slopes_fixture fixture) {
        free(fixture.leftedge);
        free(fixture.increaseslope);
        free(fixture.decreaseslope);
        free(fixture.rightedge);
}

char * test_optimise() {
        slopes_fixture fixture = create_slopes_fixture();
        // test increase
        {
                test_point_list curveincrease[3] = {
                        make_point_list(fixture.leftedge, ALPHA_ZERO, SLOPE_HEIGHT)
                        , make_point_list(fixture.increaseslope, ALPHA_FULL, SLOPE_HEIGHT)
                        , make_point_list(fixture.rightedge, ALPHA_ZERO, SLOPE_HEIGHT)
                };
                create_test_curve_list_static_array(curveincrease, SLOPE_HEIGHT);
                CP * p = curveincrease[1].curveresult->pointList;
                mu_equals_double(1.f, optimise(get(p, 0), get(p, 1), get(p, 2), CONSERVE_LEFT, 0.0f)); // cut corner
                mu_equals_double(2.f, optimise(get(p, 1), get(p, 2), get(p, 3), CONSERVE_LEFT, 0.0f)); // cut corner
                mu_equals_double(0.f, optimise(get(p, 0), get(p, 1), get(p, 2), CONSERVE_RIGHT, 0.0f)); // extend entry slope
                mu_equals_double(1.f, optimise(get(p, 1), get(p, 2), get(p, 3), CONSERVE_RIGHT, 0.0f)); // extend entry slope

        }
        // test decrease
        {
                test_point_list curvedecrease[3] = {
                        make_point_list(fixture.leftedge, ALPHA_ZERO, SLOPE_HEIGHT)
                        , make_point_list(fixture.decreaseslope, ALPHA_FULL, SLOPE_HEIGHT)
                        , make_point_list(fixture.rightedge, ALPHA_ZERO, SLOPE_HEIGHT)
                };
                create_test_curve_list_static_array(curvedecrease, SLOPE_HEIGHT);
                CP * p = curvedecrease[1].curveresult->pointList;
                mu_equals_double(3.f, optimise(get(p, 0), get(p, 1), get(p, 2), CONSERVE_LEFT, 0.0f)); // extend entry slope
                mu_equals_double(2.f, optimise(get(p, 1), get(p, 2), get(p, 3), CONSERVE_LEFT, 0.0f)); // extend entry slope
                mu_equals_double(2.f, optimise(get(p, 0), get(p, 1), get(p, 2), CONSERVE_RIGHT, 0.0f)); // cut corner
                mu_equals_double(1.f, optimise(get(p, 1), get(p, 2), get(p, 3), CONSERVE_RIGHT, 0.0f)); // cut corner
        }
        destroy_slopes_fixture(fixture);
        return NULL;
}

char * check_conserve_direction(alpha before, alpha middle, conserve expected) {
        slopes_fixture fixture = create_slopes_fixture();
        test_point_list curves[2] = {
                make_point_list(fixture.leftedge, before, SLOPE_HEIGHT),
                make_point_list(fixture.increaseslope, middle, SLOPE_HEIGHT),
        };
        create_test_curve_list_static_array(curves, SLOPE_HEIGHT);
        CP *p = curves[1].curveresult->pointList;
        mu_equals_int(expected, conserve_direction(p->scanlineList, curves[1].curveresult));
        destroy_slopes_fixture(fixture);
        return NULL;
}

char * test_conserve_direction() {
        // test conserve strategy
        mu_run_test(check_conserve_direction(ALPHA_ZERO, ALPHA_FULL, CONSERVE_LEFT));
        mu_run_test(check_conserve_direction(ALPHA_ZERO, ALPHA_PARTIAL, CONSERVE_RIGHT));
        mu_run_test(check_conserve_direction(ALPHA_FULL, ALPHA_ZERO, CONSERVE_RIGHT));
        mu_run_test(check_conserve_direction(ALPHA_FULL, ALPHA_PARTIAL, CONSERVE_RIGHT));
        mu_run_test(check_conserve_direction(ALPHA_PARTIAL, ALPHA_ZERO, CONSERVE_LEFT));
        mu_run_test(check_conserve_direction(ALPHA_PARTIAL, ALPHA_FULL, CONSERVE_LEFT));
        return NULL;
}

char * test_shrinkwrap_internal() {
        mu_run_test(test_optimise());
        mu_run_test(test_conserve_direction());
        return NULL;
}

