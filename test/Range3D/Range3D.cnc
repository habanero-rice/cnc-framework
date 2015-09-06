[ int data: i, j, k, l ];
[ int sum: () ];

( $init: () ) -> [ data: $range(5), $rangeTo(4), $range(5), $range(5) ], ( summing: () );
( summing: () ) <- [ data: $range(2), $rangeTo(4), 2, $range(4, 5) ] -> [ sum: () ];
( $finalize: () ) <- [ sum: () ];
