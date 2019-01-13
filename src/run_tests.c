//
// Created by Jarrod Moldrich on 2019-01-12.
//

#include <src/internal/minunit.h>
#include <printf.h>

int tests_run;

char * test_shrinkwrap_internal();

int main(int argc, const char ** argv) {
//        mu_run_test_suite(test_shrinkwrap_internal);
        do {
                char * message = test_shrinkwrap_internal();
                if (message) printf("Test failed: %s\n", message); else printf("Tests completed!\n");
                printf("Tests run: %d\n", tests_run);
        } while (0);
}
