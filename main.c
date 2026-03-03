#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
// #include <qcustomplot.h>
#include "castsort.h"
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

#define NB_ELEM  1000000
#define infinity DBL_MAX

// #define RANGE_MAX infinity
#define RANGE_MAX 100

#define NB_WARMUP_LOOP 10
#define NB_TEST_LOOP 10

typedef struct {
    double myTime;
    double refTime;
} time_stats_t;

typedef enum {
    MY, REF
} sort_choice_en;

typedef double(*transform_fn_ptr)(double, double, double, double);

double transform_value_linear(double x, double min, double max, double delta){
    return x;
}

double transform_value_sigmoid(double x, double min, double max, double delta){
    // return x;
    x = (x - min)/(max - min);
    if (delta > 1) delta = 1/delta;
    const double sig = 1/((1 + exp(-x)) * delta);
    return sig;
}

double transform_value_sigmoid_med(double x, double median, double NOTHING, double delta){
    x = (x - median);
    if (delta < 1) delta = 1/delta;
    delta *= delta;
    const double sig = 1/(1 + exp(-x * delta));
    return sig;
}

double transform_value_other(double x, double min, double max, double delta){
    // return x;
    x = (x - min)/(max - min);
    if (delta > 1) delta = 1/delta;
    const double sig = 1/((1 + exp(-x * delta)));
    return sig;
}

int my_sort(value_t *toSort, mem_t *work_mem, value_t *sorted, const size_t size, const transform_fn_ptr transform_value, bool verbose) {
    if (toSort == NULL || work_mem == NULL || sorted == NULL) return 0;
    double maxVal = -infinity;
    double minVal = infinity;
    double deltaVal;
    int max_depth = 0;

    // Get information about to values to sort
    if (verbose)
        printf("Array initial values = [");
    for (size_t i = 0; i < size; i++) {
        if (verbose)
            printf("%lf, ", toSort[i].transformed_val);
        if (toSort[i].transformed_val < minVal) {
            minVal = toSort[i].transformed_val;
        }
        if (toSort[i].transformed_val > maxVal) {
            maxVal = toSort[i].transformed_val;
        }
    }
    if (verbose) {
        printf("]\n");
        printf("min = %lf\n", minVal);
        printf("max = %lf\n", maxVal);
    }

    deltaVal = maxVal - minVal;

    // In the case where min == max, it means all remaining values are equal
    if (deltaVal == 0) return 1;

    size_t biggest_collision = 0;
    // Find the nb in each slot
    for (size_t i = 0; i < size; ++i) {
        const size_t newIndex = (size - 1) * ((toSort[i].transformed_val - minVal) / deltaVal);
        work_mem[newIndex].nb++;
        work_mem[newIndex].act = 0;
        if (work_mem[newIndex].nb > biggest_collision) {
            biggest_collision = work_mem[newIndex].nb;
        }
    }

    // Find start index in final array for each slot
    size_t act = 0;
    if (verbose)
        printf("number of collisions = [");
    for (size_t i = 0; i < size; ++i) {
        if (verbose)
            printf("%zu, ", work_mem[i].nb);
        work_mem[i].start = act;
        act += work_mem[i].nb;
    }
    if (verbose)
        printf("]\n");

    // Do the sort
    for (size_t i = 0; i < size; ++i) {
        const size_t slotIndex = (size - 1) * ((toSort[i].transformed_val - minVal) / deltaVal);
        const size_t newIndex = work_mem[slotIndex].start + work_mem[slotIndex].act;
        toSort[i].final_index = newIndex;
        sorted[newIndex] = toSort[i];
        // todo: improve here recursing transformation function if the values are not the same
        if (work_mem[slotIndex].max < toSort[i].origin_val || work_mem[slotIndex].act == 0) work_mem[slotIndex].max = toSort[i].origin_val;
        if (work_mem[slotIndex].min > toSort[i].origin_val || work_mem[slotIndex].act == 0) work_mem[slotIndex].min = toSort[i].origin_val;
        work_mem[slotIndex].act++;
    }

    if (verbose) {
        printf("the array after sorting = [");
        for (size_t i = 0; i < size; ++i) {
            printf("%lf, ", sorted[i].transformed_val);
        }
        printf("]\n");
    }

    //Recurse in case of collision
    mem_t *tmp_work_mem = NULL;
    if (biggest_collision > 1) {
        tmp_work_mem = calloc(biggest_collision, sizeof(mem_t));
    }

    for (size_t i = 0; i < size; ++i) {
        if (work_mem[i].nb > 1) {
            const size_t nbElem = work_mem[i].nb;
            //memset(work_mem, 0, nbElem*sizeof(mem_t));
            const size_t startIndex = work_mem[i].start;
            if(work_mem[i].max - work_mem[i].min == 0) continue;
            const double v1 = sorted[startIndex + (random() % nbElem)].origin_val;
            double v2;
            do {v2 = sorted[startIndex + (random() % nbElem)].origin_val;} while (v1 == v2);
            const double delta = fabs(v1 - v2);
            const size_t offset = (random() % nbElem);
            work_mem[i].median = sorted[startIndex + offset].origin_val;
            if (verbose)
                printf("the transformed subarray = [");
            for (int j = 0; j < nbElem; ++j) {
                toSort[startIndex + j].transformed_val = transform_value(sorted[startIndex + j].origin_val, work_mem[i].median, work_mem[i].max, delta);
                toSort[startIndex + j].origin_val = sorted[startIndex + j].origin_val;
                if (verbose)
                    printf("%lf, ", toSort[startIndex + j].transformed_val);
            }
            if (verbose)
                printf("]\n");
            int local_depth = my_sort(&toSort[startIndex], tmp_work_mem, &sorted[startIndex], nbElem, transform_value, verbose);
            memset(tmp_work_mem, 0, biggest_collision * sizeof(mem_t));
            if (local_depth > max_depth) {
                max_depth = local_depth;
            }
        }
    }
    if (verbose) {
        printf("################################## nb of elements at depth %d : %zu \n", max_depth, size);
}
    if (biggest_collision > 1) {
        free(tmp_work_mem);
    }
    return max_depth + 1;
}

static inline void print_arrays(const double *mySecondToBeSortedArray, const value_t *sortedArray, const size_t nbElem) {
    if (mySecondToBeSortedArray == NULL || sortedArray == NULL || nbElem == 0) return;
    printf("contents: \n");
    for (size_t j = 0; j < nbElem; ++j) {
        if (mySecondToBeSortedArray[j] != sortedArray[j].origin_val)
            fprintf(stderr, "ref: %lf - my: %lf \n", mySecondToBeSortedArray[j], sortedArray[j].origin_val);
    }
}

int lower(const void *a, const void *b) {
    return *(double *) a > *(double *) b;
}

static inline int check_array(bool verbose, const double * mySecondToBeSortedArray, const value_t * sortedArray, const long long seed, const size_t nbVal){
    if(mySecondToBeSortedArray == NULL || sortedArray == NULL || nbVal < 1) return 0;
    if (verbose)
        printf("Checking results\n");
    for (size_t i = 0; i < nbVal; ++i) {
        if (fabs(mySecondToBeSortedArray[i] - sortedArray[i].origin_val) > exp(-6)) {
            if (verbose) {
                printf("The arrays don't have the same values!\n");
                print_arrays(mySecondToBeSortedArray, sortedArray, nbVal);
                printf("seed = %lld", seed);
            } else {
                printf("Error");
            }
            return 0;
        }
    }
    return 1;
}

static inline void my_warmup_pass(value_t * input, value_t * output, mem_t * workMem, const size_t nbVal, const transform_fn_ptr transform_value, bool verbose){
    my_sort(input, workMem, output, nbVal, transform_value, verbose);
}

static inline void ref_warmup_pass(double * input, const size_t nbVal){
    qsort(input, nbVal, sizeof(double), lower);
}

static inline double my_test_pass(value_t * input, value_t * output, mem_t * workMem, const size_t nbVal, const transform_fn_ptr transform_value, int * depth){
    struct timeval timeBegin, timeEnd;
    gettimeofday(&timeBegin, 0);
    *depth = my_sort(input, workMem, output, nbVal, transform_value, false);
    gettimeofday(&timeEnd, 0);
    return (double)timeEnd.tv_sec - (double)timeBegin.tv_sec + (double)(timeEnd.tv_usec - timeBegin.tv_usec) * 1e-6;
}

static inline double ref_test_pass(double * input, const size_t nbVal){
    struct timeval timeBegin, timeEnd;
    gettimeofday(&timeBegin, 0);
    qsort(input, nbVal, sizeof(double), lower);
    gettimeofday(&timeEnd, 0);
    return (double)timeEnd.tv_sec - (double)timeBegin.tv_sec + (double)(timeEnd.tv_usec - timeBegin.tv_usec) * 1e-6;
}

static inline void resetArrays(bool verbose, double * refArray, value_t * myArray, const double * originArray, const size_t nbVal){
    if(refArray == NULL || myArray == NULL || originArray == NULL) return;
    for (size_t i = 0; i < nbVal; ++i) {
        myArray[i].origin_val = originArray[i];
        myArray[i].transformed_val = myArray[i].origin_val;
        myArray[i].final_index = i;
        refArray[i] = originArray[i];
    }
}

int main(int argc, char **argv) {

    value_t *myToBeSortedArray;
    double *mySecondToBeSortedArray, *myOriginalToBeSortedArray;
    value_t *sortedArray;
    mem_t *myWorkMem;
    double myElapsed = 0, refElapsed = 0;

    bool verbose = true;
    bool is_exp = true;
    bool is_gaussian = true;
    bool compare_to_qsort = true;
    size_t maxVal = RANGE_MAX;
    size_t nbVal = NB_ELEM;
    size_t warmup_loops_nb = NB_WARMUP_LOOP;
    size_t test_loops_nb = NB_TEST_LOOP;
    long long seed = (unsigned int) time(NULL);

    // init gsl random variables
    const gsl_rng_type * T;
    gsl_rng * r;
    gsl_rng_env_setup();

    T = gsl_rng_default;
    r = gsl_rng_alloc (T);

    transform_fn_ptr transform_value = transform_value_linear;
    //seed = 17278702310;

    if (argc > 1) {
        verbose = false;
        char *endCharArgv1 = argv[1] + strlen(argv[1]);
        maxVal = strtoll(argv[1], &endCharArgv1, 10);
        char *endCharArgv2 = argv[2] + strlen(argv[2]);
        nbVal = strtoll(argv[2], &endCharArgv2, 10);
        if (argc > 3) {
            char *endCharArgv3 = argv[3] + strlen(argv[3]);
            seed = strtoll(argv[3], &endCharArgv3, 10);

            char *endCharArgv4 = argv[4] + strlen(argv[4]);
            warmup_loops_nb = strtoll(argv[4], &endCharArgv4, 10);
            char *endCharArgv5 = argv[5] + strlen(argv[5]);
            test_loops_nb = strtoll(argv[5], &endCharArgv5, 10);
            if (argc > 6) {
                transform_value = transform_value_sigmoid_med;
            }
            if (argc >= 7) {
                int dist_type = atoi(argv[6]);
                is_exp = dist_type % 2 == 1;
                is_gaussian = dist_type > 1;
            }
            if (argc >= 8) {
                compare_to_qsort = strcmp(argv[7], "1") == 0;
            }
            if (argc >= 9) {
                verbose = true;
            }
        }
    }

    //nbVal = 7;
    if (verbose)
        printf("Allocating memory\n");
    myToBeSortedArray = calloc(nbVal, sizeof(value_t));
    mySecondToBeSortedArray = malloc(nbVal * sizeof(double));
    myOriginalToBeSortedArray = malloc(nbVal * sizeof(double));
    sortedArray = calloc(nbVal, sizeof(value_t));
    myWorkMem = malloc(nbVal * sizeof(mem_t));
    srandom(seed);
    //srand((unsigned int) 42);

    // creating variables in case of normal / lognormal distribution
    double sigma = 0;
    double zeta = 0;
    sigma = ((double) random() / (double) (RAND_MAX)) * maxVal * 2 - maxVal;
    zeta = ((double) random() / (double) (RAND_MAX));


    double v_myToBeSortedArray[] = {-2, 15, 16, 14, 16, 17, 35};
    if (verbose)
        printf("Initializing the arrays\n");
    for (size_t i = 0; i < nbVal; ++i) {
        double new_val = ((double) random() / (double) (RAND_MAX)) * maxVal * 2 - maxVal;
        //new_val = v_myToBeSortedArray[i];
        if(is_exp && !is_gaussian) {
            myOriginalToBeSortedArray[i] = exp(new_val);
        }else if (!is_exp && !is_gaussian) {
            myOriginalToBeSortedArray[i] = new_val;
        }else if (!is_exp) {
            myOriginalToBeSortedArray[i] = gsl_ran_gaussian(r, sigma);
        }else {
            myOriginalToBeSortedArray[i] = gsl_ran_lognormal(r, zeta, sigma);
        }
    }

    resetArrays(verbose, mySecondToBeSortedArray, myToBeSortedArray, myOriginalToBeSortedArray, nbVal);
    memset(myWorkMem, 0, nbVal * sizeof(mem_t));

    if (verbose)
        printf("Starting my warming, may the proc be ready!\n");
    for (int i = 0; i < warmup_loops_nb; ++i) {
        resetArrays(verbose, mySecondToBeSortedArray, myToBeSortedArray, myOriginalToBeSortedArray, nbVal);
        memset(myWorkMem, 0, nbVal * sizeof(mem_t));
        my_warmup_pass(myToBeSortedArray, sortedArray, myWorkMem, nbVal, transform_value_sigmoid_med, verbose && i == 0);
    }

    int max_depth = 0;
    if (verbose)
        printf("Starting my measures, may the proc be with me!\n");
    for (int i = 0; i < test_loops_nb; ++i) {
        resetArrays(verbose, mySecondToBeSortedArray, myToBeSortedArray, myOriginalToBeSortedArray, nbVal);
        memset(myWorkMem, 0, nbVal * sizeof(mem_t));
        int local_depth = 0;
        myElapsed += my_test_pass(myToBeSortedArray, sortedArray, myWorkMem, nbVal, transform_value_sigmoid_med, &local_depth) / test_loops_nb;
        if (local_depth > max_depth) {
            max_depth = local_depth;
        }
    }

    int max_depth_ref = 0;
    if (verbose)
        printf("Starting the warming of the reference, may the proc be ready!\n");
    if (compare_to_qsort) {
        max_depth_ref = max_depth;
        for (int i = 0; i < warmup_loops_nb; ++i) {
            resetArrays(verbose, mySecondToBeSortedArray, myToBeSortedArray, myOriginalToBeSortedArray, nbVal);
            ref_warmup_pass(mySecondToBeSortedArray, nbVal);
        }

        if (verbose)
            printf("Starting the measure of the ref, may the fastest win!\n");
        for (int i = 0; i < test_loops_nb; ++i) {
            resetArrays(verbose, mySecondToBeSortedArray, myToBeSortedArray, myOriginalToBeSortedArray, nbVal);
            refElapsed += ref_test_pass(mySecondToBeSortedArray, nbVal) / test_loops_nb;
        }
    }else {
        for (int i = 0; i < warmup_loops_nb; ++i) {
            resetArrays(verbose, mySecondToBeSortedArray, myToBeSortedArray, myOriginalToBeSortedArray, nbVal);
            memset(myWorkMem, 0, nbVal * sizeof(mem_t));
            my_warmup_pass(myToBeSortedArray, sortedArray, myWorkMem, nbVal, transform_value_linear, false);
        }

        if (verbose)
            printf("Starting the measure of the ref, may the fastest win!\n");
        for (int i = 0; i < test_loops_nb; ++i) {
            int curr_depth = 0;
            resetArrays(verbose, mySecondToBeSortedArray, myToBeSortedArray, myOriginalToBeSortedArray, nbVal);
            memset(myWorkMem, 0, nbVal * sizeof(mem_t));
            refElapsed += my_test_pass(myToBeSortedArray, sortedArray, myWorkMem, nbVal, transform_value_linear, &curr_depth) / test_loops_nb;
            if (curr_depth > max_depth_ref) {
                max_depth_ref = curr_depth;
            }
        }
    }

    if (verbose && check_array(verbose, mySecondToBeSortedArray, sortedArray, seed, nbVal)) {
        printf("Results are the same\n");
        printf("And the winner is... ");
        if (refElapsed > myElapsed) {
            printf("Me!\n");
        } else {
            printf("The reference\n");
        }
        printf("With ref: %lf and my time: %lf", refElapsed, myElapsed);
    } else {
        printf("%lf - %lf - %d - %d", refElapsed, myElapsed, max_depth, max_depth_ref);
    }

    free(myWorkMem);
    free(myToBeSortedArray);
    free(myOriginalToBeSortedArray);
    free(mySecondToBeSortedArray);
    free(sortedArray);
    return 0;
}
