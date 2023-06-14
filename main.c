#include <math.h>
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
    bool mean_ready;
    double mean;
    double std_deviation;
    size_t nb;
    size_t act;
} mem_t;

typedef struct value_analysis_st {
    double above_val;
    double beneath_val;
    double new_val;
    double final_index;
    const double base_val;
} value_analysis_t;

double sigmoid(double x, double mean, double std_dev){
    double ret_val = 1/(1 + exp(1/std_dev * (x - mean)));
    return ret_val;
}

void my_sort(value_analysis_t *toSort, mem_t *work_mem, double *sorted, size_t size) {
    double maxVal = -infinity;
    double minVal = infinity;
    double deltaVal;
    double mean = 0;
    double std_dev = 0.00000001;

    // Get information about to values to sort
    for (size_t i = 0; i < size; i++) {
        if (toSort[i].new_val < minVal) {
            minVal = toSort[i].new_val;
        }
        if (toSort[i].new_val > maxVal) {
            maxVal = toSort[i].new_val;
        }
    }
    deltaVal = maxVal - minVal;

    // In the case where min == max, it means all remaining values are equal
    if (deltaVal == 0) return;

    size_t biggest_collision = 0;
    // Find the nb in each slot
    for (size_t i = 0; i < size; ++i) {
        size_t newIndex = (size - 1) * ((toSort[i].new_val - minVal) / deltaVal);
        work_mem[newIndex].nb++;
        work_mem[newIndex].act = 0;
        work_mem[newIndex].mean += toSort->new_val;
        if (work_mem[newIndex].nb > biggest_collision) {
            biggest_collision = work_mem[newIndex].nb;
        }
    }

    // Find start index in final array for each slot
    size_t act = 0;
    for (size_t i = 0; i < size; ++i) {
        work_mem[i].start = act;
        act += work_mem[i].nb;
        if(!work_mem[i].mean_ready){
            work_mem[i].mean_ready = true;
            if(work_mem[i].nb != 0)
                work_mem[i].mean /= work_mem[i].nb;
        }
    }

    for (size_t i = 0; i < size; ++i) {
        size_t newIndex = (size - 1) * ((toSort[i].new_val - minVal) / deltaVal);
        work_mem[newIndex].std_deviation += (toSort[i].new_val - work_mem[i].mean) * (toSort[i].new_val - work_mem[i].mean) / work_mem[newIndex].nb;
    }

    // Do the sort
    size_t act_index = 0;
    size_t padding = 0;
    double new_val, last_val = toSort[act_index].new_val;
    for (size_t i = 0; i < size; ++i) {
        size_t slotIndex = (size - 1) * ((toSort[act_index].new_val - minVal) / deltaVal);
        size_t newIndex = work_mem[slotIndex].start + work_mem[slotIndex].act;
        toSort[act_index].final_index = newIndex;
        toSort[act_index].new_val = sigmoid(toSort[act_index].new_val, work_mem[slotIndex].mean, work_mem[slotIndex].std_deviation);
        new_val = toSort[newIndex].new_val;
        if(newIndex == padding) {
            padding++;
            new_val = toSort[newIndex + padding].new_val;
        }
        toSort[newIndex].final_index = newIndex;
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
                toSort[startIndex + j].new_val = sorted[startIndex + j];
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
