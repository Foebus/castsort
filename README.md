
# Algo explanation

The algorithm is close to the [proxmap](https://en.wikipedia.org/wiki/Proxmap_sort) sort. The main difference is the approach and the function used to determine the new position of each value.

It can sort any kind of value as long as it can be translated to double (or float or any, we actually need to have three operations: addition, substraction and division).

It does a first pass to analyze the values to sort. It extracts the min val and the max val.

If the delta between min and max is equal to 0, the values left are the same so we don't need to reorder, so we are done.

Then we do a second pass to prepare the slots to sort the array.
For that, we count the number of collision we will encounter, to attribute the right sizes to the different slots in the final array.

A slot is a logical part of the array where all values collide. 
The number of slots must be O(n) to keep the memory usage complexity.

This will allow us to know where each slot begins in the actual array.
We store those sizes in a dedicated array. To know where the value will be sent, we do the operation we will do in the next pass: nmemb * (val - min) / (max - min).
This operation has the good property to be fast to compute. 
To reduce the risk of collision, for the recursion, the value is transformed using a sigmoid function to modify as much as possible the closest values.
The code doing this transformation is 
```
    x = (x - min)/(max - min);
    if (delta > 1) delta = 1/delta;
    double sig = 1/((1 + exp(-x)) * delta);
    return sig;
```
Or even better is to center the points in the function. To do that, the goal is to take a value close to most of the points, ignoring the outliers. 
The best value to get is the median, but it is hard to compute. A solution is to take a random value, statistically it is not so bad (as it is used in QuickSort).
The function then becomes 
```
    x = (x - median)/(delta);
    if (delta > 1) delta = 1/delta;
    double sig = 1/((1 + exp(-x)) * delta);
    return sig;
```
And it's great because now the sigmoid is centered around the median, so if some points are very close and around the middle these will be sent far from one another and far points will be flattened.

Once this analysis is done, we do one last pass, placing the values to the corresponding location in each slot.

Then, for each slot with more than one value, we recurse in the specific slot.

## Example on dummy array

```
┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐
│ 62││-12││ 15││ 45││  0││ 30││ 25││  3││ 25││ 50││  5││100││ 10│
└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘
```
First pass => min = -12, max = 100

Second pass =>
```
 -12  -3.4   5.2    13.8   22.4   31.6   40.2    49    57.6   66.2   74.8    83.4  91.4   100
 ┌─────┐┌─────┐┌─────┐┌─────┐┌─────┐┌─────┐┌─────┐┌─────┐┌─────┐┌─────┐┌─────┐┌─────┐┌─────┐
 │  1  ││  3  ││  1  ││  1  ││  3  ││  0  ││    1││    1││    1││    0││    0││    0││    1│
 └─────┘└─────┘└─────┘└─────┘└─────┘└─────┘└─────┘└─────┘└─────┘└─────┘└─────┘└─────┘└─────┘
```
 Then do the sort =>
```
┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐
│-12││  0││  3││  5││ 10││ 15││ 30││ 25││ 25││ 45││ 50││ 62││100│
└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘
```

Then we recurse for the sub arrays of size > 1
```
┌───┐┌───┐┌───┐
│  0││  3││  5│
└───┘└───┘└───┘
┌───┐┌───┐┌───┐
│ 30││ 25││ 25│
└───┘└───┘└───┘
```
Here, both will be sorted fast. The first won't change but the second will need one additional recursion:

```
┌───┐┌───┐┌───┐
│ 30││ 25││ 25│
└───┘└───┘└───┘
```
Min = 25, max = 30 and distribution array:
```
┌───┐┌───┐┌───┐
│  2││  0││  1│
└───┘└───┘└───┘
```
Results:
```
┌───┐┌───┐┌───┐
│ 25││ 25││ 30│
└───┘└───┘└───┘
```
And here the first slot has 2 values identical, so it will recurse but stop after the first pass as min == max

# Memory usage

This algorithm needs memory to store the actual offset in each slot for the next insertion. 
The required quantity for the first version of this algorithm is of O(N) in the worst case, but it probably could be lowered by a smarter way to store the values.

In practice, with the default values given in this code, it will need around 8Go of memory. 
This makes sense in the context of this proof of concept, as there are 5 arrays, for a total of 10 values stored for each value to sort, and each value has 8 bytes.
So the code require 80 byte per value we want to sort, and the default number is 100 million. 

# Time complexity

This algorithm has a time complexity a bit hard to compute as it depends on the values, and thus the way those values are represented.
Most of the time, the time complexity is of order of n, if the values create order of n collisions, the complexity may be higher.
To have a time complexity higher than O(n), the values must be in the same slot in a significant way recursively. 
To have a complexity of O(n*s), we thus need to have values recursively close s times. 
Depending on the representation of the values, this value may be limited.

# Parallelism

As this algorithm only focuses on the values, it is completely possible to fully parallelize the process. 
The interaction between the threads will occur in the writing of the extracted values (min and max), the number of values in each slot and in the update of the offset while the sorting occurs.
If the algorithm is highly parallelized, the stability property is lost.

# Big O 

```
                 ┌───────────────────────┐┌────────────────────┐
                 │comparisons            ││swap memory         │
┌───────────────┐├───────┬───────┬───────┤├──────┬──────┬──────┤┌──────┐┌─────────┐┌─────────┐┌─────────┐
│name           ││min    │avg    │max    ││min   │avg   │max   ││stable││partition││adaptive ││compares │
├───────────────┤├───────┼───────┼───────┤├──────┼──────┼──────┤├──────┤├─────────┤├─────────┤├─────────┤
│blitsort       ││n      │n log n│n log n││1     │1     │1     ││yes   ││yes      ││yes      ││yes      │
├───────────────┤├───────┼───────┼───────┤├──────┼──────┼──────┤├──────┤├─────────┤├─────────┤├─────────┤
│crumsort       ││n      │n log n│n log n││1     │1     │1     ││no    ││yes      ││yes      ││yes      │
├───────────────┤├───────┼───────┼───────┤├──────┼──────┼──────┤├──────┤├─────────┤├─────────┤├─────────┤
│fluxsort       ││n      │n log n│n log n││n     │n     │n     ││yes   ││yes      ││yes      ││yes      │
├───────────────┤├───────┼───────┼───────┤├──────┼──────┼──────┤├──────┤├─────────┤├─────────┤├─────────┤
│gridsort       ││n      │n log n│n log n││n     │n     │n     ││yes   ││yes      ││yes      ││yes      │
├───────────────┤├───────┼───────┼───────┤├──────┼──────┼──────┤├──────┤├─────────┤├─────────┤├─────────┤
│quadsort       ││n      │n log n│n log n││1     │n     │n     ││yes   ││no       ││yes      ││yes      │
├───────────────┤├───────┼───────┼───────┤├──────┼──────┼──────┤├──────┤├─────────┤├─────────┤├─────────┤
│wolfsort       ││n      │n log n│n log n││n     │n     │n     ││yes   ││yes      ││yes      ││hybrid   │
├───────────────┤├───────┼───────┼───────┤├──────┼──────┼──────┤├──────┤├─────────┤├─────────┤├─────────┤
│rhsort         ││n      │n log n│n log n││n     │n     │n     ││yes   ││yes      ││semi     ││hybrid   │
├───────────────┤├───────┼───────┼───────┤├──────┼──────┼──────┤├──────┤├─────────┤├─────────┤├─────────┤
│skasort_copy   ││n k    │n k    │n k    ││n     │n     │n     ││yes   ││yes      ││no       ││no       │
├───────────────┤├───────┼───────┼───────┤├──────┼──────┼──────┤├──────┤├─────────┤├─────────┤├─────────┤
│castsort       ││0      │0      │0      ││n     │n     │n^2   ││yes   ││?        ││?        ││no       │
└───────────────┘└───────┴───────┴───────┘└──────┴──────┴──────┘└──────┘└─────────┘└─────────┘└─────────┘

```
It is to know that the worst case has an extremely low probability to occur,
as the distribution of the values would have to be recursively of the same repartition: 
one very far from the others and all others VERY close.

# Benchmark

In progress