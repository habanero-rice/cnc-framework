#ifndef _CNCOCR_STENCIL1D_TYPES_H_
#define _CNCOCR_STENCIL1D_TYPES_H_

typedef struct Stencil1DArguments {
    /* TODO: Add struct members.
     * Use this struct to pass all arguments for
     * graph initialization. This should not contain any
     * pointers (unless you know you'll only be executing
     * in shared memory and thus passing pointers is safe).
     */
} Stencil1DArgs;

#define NUM_TILES 4
#define TILE_SIZE 16
#define LAST_TIMESTEP 4

#endif /*_CNCOCR_STENCIL1D_TYPES_H_*/
