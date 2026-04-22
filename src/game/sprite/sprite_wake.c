/* sprite_wake.c
 * Experimental: Little particle effects that combine to describe a boat's wake.
 */
 
#include "game/xrm.h"

#define TTL 0.500

struct sprite_wake {
  struct sprite hdr;
  double ttl;
  double dx,dy;
  double rx,ry;
};

#define SPRITE ((struct sprite_wake*)sprite)

static int _wake_init(struct sprite *sprite) {
  sprite->layer=90;
  SPRITE->ttl=TTL;
  return 0;
}

int sprite_wake_setup(struct sprite *sprite,double t) {
  if (!sprite||(sprite->type!=&sprite_type_wake)) return -1;
  const double radius=0.750; // m
  const double spread=2.000; // m/2
  double sint=sin(t);
  double cost=cos(t);
  SPRITE->dx=-cost*spread; // Wake moves orthogonal to the boat's travel.
  SPRITE->dy=sint*spread;
  // Add a little noise to the delta, so it doesn't look geometrically perfect.
  SPRITE->dx+=(((rand()&0xffff)-32768)*0.200)/32768.0;
  SPRITE->dy+=(((rand()&0xffff)-32768)*0.200)/32768.0;
  return 0;
}

static void _wake_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->ttl-=elapsed)<=0.0) {
    sprite->defunct=1;
    return;
  }
  SPRITE->rx+=SPRITE->dx*elapsed;
  SPRITE->ry+=SPRITE->dy*elapsed;
}

static void _wake_render(struct sprite *sprite,int x,int y) {
  int dx=lround(SPRITE->rx*NS_sys_tilesize);
  int dy=lround(SPRITE->ry*NS_sys_tilesize);
  int size=(SPRITE->ttl*32.0)/TTL;
  if (size<1) size=1;
  int alpha=SPRITE->ttl*255.0;
  if (alpha<1) alpha=1; else if (alpha>0xff) alpha=0xff;
  uint32_t tint=0x00000000;
  uint32_t primary=0x80808000|alpha;
  graf_fancy(&g.graf,x+dx,y+dy,0x61,0,0,size,tint,primary);
  graf_fancy(&g.graf,x-dx,y-dy,0x61,0,0,size,tint,primary);
}

const struct sprite_type sprite_type_wake={
  .name="wake",
  .objlen=sizeof(struct sprite_wake),
  .init=_wake_init,
  .update=_wake_update,
  .render=_wake_render,
};
