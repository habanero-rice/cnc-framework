#!/bin/bash

sed -i '{
s`/. TODO: Do something with data ./`printf("result is %ld\\n", data);`;
}' BigTags.c

sed -i '{
s`/. TODO: Initialize data ./`*data = n;`;
}' BigTags_step.c
