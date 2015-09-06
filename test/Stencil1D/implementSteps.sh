#!/bin/bash

sed -i '{
s`// int \(lastTile\);`context->\1 = 9;`;
s`// int \(tileWidth\);`context->\1 = 10;`;
s`// int \(numIters\);`context->\1 = 100;`;
}' Main.c

sed -i '{
s`^\( *\)/. TODO: Initialize tile ./`\1int j;\n\1for (j=0; j<ctx->tileWidth; j++) tile[j] = 0;`;
s`/. TODO: Initialize \(left\|right\) ./`*\1 = 0;`;
s`void Stencil1D_cncFinalize.*{$`& \n    double total = 0;`
s`^\(\(    \)*\)/. TODO: Do something with tile._i. ./`\1int j;\n\1for (j=0; j<ctx->tileWidth; j++) total += tile[_i][j];\n\2\2}\n\2\2{\n\1double count = ctx->tileWidth * (ctx->lastTile + 1);\n\1printf("avg = %f\\n", total/count);`
}' Stencil1D.c

sed -i '{
s`#include "Stencil1D.h"$`& \n\n#define STENCIL(left, center, right) (0.5f*(center) + 0.25f*((left) + (right)))`;
s`^\(\(    \)*\)/. TODO: Initialize nextT ./`\1nextT[0] = STENCIL(left ? *left : 1, tile[0], tile[1]);\n\1const int lastJ = ctx->tileWidth - 1;\n\1int j;\n\1for (j=1; j<lastJ; j++) nextT[j] = STENCIL(tile[j-1], tile[j], tile[j+1]);\n\1nextT[lastJ] = STENCIL(tile[lastJ-1], tile[lastJ], right ? *right : 1);`;
s`/. TODO: Initialize nextL ./`*nextL = nextT[lastJ];`;
s`/. TODO: Initialize nextR ./`*nextR = nextT[0];`;
}' Stencil1D_stencil.c

