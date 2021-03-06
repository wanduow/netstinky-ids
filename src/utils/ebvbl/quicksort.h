/*
 *
 * Copyright (c) 2020 The University of Waikato, Hamilton, New Zealand.
 *
 * This file is part of netstinky-ids.
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/BSD-2-Clause
 *
 *
 */
/** @file
 * @brief Implements quicksort for an array of items.
 *
 * Items can be of any type as long as a comparison function can be defined
 * for them and they are each of equal length. The user is responsible for
 * that comparison function.
 */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef void *ELEMENT_PTR;
typedef size_t ELEMENT_SZ;  // Size of each element in bytes
typedef int ELEMENT_INDEX;   // Index of element within the array

typedef int (*COMPARE_F_PTR)(ELEMENT_PTR, ELEMENT_PTR);

/**
 * Quicksorts an array. If the user is calling this function lo should be the
 * index of the first element in the array and hi should be the index of the
 * last element in the array.
 * 
 * Behaviour is undefined if the array or the elements at indexes lo or high are
 * out of bounds. This can't be checked by the function.
 * 
 * The sort can fail in a very specific case: there was not enough memory to
 * allocate a temporary variable used while swapping elements. If this occurs
 * the sort cannot be completed and the function will return false.
 * 
 * @param array Address of the first element in the array.
 * @param size  Size of each element in bytes.
 * @param lo    Index of the first element in this partition.
 * @param hi    Index of the last element in this partition.
 * @return      True if the sort succeeded.
 */
bool
quicksort (void *array, ELEMENT_SZ size, COMPARE_F_PTR compare,
           ELEMENT_INDEX lo, ELEMENT_INDEX hi);

/**
 * Using the element at LO as the pivot point, sorts the partition into elements
 * that lie above and elements that are below the pivot point. Returns the first
 * element in the group of elements that are above the pivot point.
 * @param array Address of the array
 * @param size  Size of each element in bytes
 * @param compare   Function that compares two elements
 * @param lo        Index of the first element in this partition
 * @param hi        Index of the last element in this partition
 * @return          True if the sort succeeded
 */
ELEMENT_INDEX
partition (void *array, ELEMENT_SZ size, COMPARE_F_PTR compare,
           ELEMENT_INDEX lo, ELEMENT_INDEX hi);

/**
 * Calculates the offset from a pointer (in bytes) given the size of the
 * elements and the number of elements.
 * @param size          Size of each element in bytes.
 * @param numElements   Number of elements.
 * @return              Offset in bytes.
 */
size_t
calcOffset (ELEMENT_SZ size, size_t numElements);