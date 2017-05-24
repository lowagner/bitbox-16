#ifndef HIT_H
#define HIT_H
#include "sprites.h"

void sprite_collide(struct object *o, struct object *p);
void object_run_commands(int i);
int damage_sprite(struct object *o, float damage);

#endif
