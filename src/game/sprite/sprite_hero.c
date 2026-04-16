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

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
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
