// 1-dimensional, 3-point stencil
// CnC-OCR tutorial example

// Global graph context containing the graph's parameters.
$context {
    int numTiles;
    int tileSize;
    int lastTimestep;
};

// element tiles
[ float tile[#tileSize]: i, t ];

// boundary elements (for exchange)
[ float *fromLeft:  i, t ];
[ float *fromRight: i, t ];

// graph initializer
( $initialize: () )
 -> [ tile:      $range(0, #numTiles),   0 ],
    [ fromLeft:  $range(1, #numTiles),   0 ],
    [ fromRight: $range(0, #numTiles-1), 0 ],
    ( stencil:   $range(0, #numTiles),   1 );

// stencil updater step
( stencil: i, t )
 <- [ tile:      i, t-1 ],
    [ fromLeft:  i, t-1 ] $when(i > 0),
    [ fromRight: i, t-1 ] $when(i+1 < #numTiles)
 -> [ newTile @ tile:      i,   t ],
    [ toRight @ fromLeft:  i+1, t ] $when(fromRight),
    [ toLeft  @ fromRight: i-1, t ] $when(fromLeft),
    ( stencil: i, t+1 ) $when(t < #lastTimestep);

// graph finalizer
( $finalize: () )
 <- [ tile: $range(0, #numTiles), #lastTimestep ];

