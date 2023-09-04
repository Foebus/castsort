#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "castsort.h"

#define infinity DBL_MAX

#define RANGE_MAX infinity


void castsort(void * toSort, void *sorted, mem_t *work_mem, size_t nmemb, size_t size, VALFUNC *cast) {
    double maxVal = -infinity;
    double minVal = infinity;
    double deltaVal;

    // Get information about to values to sort
    for (size_t i = 0; i < nmemb; i++) {
        if (cast(&toSort[i * size]) < minVal) {
            minVal = cast(&toSort[i * size]);
        }
        if (cast(&toSort[i * size]) > maxVal) {
            maxVal = cast(&toSort[i * size]);
        }
    }
    deltaVal = maxVal - minVal;

    // In the case where min == max, it means all remaining values are equal
    if (deltaVal == 0) return;

    size_t biggest_collision = 0;
    // Find the nb in each slot
    for (size_t i = 0; i < nmemb; ++i) {
        size_t newIndex = (nmemb - 1) * ((cast(&toSort[i * size]) - minVal) / deltaVal);
        work_mem[newIndex].nb++;
        work_mem[newIndex].act = 0;
        if (work_mem[newIndex].nb > biggest_collision) {
            biggest_collision = work_mem[newIndex].nb;
        }
    }

    // Find start index in final array for each slot
    size_t act = 0;
    for (size_t i = 0; i < nmemb; ++i) {
        work_mem[i].start = act;
        act += work_mem[i].nb;
    }

    // Do the sort
    size_t act_index = 0;
    size_t padding = 0;
    double new_val, last_val = cast(&toSort[act_index * size]);
    for (size_t i = 0; i < nmemb; ++i) {
        size_t slotIndex = (size - 1) * ((cast(&toSort[act_index * size]) - minVal) / deltaVal);
        size_t newIndex = work_mem[slotIndex].start + work_mem[slotIndex].act;
        new_val = cast(&toSort[newIndex * size]);
        if(newIndex == padding) {
            padding++;
            new_val = cast(&toSort[(newIndex + padding) * size]);
        }
        act_index = newIndex + padding;
        memcpy(&sorted[newIndex * size], &last_val, size);
        // (sorted[newIndex]) = last_val;
        last_val = new_val;
        work_mem[slotIndex].act++;
    }

    //Recurse in case of collision
    mem_t *tmp_work_mem;
    if (biggest_collision > 1) {
        tmp_work_mem = calloc(biggest_collision, sizeof(mem_t));
    }
    for (size_t i = 0; i < nmemb; ++i) {
        if (work_mem[i].nb > 1) {
            int nbElem = work_mem[i].nb;
            //memset(work_mem, 0, nbElem*sizeof(mem_t));
            size_t startIndex = work_mem[i].start;
            for (int j = 0; j < nbElem; ++j) {
                memcpy(&sorted[(startIndex + j) * size], &toSort[(startIndex + j) * size], size);
                // toSort[(startIndex + j) * size] = sorted[(startIndex + j) * size];
            }
            castsort(&toSort[startIndex * size], &sorted[startIndex * size], tmp_work_mem, nbElem, size, cast);
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
