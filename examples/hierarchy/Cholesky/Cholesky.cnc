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
    int numTiles, tileSize;
};

////////////////////////////////////////////////////////////////////////////////
// item collection declarations

// Matrix tiles
[ double MC[]: i ];
[ double MT[]: i, r ];
[ double MU[]: i, r, c ];


////////////////////////////////////////////////////////////////////////////////

// Sequential Cholesky step
( C: i )
 <- [ data1D @ MU: i, i, i ]
 -> [ MC: i+1 ];

// Trisolve step
( T: i, r )
 <- [ dataA1D @ MU: i, r, i ],
    [ dataB1D @ MC: i+1 ]
 -> [ MT: i+1, r ];

// Update step
( U: i, r, c )
 <- [ dataA1D @ MU: i, r, c ],
    [ dataB1D @ MT: i+1, r ], // $when(i != j)
    [ dataC1D @ MT: i+1, c ]
 -> [ MU: i+1, r, c ];

// Input from the caller: tile pointers, tile size and loop end value
( $initialize: () )
 -> [ MU: 0, $range(#numTiles), $range(#numTiles) ],
    ( kComputeStep: () );

// Return to the caller
// NOTE: only returns the bottom-right tile
( $finalize: () ) <- [ MC: #numTiles ];

// NOTE:
// i = iteration number
// r = matrix tile's row
// c = matrix tile's column
