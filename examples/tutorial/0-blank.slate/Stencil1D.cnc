// 1-dimensional, 3-point stencil
// CnC-OCR tutorial example

/* comment */
[ float tile[TILE_SIZE]: i, t ];
[ float *fromLeft: i, t ];
[ float *fromRight: i, t ];

( $initialize: () )
 -> [ tile: $range(0, NUM_TILES), 0 ],
    [ fromLeft: $range(1, NUM_TILES), 0 ],
    [ fromRight: $range(0, NUM_TILES-1), 0 ],
    ( stencil: $range(0, NUM_TILES), $range(1, LAST_TIMESTEP+1) );

( stencil: i, t )
    <- [ tile: i, t   ], [ fromLeft: i, t   ], [ fromRight: i, t ]
    -> [ tile: i, t+1 ], [ fromLeft: i+1, t+1 ], [ fromRight: i-1, t+1 ];

( $finalize: () )
    <- [ tile: $range(0, NUM_TILES), LAST_TIMESTEP ];
