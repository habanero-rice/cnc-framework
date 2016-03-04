////////////////////////////////////////////////////////////////////////////////
// Author: Nick Vrvilo (nick.vrvilo@rice.edu)
////////////////////////////////////////////////////////////////////////////////

$context {
    int tw, th;
    int ntw, nth;
};

////////////////////////////////////////////////////////////////////////////////
// item collection declarations

[ int above[] : i, j ];    // Last row of tile above
[ int left[]  : i, j ];    // Last column of tile to the left
[ SeqData *data : () ];    // Static data for the sequences and tile sizes
[ struct timeval *startTime : () ]; // Only used for timing the computation

////////////////////////////////////////////////////////////////////////////////
// Input output relationships

( swStep: i, j )
    <- [ data: () ],
       [ above: i, j ] $when(i > 0),
       [ left: i, j ]  $when(j > 0)
    -> [ below @ above: i+1, j ],
       [ right @ left:  i, j+1 ],
       ( swStep: i+i, j ) $when(i+1 < #nth);

// Write graph inputs and start steps
( $initialize: () )
    -> [ startTime: () ],
       [ data: () ],
       ( swStep: 0, $range(#ntw) );

( $finalize: () )
    <- [ startTime: () ], [ above: #nth, #ntw-1 ];
