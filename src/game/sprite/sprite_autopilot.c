#include "game/xrm.h"

struct sprite_autopilot {
  struct sprite hdr;
  double bladet; // For chopper.
  int planp;
};

#define SPRITE ((struct sprite_autopilot*)sprite)

/* Cleanup.
 */
 
static void _autopilot_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _autopilot_init(struct sprite *sprite) {
  vehicle_acquire_config(sprite);
  return 0;
}

/* Update.
 */
 
static void _autopilot_update(struct sprite *sprite,double elapsed) {

  if (sprite->vehicle==NS_vehicle_chopper) {
    double dt=sprite->drive/sprite->topspeed;
    dt=dt*20.0+(1.0-dt)*4.0;
    SPRITE->bladet+=dt*elapsed;
    if (SPRITE->bladet>M_PI) SPRITE->bladet-=M_PI*2.0;
  }
  
  if (g.planc>0) {
    if ((SPRITE->planp<0)||(SPRITE->planp>=g.planc)) SPRITE->planp=0;
    const struct plan *plan=g.planv+SPRITE->planp;
    
    // If we're within a meter of the target point, advance to the next and skip decision-making this frame.
    double dx=plan->x-sprite->x;
    double dy=plan->y-sprite->y;
    double d2=dx*dx+dy*dy;
    if (d2<=1.0) {
      SPRITE->planp++;
    } else {
      
      // Orient toward the target.
      const double T_STEER_ON =M_PI*0.100; // Begin turning if we're so far off kilter.
      const double T_STEER_OFF=M_PI*0.020; // Stop turning when we're so close to the target.
      const double T_GAS_OFF  =M_PI*0.400; // Accelerate only when so close to target.
      double ttarget=atan2(dx,-dy);
      //fprintf(stderr,"t=%f ttarget=%f at=%f,%f toward=%f,%f\n",sprite->t,ttarget,sprite->x,sprite->y,plan->x,plan->y);
      double dt=ttarget-sprite->t;
      if (dt>M_PI) dt-=M_PI*2.0;
      else if (dt<-M_PI) dt+=M_PI*2.0;
      if (sprite->steer) {
        if (dt<-T_STEER_OFF) sprite->steer=-1;
        else if (dt>T_STEER_OFF) sprite->steer=1;
        else sprite->steer=0;
      } else {
        if (dt<-T_STEER_ON) sprite->steer=-1;
        else if (dt>T_STEER_ON) sprite->steer=1;
        else sprite->steer=0;
      }
      
      // If we're looking at it, accelerate.
      if ((dt>-T_GAS_OFF)&&(dt<T_GAS_OFF)) {
        sprite->gas=1;
      } else {
        sprite->gas=0;
      }
    }
  }
  
  sprite_vehicle_update(sprite,elapsed);
}

/* Render.
 */
 
static void _autopilot_render(struct sprite *sprite,int x,int y) {
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
 
const struct sprite_type sprite_type_autopilot={
  .name="autopilot",
  .objlen=sizeof(struct sprite_autopilot),
  .del=_autopilot_del,
  .init=_autopilot_init,
  .update=_autopilot_update,
  .render=_autopilot_render,
};
