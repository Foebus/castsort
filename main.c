#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
// #include <qcustomplot.h>
#include "castsort.h"

#define NB_ELEM  150000000
#define infinity 1000000000.0

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

double transform_value(double x, double min, double max, double delta){
    // return x;
    x = (x - min)/(max - min);
    if (delta > 1) delta = 1/delta;
    const double sig = 1/((1 + exp(-x)) * delta);
    return sig;
}

void my_sort(value_t *toSort, mem_t *work_mem, value_t *sorted, const size_t size) {
    double maxVal = -infinity;
    double minVal = infinity;
    double deltaVal;

    // Get information about to values to sort
    for (size_t i = 0; i < size; i++) {
        if (toSort[i].transformed_val < minVal) {
            minVal = toSort[i].transformed_val;
        }
        if (toSort[i].transformed_val > maxVal) {
            maxVal = toSort[i].transformed_val;
        }
    }
    deltaVal = maxVal - minVal;

    // In the case where min == max, it means all remaining values are equal
    if (deltaVal == 0) return;

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
    for (size_t i = 0; i < size; ++i) {
        work_mem[i].start = act;
        act += work_mem[i].nb;
    }

    // Do the sort
    for (size_t i = 0; i < size; ++i) {
        const size_t slotIndex = (size - 1) * ((toSort[i].transformed_val - minVal) / deltaVal);
        const size_t newIndex = work_mem[slotIndex].start + work_mem[slotIndex].act;
        toSort[i].final_index = newIndex;
        sorted[newIndex] = toSort[i];
        if (work_mem[slotIndex].max < toSort[i].origin_val || work_mem[slotIndex].act == 0) work_mem[slotIndex].max = toSort[i].origin_val;
        if (work_mem[slotIndex].min > toSort[i].origin_val || work_mem[slotIndex].act == 0) work_mem[slotIndex].min = toSort[i].origin_val;
        work_mem[slotIndex].act++;
    }

    //Recurse in case of collision
    mem_t *tmp_work_mem = NULL;
    if (biggest_collision > 1) {
        tmp_work_mem = calloc(biggest_collision, sizeof(mem_t));
    }
    for (size_t i = 0; i < size; ++i) {
        if (work_mem[i].nb > 1) {
            const int nbElem = work_mem[i].nb;
            //memset(work_mem, 0, nbElem*sizeof(mem_t));
            const size_t startIndex = work_mem[i].start;
            const double delta = work_mem[i].max - work_mem[i].min;
            if(work_mem[i].max - work_mem[i].min == 0) continue;
            for (int j = 0; j < nbElem; ++j) {
                toSort[startIndex + j].transformed_val = transform_value(sorted[startIndex + j].origin_val, work_mem[i].min, work_mem[i].max, delta);
                toSort[startIndex + j].origin_val = sorted[startIndex + j].origin_val;
            }
            my_sort(&toSort[startIndex], tmp_work_mem, &sorted[startIndex], nbElem);
            memset(tmp_work_mem, 0, biggest_collision * sizeof(mem_t));
        }
    }
    if (biggest_collision > 1) {
        free(tmp_work_mem);
    }
}

static inline void print_arrays(const double *mySecondToBeSortedArray, const value_t *sortedArray, const size_t nbElem) {
    printf("contents: \n");
    for (size_t j = 0; j < nbElem; ++j) {
        if (mySecondToBeSortedArray[j] !=  sortedArray[j].origin_val)
            fprintf(stderr, "ref: %lf - my: %lf \n", mySecondToBeSortedArray[j], sortedArray[j].origin_val);
    }
}

int lower(const void *a, const void *b) {
    return *(double *) a > *(double *) b;
}

static inline int check_array(bool verbose, double * mySecondToBeSortedArray, value_t * sortedArray, long long seed, size_t nbVal){
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
            break;
        }
    }
    return 1;
}

static inline void my_warmup_pass(value_t * input, value_t * output, mem_t * workMem, size_t nbVal){
    my_sort(input, workMem, output, nbVal);
}

static inline void ref_warmup_pass(double * input, size_t nbVal){
    qsort(input, nbVal, sizeof(double), lower);
}

static inline double my_test_pass(value_t * input, value_t * output, mem_t * workMem, size_t nbVal){
    struct timeval timeBegin, timeEnd;
    gettimeofday(&timeBegin, 0);
    my_sort(input, workMem, output, nbVal);
    gettimeofday(&timeEnd, 0);
    return timeEnd.tv_sec - timeBegin.tv_sec + (timeEnd.tv_usec - timeBegin.tv_usec) * 1e-6;
}

static inline double ref_test_pass(double * input, size_t nbVal){
    struct timeval timeBegin, timeEnd;
    gettimeofday(&timeBegin, 0);
    qsort(input, nbVal, sizeof(double), lower);
    gettimeofday(&timeEnd, 0);
    return timeEnd.tv_sec - timeBegin.tv_sec + (timeEnd.tv_usec - timeBegin.tv_usec) * 1e-6;
}

static inline void resetArrays(bool verbose, double * refArray, value_t * myArray, double * originArray, size_t nbVal){
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
    size_t maxVal = RANGE_MAX;
    size_t nbVal = NB_ELEM;
    size_t warmup_loops_nb = NB_WARMUP_LOOP;
    size_t test_loops_nb = NB_TEST_LOOP;
    long long seed = (unsigned int) time(NULL);
    // seed = 1693227749;

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
                verbose = true;
            }
        }
    }

    if (verbose)
        printf("Allocating memory\n");
    myToBeSortedArray = calloc(nbVal, sizeof(value_t));
    mySecondToBeSortedArray = malloc(nbVal * sizeof(double));
    myOriginalToBeSortedArray = malloc(nbVal * sizeof(double));
    sortedArray = calloc(nbVal, sizeof(value_t));
    myWorkMem = malloc(nbVal * sizeof(mem_t));
    srand(seed);
    //srand((unsigned int) 42);

    if (verbose)
        printf("Initializing the arrays\n");
    for (size_t i = 0; i < nbVal; ++i) {
        myOriginalToBeSortedArray[i] = ((double) rand() / (double) (RAND_MAX)) * maxVal * 2 - maxVal;
    }

    resetArrays(verbose, mySecondToBeSortedArray, myToBeSortedArray, myOriginalToBeSortedArray, nbVal);
    memset(myWorkMem, 0, nbVal * sizeof(mem_t));

    if (verbose)
        printf("Starting my warming, may the proc be ready!\n");
    for (int i = 0; i < warmup_loops_nb; ++i) {
        resetArrays(verbose, mySecondToBeSortedArray, myToBeSortedArray, myOriginalToBeSortedArray, nbVal);
        memset(myWorkMem, 0, nbVal * sizeof(mem_t));
        my_warmup_pass(myToBeSortedArray, sortedArray, myWorkMem, nbVal);
    }

    if (verbose)
        printf("Starting my measures, may the proc be with me!\n");
    for (int i = 0; i < test_loops_nb; ++i) {
        resetArrays(verbose, mySecondToBeSortedArray, myToBeSortedArray, myOriginalToBeSortedArray, nbVal);
        memset(myWorkMem, 0, nbVal * sizeof(mem_t));
        myElapsed += my_test_pass(myToBeSortedArray, sortedArray, myWorkMem, nbVal) / nbVal;
    }

    if (verbose)
        printf("Starting the warming of the reference, may the proc be ready!\n");
    for (int i = 0; i < warmup_loops_nb; ++i) {
        resetArrays(verbose, mySecondToBeSortedArray, myToBeSortedArray, myOriginalToBeSortedArray, nbVal);
        ref_warmup_pass(mySecondToBeSortedArray, nbVal);
    }

    if (verbose)
        printf("Starting the measure of the ref, may the fastest win!\n");
    for (int i = 0; i < test_loops_nb; ++i) {
        resetArrays(verbose, mySecondToBeSortedArray, myToBeSortedArray, myOriginalToBeSortedArray, nbVal);
        refElapsed += ref_test_pass(mySecondToBeSortedArray, nbVal) / nbVal;
    }

    memset(myWorkMem, 0, nbVal * sizeof(mem_t));
    my_warmup_pass(myToBeSortedArray, sortedArray, myWorkMem, nbVal);

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
        printf("%lf - %lf", refElapsed, myElapsed);
    }

    free(myWorkMem);
    free(myToBeSortedArray);
    free(myOriginalToBeSortedArray);
    free(mySecondToBeSortedArray);
    free(sortedArray);
    return 0;
}
