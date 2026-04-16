#include "game/xrm.h"

struct sprite_hero {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Cleanup.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {

  //TODO Vehicle should be configured generically with a command.
  sprite->vehicle=NS_vehicle_car;
  sprite->grip=0.750;
  sprite->topspeed=25.0; // 30 feels good. 40 is maybe too fast.

  return 0;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  switch (g.input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: sprite->steer=-1; break;
    case EGG_BTN_RIGHT: sprite->steer=1; break;
    default: sprite->steer=0; break;
  }
  if (g.input&EGG_BTN_WEST) sprite->gas=-1;
  else if (g.input&EGG_BTN_SOUTH) sprite->gas=1;
  else sprite->gas=0;
  sprite_vehicle_update(sprite,elapsed);
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int x,int y) {
  //TODO currently unused
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  //.render=_hero_render,
};

/* Public accessors.
 */
 
int sprite_hero_is_offroad(const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_hero)) return 0;
  return 0;//return SPRITE->offroad;
}


double sprite_hero_get_speed(const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_hero)) return 0.0;
  return 0.0;//return SPRITE->report_speed;
}
