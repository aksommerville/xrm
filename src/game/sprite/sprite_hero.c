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
  return 0;
}

/* Gas is applied: move.
 * TODO This is obviously inadequate; there needs to be some continuation of movement after releasing gas.
 */
 
static void hero_move(struct sprite *sprite,double elapsed) {
  const double speed=20.0; // m/s
  double dx=sin(sprite->t)*speed*elapsed;
  double dy=-cos(sprite->t)*speed*elapsed;
  sprite->x+=dx;
  sprite->y+=dy;
}

/* Turn, in response to dpad.
 * TODO For now, we're just rotating immediately, which is definitely not how cars work.
 */
 
static void hero_turn(struct sprite *sprite,double elapsed,int d) {
  const double rate=5.0; // rad/s
  sprite->t+=rate*elapsed*d;
  if (sprite->t>M_PI) sprite->t-=M_PI*2.0;
  else if (sprite->t<-M_PI) sprite->t+=M_PI*2.0;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  //XXX temporary
  // Turn?
  switch (g.input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: hero_turn(sprite,elapsed,-1); break;
    case EGG_BTN_RIGHT: hero_turn(sprite,elapsed,1); break;
  }
  // Move?
  if (g.input&EGG_BTN_SOUTH) hero_move(sprite,elapsed);
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
