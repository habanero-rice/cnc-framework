////////////////////////////////////////////////////////////////////////////////
// Author: Nick Vrvilo (nick.vrvilo@rice.edu)
////////////////////////////////////////////////////////////////////////////////

/* Really simple CnC example:
 * The init function prescribes an "SX" and an "SY" step instance.
 * Each step puts a single integer item to the corresponding item collection.
 * (The value of the integer put matches the step's tag value.)
 * The finalize function reads two two items and prints their values.
 */

[ int X: () ];
[ int Y: () ];

( $initialize: () ) -> ( SX: 1 ), ( SY: 2 );

( SX: x ) -> [ X: () ];
( SY: y ) -> [ Y: () ];

( $finalize: () ) <- [ X: () ], [ Y: () ];

