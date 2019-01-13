#include <stdio.h>

#define MU_MAX_MESSAGE 1024
static char mu_last_message[MU_MAX_MESSAGE];

#define mu_assert(message, test)\
        do {\
                if (!(test)) return message;\
        } while (0)

#define mu_equals_generic(TYPE, printf_modifier, expected, test)\
        do {\
                TYPE result = (TYPE)(test);\
                if (expected != result) {\
                        snprintf(\
                                mu_last_message\
                                , MU_MAX_MESSAGE\
                                , "%" printf_modifier " != %" printf_modifier ". %s:%d:%s"\
                                , (TYPE)expected\
                                , result\
                                , __FILE__\
                                , __LINE__\
                                , #test\
                        );\
                        return mu_last_message;\
                }\
        } while (0)

#define mu_equals_double(expected, test) mu_equals_generic(double, "f", expected, test)
#define mu_equals_int(expected, test) mu_equals_generic(int, "d", expected, test)

#define mu_run_test(test)\
        do {\
                printf("TEST: " #test "\n");\
                char *message = test;\
                tests_run++;\
                if (message) return message;\
        } while (0)

#define mu_run_test_suite(test)\
        do {\
                char * message = test();\
                if (message) printf("Test failed: %s\n", message); else printf("Tests completed!\n");\
                printf("Tests run: %d\n", tests_run);\
        } while (0)

extern int tests_run;
