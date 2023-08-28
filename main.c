#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
// #include <qcustomplot.h>
#include "castsort.h"

#define NB_ELEM  100000000
#define infinity 1000000000.0

#define RANGE_MAX infinity

double transform_value(double x, double min, double max, double delta){
    // return x;
    x = (x - min)/(max - min);
    // if (delta > 1) delta = 1/delta;
    double sig = 1/((1 + exp(-x)) * delta);
    return sig;
}

void my_sort(value_t *toSort, mem_t *work_mem, double *sorted, size_t size) {
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
        size_t newIndex = (size - 1) * ((toSort[i].transformed_val - minVal) / deltaVal);
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
        size_t slotIndex = (size - 1) * ((toSort[i].transformed_val - minVal) / deltaVal);
        size_t newIndex = work_mem[slotIndex].start + work_mem[slotIndex].act;
        toSort[i].final_index = newIndex;
        sorted[newIndex] = toSort[i].origin_val;
        if (work_mem[slotIndex].max < toSort[i].origin_val || work_mem[slotIndex].act == 0) work_mem[slotIndex].max = toSort[i].origin_val;
        if (work_mem[slotIndex].min > toSort[i].origin_val || work_mem[slotIndex].act == 0) work_mem[slotIndex].min = toSort[i].origin_val;
        work_mem[slotIndex].act++;
    }

    //Recurse in case of collision
    mem_t *tmp_work_mem;
    if (biggest_collision > 1) {
        tmp_work_mem = calloc(biggest_collision, sizeof(mem_t));
    }
    for (size_t i = 0; i < size; ++i) {
        if (work_mem[i].nb > 1) {
            int nbElem = work_mem[i].nb;
            //memset(work_mem, 0, nbElem*sizeof(mem_t));
            size_t startIndex = work_mem[i].start;
            double delta = work_mem[i].max - work_mem[i].min;
            if(work_mem[i].max - work_mem[i].min == 0) continue;
            for (int j = 0; j < nbElem; ++j) {
                toSort[startIndex + j].transformed_val = transform_value([startIndex + j], work_mem[i].min, work_mem[i].max, delta);
                toSort[startIndex + j].origin_val = sorted[startIndex + j];
            }
            my_sort(&toSort[startIndex], tmp_work_mem, &sorted[startIndex], nbElem);
            memset(tmp_work_mem, 0, biggest_collision * sizeof(mem_t));
        }
    }
    if (biggest_collision > 1) {
        free(tmp_work_mem);
    }
}

void print_arrays(const double *mySecondToBeSortedArray, const double *sortedArray, const size_t nbElem) {
    printf("contents: \n");
    for (size_t j = 0; j < nbElem; ++j) {
        if (mySecondToBeSortedArray[j] !=  sortedArray[j])
            fprintf(stderr, "ref: %lf - my: %lf \n", mySecondToBeSortedArray[j], sortedArray[j]);
    }
}

int main(int argc, char **argv) {

    value_t *myToBeSortedArray;
    double *mySecondToBeSortedArray;
    double *sortedArray;
    mem_t *myWorkMem;
    struct timeval myBegin, myEnd, refBegin, refEnd;
    double myElapsed, refElapsed;

    bool verbose = true;
    size_t maxVal = RANGE_MAX;
    size_t nbVal = NB_ELEM;
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
            if (argc > 4) {
                verbose = true;
            }
        }
    }

    if (verbose)
        printf("Allocating memory\n");
    myToBeSortedArray = calloc(nbVal, sizeof(value_t));
    mySecondToBeSortedArray = malloc(nbVal * sizeof(double));
    sortedArray = malloc(nbVal * sizeof(double));
    myWorkMem = malloc(nbVal * sizeof(mem_t));
    srand(seed);
    //srand((unsigned int) 42);

    if (verbose)
        printf("Initializing the arrays\n");
    for (size_t i = 0; i < nbVal; ++i) {
        myToBeSortedArray[i].origin_val = ((double) rand() / (double) (RAND_MAX)) * maxVal * 2 - maxVal;
        myToBeSortedArray[i].transformed_val = myToBeSortedArray[i].origin_val;
        myToBeSortedArray[i].final_index = i;
        mySecondToBeSortedArray[i] = myToBeSortedArray[i].origin_val;
    }
    memset(myWorkMem, 0, nbVal * sizeof(mem_t));

    if (verbose)
        printf("Starting the race, may the fastest win!\n");

    gettimeofday(&myBegin, 0);
    my_sort(myToBeSortedArray, myWorkMem, sortedArray, nbVal);
    gettimeofday(&myEnd, 0);
    myElapsed = myEnd.tv_sec - myBegin.tv_sec + (myEnd.tv_usec - myBegin.tv_usec) * 1e-6;

    gettimeofday(&refBegin, 0);
    qsort(mySecondToBeSortedArray, nbVal, sizeof(double), lower);
    gettimeofday(&refEnd, 0);
    refElapsed = refEnd.tv_sec - refBegin.tv_sec + (refEnd.tv_usec - refBegin.tv_usec) * 1e-6;

    if (verbose)
        printf("Checking results\n");
    for (size_t i = 0; i < nbVal; ++i) {
        if (fabs(mySecondToBeSortedArray[i] - sortedArray[i]) > exp(-6)) {
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
    if (verbose) {
        printf("Results are the same\n");
        printf("And the winner is...");
        if (refElapsed > myElapsed) {
            printf("Me!");
        } else {
            printf("The reference");
        }
        printf("With ref: %lf and my time: %lf", refElapsed, myElapsed);
    } else {
        printf("%lf - %lf", refElapsed, myElapsed);
    }
    return 0;
}
