/* sprite_finishline.c
 * Decoration created by race.c, to mark the backward edge of checkpoint zero, ie where the race ends.
 */

#include "game/xrm.h"

struct sprite_finishline {
  struct sprite hdr;
  int dx,dy; // (0,NS_sys_tilesize) or (NS_sys_tilesize,0), the step between our tiles.
  int tilec; // How many full tiles.
  int plushalf; // If our size is odd, which is typical, draw another tile halfway past the end.
};

#define SPRITE ((struct sprite_finishline*)sprite)

/* Init.
 * We're created with no context, not even a position. So not much to do here.
 */

static int _finishline_init(struct sprite *sprite) {
  sprite->layer=10;
  sprite->tileid=0x60;
  return 0;
}

/* Setup: The real "init".
 */
 
int sprite_finishline_setup(struct sprite *sprite,const struct checkpoint *checkpoint,uint8_t orient) {
  if (!sprite||(sprite->type!=&sprite_type_finishline)) return -1;
  if (!checkpoint) return -1;
  switch (orient) {
    case 0x40: {
        sprite->x=checkpoint->x+1.0;
        sprite->y=checkpoint->y+checkpoint->h-0.5;
        SPRITE->tilec=checkpoint->w>>1;
        SPRITE->plushalf=checkpoint->w&1;
        SPRITE->dx=NS_sys_tilesize<<1;
        sprite->xform=EGG_XFORM_SWAP;
      } break;
    case 0x02: {
        sprite->x=checkpoint->x+1.0;
        sprite->y=checkpoint->y+0.5;
        SPRITE->tilec=checkpoint->w>>1;
        SPRITE->plushalf=checkpoint->w&1;
        SPRITE->dx=NS_sys_tilesize<<1;
        sprite->xform=EGG_XFORM_SWAP;
      } break;
    case 0x10: {
        sprite->x=checkpoint->x+checkpoint->w-0.5;
        sprite->y=checkpoint->y+1.0;
        SPRITE->tilec=checkpoint->h>>1;
        SPRITE->plushalf=checkpoint->h&1;
        SPRITE->dy=NS_sys_tilesize<<1;
      } break;
    case 0x08: {
        sprite->x=checkpoint->x+0.5;
        sprite->y=checkpoint->y+1.0;
        SPRITE->tilec=checkpoint->h>>1;
        SPRITE->plushalf=checkpoint->h&1;
        SPRITE->dy=NS_sys_tilesize<<1;
      } break;
    default: return -1;
  }
  return 0;
}

/* Render.
 */

static void _finishline_render(struct sprite *sprite,int x,int y) {
  int i=SPRITE->tilec;
  for (;i-->0;x+=SPRITE->dx,y+=SPRITE->dy) {
    graf_tile(&g.graf,x,y,sprite->tileid,sprite->xform);
  }
  if (SPRITE->plushalf) {
    x-=SPRITE->dx>>1;
    y-=SPRITE->dy>>1;
    graf_tile(&g.graf,x,y,sprite->tileid,sprite->xform);
  }
}

/* Type definition.
 */

const struct sprite_type sprite_type_finishline={
  .name="finishline",
  .objlen=sizeof(struct sprite_finishline),
  .init=_finishline_init,
  .render=_finishline_render,
};
