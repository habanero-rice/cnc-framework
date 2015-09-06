#!/bin/bash

sed -i '{
s`/. TODO: Initialize data ./`*data = _i + _j * (_k + _l);`;
s`/. TODO: Do something with sum ./`printf("Sum is %d\\n", sum);`;
}' Range3D.c

sed -i '{
s`.*{ // Access "data" inputs$`    int total = 0;\n& `;
s`/. TODO: Do something with \(data._i.._j.._l.\) ./`total += \1;`;
s`/. TODO: Initialize sum ./`*sum = total;`;
}' Range3D_summing.c

