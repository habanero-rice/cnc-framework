{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

#include "cnc_common.h"

void *_cncRangedInputAlloc(u32 n, u32 dims[], size_t itemSize, void **dataStartPtr) {
    u32 i, j, k;
    ///////////////////////////////////////
    // Figure out how much memory we need
    ///////////////////////////////////////
    u32 sum = 0, product = 1;
    for (i=0; i<n-1; i++) {
        product *= dims[i];
        sum += sizeof(void*) * product;
    }
    product *= dims[i];
    sum += itemSize * product;
    ///////////////////////////////////////
    // Allocate a block of memory
    ///////////////////////////////////////
    void **ptrs = cncLocalAlloc(sum);
    ///////////////////////////////////////
    // Set up the internal pointers
    ///////////////////////////////////////
    if (n > 1) {
        u32 prevDim = 1, currDim = 1, nextDim = dims[0];
        // Set up the pointers-to-pointers
        void **current = ptrs;
        void **tail = ptrs + nextDim; // make room for first array
        for (i=1; i<n-1; i++) {
            // Slide the window
            prevDim = currDim;
            currDim = nextDim;
            nextDim = dims[i];
            // One array for each parent
            for (j=0; j<prevDim; j++) {
                // Initialize each current pointer
                for (k=0; k<currDim; k++) {
                    *current = (void*)tail;
                    tail += nextDim; // Make room for new array
                    ++current;
                }
            }
        }
        // Save start of actual data's memory
        *dataStartPtr = tail;
        // Set up the pointers-to-data
        u8 **itemCurrent = (u8**)current;
        u8 *itemTail = (u8*)tail;
        // Slide the window
        prevDim = currDim;
        currDim = nextDim;
        nextDim = dims[i];
        // One array for each last-level parent
        for (j=0; j<prevDim; j++) {
            // Initialize each current pointer
            for (k=0; k<currDim; k++) {
                *itemCurrent = itemTail;
                itemTail += itemSize * nextDim; // Make room for new items
                ++itemCurrent;
            }
        }
        assert(itemTail == ((u8*)ptrs + sum));
    }
    else {
        // Save start of actual data's memory
        // (which is just the start of the block in this case)
        *dataStartPtr = ptrs;
    }
    ///////////////////////////////////////////
    // Return the initialized block of items
    ///////////////////////////////////////////
    return ptrs;
}

