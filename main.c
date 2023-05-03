#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#define NB_ELEM  100000000
#define infinity 1000000000.0

#define RANGE_MAX infinity

typedef struct mem_st {
    size_t start;
    size_t nb;
    size_t act;
} mem_t;

void my_sort(double *toSort, mem_t *work_mem, double *sorted, size_t size) {
    double maxVal = -infinity;
    double minVal = infinity;
    double deltaVal;

    // Get information about to values to sort
    for (size_t i = 0; i < size; i++) {
        if (toSort[i] < minVal) {
            minVal = toSort[i];
        }
        if (toSort[i] > maxVal) {
            maxVal = toSort[i];
        }
    }
    deltaVal = maxVal - minVal;

    // In the case where min == max, it means all remaining values are equal
    if (deltaVal == 0) return;

    size_t biggest_collision = 0;
    // Find the nb in each slot
    for (size_t i = 0; i < size; ++i) {
        size_t newIndex = (size - 1) * ((toSort[i] - minVal) / deltaVal);
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
    size_t act_index = 0;
    size_t padding = 0;
    double new_val, last_val = toSort[act_index];
    for (size_t i = 0; i < size; ++i) {
        size_t slotIndex = (size - 1) * ((toSort[act_index] - minVal) / deltaVal);
        size_t newIndex = work_mem[slotIndex].start + work_mem[slotIndex].act;
        new_val = toSort[newIndex];
        if(newIndex == padding) {
            padding++;
            new_val = toSort[newIndex + padding];
        }
        act_index = newIndex + padding;
        sorted[newIndex] = last_val;
        last_val = new_val;
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
            for (int j = 0; j < nbElem; ++j) {
                toSort[startIndex + j] = sorted[startIndex + j];
            }
            my_sort(&toSort[startIndex], tmp_work_mem, &sorted[startIndex], nbElem);
            memset(tmp_work_mem, 0, biggest_collision * sizeof(mem_t));
        }
    }
    if (biggest_collision > 1) {
        free(tmp_work_mem);
    }
}

int lower(const void *a, const void *b) {
    return *(double *) a > *(double *) b;
}

void print_arrays(const double *mySecondToBeSortedArray, const double *sortedArray, const size_t nbElem) {
    printf("contents: \n");
    for (size_t j = 0; j < nbElem; ++j) {
        fprintf(stderr, "ref: %lf - my: %lf \n", mySecondToBeSortedArray[j], sortedArray[j]);
    }
}

int main(int argc, char **argv) {

    double *myToBeSortedArray;
    double *mySecondToBeSortedArray;
    double *sortedArray;
    mem_t *myWorkMem;
    struct timeval myBegin, myEnd, refBegin, refEnd;
    double myElapsed, refElapsed;

    bool verbose = true;
    size_t maxVal = RANGE_MAX;
    size_t nbVal = NB_ELEM;
    long long seed = (unsigned int) time(NULL);

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
    myToBeSortedArray = malloc(nbVal * sizeof(double));
    mySecondToBeSortedArray = malloc(nbVal * sizeof(double));
    sortedArray = malloc(nbVal * sizeof(double));
    myWorkMem = malloc(nbVal * sizeof(mem_t));
    srand(seed);
    //srand((unsigned int) 42);

    if (verbose)
        printf("Initializing the arrays\n");
    for (size_t i = 0; i < nbVal; ++i) {
        myToBeSortedArray[i] = ((double) rand() / (double) (RAND_MAX)) * maxVal * 2 - maxVal;
        mySecondToBeSortedArray[i] = myToBeSortedArray[i];
    }
    memset(myWorkMem, 0, nbVal * sizeof(mem_t));

    if (verbose)
        printf("Starting the race, may the faster win!\n");

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
        if (mySecondToBeSortedArray[i] != sortedArray[i]) {
            if (verbose) {
                printf("The arrays don't have the same values!\n");
                print_arrays(mySecondToBeSortedArray, sortedArray, nbVal);
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
