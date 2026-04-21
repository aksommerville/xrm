#include "game/xrm.h"

struct sprite_hero {
  struct sprite hdr;
  double bladet; // For chopper.
  int pvwheel;
  double soundclock;
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Cleanup.
 */
 
static void _hero_del(struct sprite *sprite) {
  egg_song_event_note_off(2,0,0x30);
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  vehicle_acquire_config(sprite);
  return 0;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {

  /* Update motor sound.
   * It is technically correct to use (drive) for this: int wheel=(sprite->drive*8191.0)/sprite->topspeed;
   * But (drive) tends to max out and stay there.
   * Using plain old velocity to trigger it feels more natural, and is actually useful to the player.
   */
  double velocity=sqrt(sprite->vx*sprite->vx+sprite->vy*sprite->vy);
  int wheel=(velocity*8191.0)/sprite->topspeed;
  if (wheel<0) wheel=0;
  else if (wheel>8191) wheel=8191;
  if (wheel!=SPRITE->pvwheel) {
    SPRITE->pvwheel=wheel;
    egg_song_event_wheel(2,0,wheel);
  }
  // Need to re-send Note On every so often; the synthesizer has a safety mechanism that drops stale notes.
  if ((SPRITE->soundclock-=elapsed)<=0.0) {
    SPRITE->soundclock+=5.0;
    egg_song_event_note_off(2,0,0x30);
    egg_song_event_note_on(2,0,0x30,0x40);
  }

  if (sprite->vehicle==NS_vehicle_chopper) {
    double dt=sprite->drive/sprite->topspeed;
    dt=dt*20.0+(1.0-dt)*4.0;
    SPRITE->bladet+=dt*elapsed;
    if (SPRITE->bladet>M_PI) SPRITE->bladet-=M_PI*2.0;
  }
  
  if (g.cooldown>0.0) {
    sprite->steer=0;
    sprite->gas=0;
    sprite_vehicle_update(sprite,elapsed);
    return;
  }

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
  int8_t rot=(int8_t)((sprite->t*128.0)/M_PI); // Careful! Must perform the cast signed or Wasm will clamp it.
  graf_fancy(&g.graf,x,y,sprite->tileid,sprite->xform,rot,SPRITE_TILESIZE,sprite->tint,sprite->color);
  switch (sprite->vehicle) {
    case NS_vehicle_boat: {
        //TODO motor and wake
      } break;
    case NS_vehicle_chopper: {
        int bladex=x+lround(sin(sprite->t)*5.0);
        int bladey=y-lround(cos(sprite->t)*5.0);
        int bladerot=(int8_t)((SPRITE->bladet*128.0)/M_PI);
        graf_fancy(&g.graf,bladex,bladey,sprite->tileid+1,0,bladerot,SPRITE_TILESIZE,sprite->tint,sprite->color);
      } break;
  }
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};
