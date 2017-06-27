// 1-dimensional, 3-point stencil
// CnC-OCR tutorial example

// element tiles
[ float tile[TILE_SIZE]: i, t ];

// boundary elements (for exchange)
[ float *fromLeft:  i, t ];
[ float *fromRight: i, t ];

// graph initializer
( $initialize: () )
 -> [ tile:      $range(0, NUM_TILES),   0 ],
    [ fromLeft:  $range(1, NUM_TILES),   0 ],
    [ fromRight: $range(0, NUM_TILES-1), 0 ],
    ( stencil:   $range(0, NUM_TILES), $range(1, LAST_TIMESTEP+1) );

// stencil updater step
( stencil: i, t )
 <- [ tile:      i, t-1 ],
    [ fromLeft:  i, t-1 ],
    [ fromRight: i, t-1 ]
 -> [ newTile @ tile:      i,   t ],
    [ toRight @ fromLeft:  i+1, t ],
    [ toLeft  @ fromRight: i-1, t ];

// graph finalizer
( $finalize: () )
 <- [ tile: $range(0, NUM_TILES), LAST_TIMESTEP ];

