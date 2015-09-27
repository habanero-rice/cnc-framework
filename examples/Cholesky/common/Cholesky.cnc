////////////////////////////////////////////////////////////////////////////////
// Alina Sbirlea - alina@rice.edu
// Adapted for CnC-HC after Sagnak Tasirlar's Cholesky for HJ-CnC
// influenced by Aparna Chandramowlishwaran's Intel CnC C++ implementation
// and by Zoran Budimlic's Habanero CnC Java implementation
//
// Updated for CnC-OCR by Nick Vrvilo (nick.vrvilo@rice.edu)
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// graph parameters

$context {
    int numTiles, tileSize, tileCount;
};

////////////////////////////////////////////////////////////////////////////////
// item collection declarations

[ double data[]: j, i, k ];  // The input/intermediate matrix tiles
[ double results[]: i ];     // The result matrix tiles
[ struct timeval *startTime : () ]; // Only used for timing the computation

////////////////////////////////////////////////////////////////////////////////
// Input output relationships (comments are Aparna's )

// The kComputeStep produces 'k' loop indices (in the form of tag instance )
( kComputeStep: () )
 -> ( kjComputeStep: $range(0, #numTiles) ),
    ( sequentialStep: $range(0, #numTiles) );

// The kjCompute step produces 'j' loop indices (in the form of tag instance )
( kjComputeStep: k )
 -> ( kjiComputeStep: k, $range(k+1, #numTiles) ),
    ( trisolveStep:  k, $range(k+1, #numTiles) );

// The kjiComputeStep produces 'i' loop indices (in the form of tag instance )
( kjiComputeStep: k, j ) -> ( updateStep: k, j, $range(k+1, j+1) );

// Step 1 Executions
( sequentialStep: k )
 <- [ data1D @ data: k, k, k ]
 -> [ data: k, k, k+1 ];

// Step 2 Executions
( trisolveStep: k, j )
 <- [ dataA1D @ data: j, k, k ],
    [ dataB1D @ data: k, k, k+1 ]
 -> [ data: j, k, k+1 ];

// Step 3 Executions
( updateStep: k, j, i )
 <- [ dataA1D @ data: j, i, k ],
    [ dataB1D @ data: j, k, k+1 ] $when(i != j),
    [ dataC1D @ data: i, k, k+1 ]
 -> [ data: j, i, k+1 ];

// Input from the caller: tile pointers, tile size and loop end value
( $initialize: () )
 -> [ startTime: () ],
    [ data: $range(tileCount), $range(tileCount), 0 ],
    ( kComputeStep: () );

// Return to the caller
// NOTE: tileCount is the total number of result tiles;
//       numTiles is the number of tiles in the first column of the matrix.
( $finalize: tileCount )
 <- [ startTime: () ],
    [ results: $range(0, tileCount) ];

// NOTE:
// j = matrix tile's row
// i = matrix tile's column
// k = iteration number (up to i+1)
