#!/bin/bash

LF='\'$'\n'

sed -i.bak '{
s`/. TODO: Do something with data ./`printf("result is %ld\\n", data);`;
}' BigTags.c

sed -i.bak '{
s`/. TODO: Initialize data ./`*data = n;`;
}' BigTags_step.c

rm *.c.bak
