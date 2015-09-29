#!/bin/bash

LF='\'$'\n'

sed -i.bak '{
s`// int \(lastTile\);`context->\1 = 9;`;
s`// int \(tileWidth\);`context->\1 = 10;`;
s`// int \(numIters\);`context->\1 = 100;`;
}' Main.c

sed -i.bak '{
s`^\( *\)/. TODO: Initialize tile ./`\1int j;'"$LF"'\1for (j=0; j<ctx->tileWidth; j++) tile[j] = 0;`;
s`/. TODO: Initialize \(left\|right\) ./`*\1 = 0;`;
s`void Stencil1D_cncFinalize.*{$`& '"$LF"'    double total = 0;`
s`^\(\(    \)*\)/. TODO: Do something with tile._i. ./`\1int j;'"$LF"'\1for (j=0; j<ctx->tileWidth; j++) total += tile[_i][j];'"$LF"'\2\2}'"$LF"'\2\2{'"$LF"'\1double count = ctx->tileWidth * (ctx->lastTile + 1);'"$LF"'\1printf("avg = %f'"$LF"'", total/count);`
}' Stencil1D.c

sed -i.bak '{
s`#include "Stencil1D.h"$`& '"$LF$LF"'#define STENCIL(left, center, right) (0.5f*(center) + 0.25f*((left) + (right)))`;
s`^\(\(    \)*\)/. TODO: Initialize nextT ./`\1nextT[0] = STENCIL(left ? *left : 1, tile[0], tile[1]);'"$LF"'\1const int lastJ = ctx->tileWidth - 1;'"$LF"'\1int j;'"$LF"'\1for (j=1; j<lastJ; j++) nextT[j] = STENCIL(tile[j-1], tile[j], tile[j+1]);'"$LF"'\1nextT[lastJ] = STENCIL(tile[lastJ-1], tile[lastJ], right ? *right : 1);`;
s`/. TODO: Initialize nextL ./`*nextL = nextT[lastJ];`;
s`/. TODO: Initialize nextR ./`*nextR = nextT[0];`;
}' Stencil1D_stencil.c

rm *.c.bak
