////////////////////////////////////////////////////////////////////////////////
// Author: Nick Vrvilo (nick.vrvilo@rice.edu)
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Simple CnC example to demonstrate three translator features:
//
// 1) Creating virtual item collection mappings.
// 2) Declaring singleton collections using the "unit" tag.
// 3) Using "@" notation in the $init function's outputs' tag functions
//    to access the init function's arg struct.
//
// The application takes a single positive integer argument N.
// The application computes the sum of the even and odd numbers
// in the range 0 until N, and then prints the results.
////////////////////////////////////////////////////////////////////////////////

// Item collection for holding the integers from 0 until N.
[ int naturals: i ];

////////////////////////////////////////////////////////////////////////////////
// Below we declare two "virtual" item collections.
// A virtual item collection is a mapping from a "virtual"
// item tagspace onto a concrete item collection's tagspace.
// This can be useful for transforming a complex input relationship
// into something that can be specified with simple ranges.
////////////////////////////////////////////////////////////////////////////////

// Virtual item collection for the even entries in naturals.
// Uses an inline tagspace mapping from evens to naturals.
[ int evens: i = naturals: i*2 ];

// Virtual item collection for the odd entries in naturals.
// Uses an external function (declared in the source code)
// to map from the odd to the naturals tagspace.
[ int odds: i = naturals using oddsToNaturals ];

// Singleton collections for holding the even & odd sums
[ int evensTotal: () ];
[ int oddsTotal:  () ];

// The notation @x in a tag expression is expanded by the
// translator into code to access the member x of the
// graph init function's args struct.
( $init: () )
 -> [ naturals: $range(0, @n) ],
    ( sumEvens: @n/2 + @n%2 ),
    ( sumOdds:  @n/2 ); 

( sumEvens: n ) <- [ evens: $range(0, n) ] -> [ evensTotal: () ];
( sumOdds: n ) <- [ odds: $range(0, n) ] -> [ oddsTotal: () ];

( $finalize: () ) <- [ evensTotal: () ], [ oddsTotal: () ];

