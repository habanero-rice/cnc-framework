////////////////////////////////////////////////////////////////////////////////
// Author: Nick Vrvilo (nick.vrvilo@rice.edu)
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Test the following features:
//
// 1) $context
// 2) vector types (using [])
// 3) $when clauses for input and output
////////////////////////////////////////////////////////////////////////////////

$context {
    int lastTile;
    int tileWidth;
    int numIters;
};

// tile at index i, iteration (timestep) t
[ float tile[#tileWidth]: i, t ];

// left-hand border element of [ tile: i-1, t ]
[ float *right: i, t ];
[ float *left:  i, t ];

( $initialize: () )
 -> [ tile:    $rangeTo(#lastTile), 0 ],
    [ left:    $rangeTo(#lastTile), 0 ],
    [ right:   $rangeTo(#lastTile), 0 ],
    ( stencil: $rangeTo(#lastTile), 0 );

( stencil: i, t )
 <- [ tile:  i, t ],
    [ left:  i, t ] $when(i > 0),
    [ right: i, t ] $when(i < #lastTile)
 -> [ nextT @ tile:  i,   t+1 ],
    [ nextL @ left:  i+1, t+1 ] $when(i < #lastTile),
    [ nextR @ right: i-1, t+1 ] $when(i > 0),
    ( stencil: i, t+1 ) $when(t+1 < #numIters);

( $finalize: () )
 <- [ tile: $rangeTo(#lastTile), #numIters ];

