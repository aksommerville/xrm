#include "game/xrm.h"

// If we don't travel so far (squared) in so long, take a closer look at the situation.
#define SAFETY_INTERVAL   1.000
#define SAFETY_DISTANCE_2 1.000

struct sprite_autopilot {
  struct sprite hdr;
  double bladet; // For chopper.
  int planp;
  
  // Sample my effective travel every so often, and if we don't seem to be making progress, take measures.
  double safetyclock;
  double safetyx,safetyy;
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
  SPRITE->safetyclock=SAFETY_INTERVAL;
  SPRITE->safetyx=sprite->x;
  SPRITE->safetyy=sprite->y;
  
  /* Our tiles are made for color replacement.
   * Make up a random color.
   */
  uint8_t r=0x40+(rand()&0x7f);
  uint8_t g=0x40+(rand()&0x7f);
  uint8_t b=0x40+(rand()&0x7f);
  sprite->color=(r<<24)|(g<<16)|(b<<8)|0xff;
  
  return 0;
}

/* Update.
 */
 
static void _autopilot_update(struct sprite *sprite,double elapsed) {

  // Animation.
  if (sprite->vehicle==NS_vehicle_chopper) {
    double dt=sprite->drive/sprite->topspeed;
    dt=dt*20.0+(1.0-dt)*4.0;
    SPRITE->bladet+=dt*elapsed;
    if (SPRITE->bladet>M_PI) SPRITE->bladet-=M_PI*2.0;
  }
  
  // Countdown? Hold.
  if (g.countdown>0.0) return;
  
  // At each cycle of my safetyclock, ensure we've travelled a sensible distance.
  if ((SPRITE->safetyclock-=elapsed)<=0.0) {
    SPRITE->safetyclock+=SAFETY_INTERVAL;
    double dx=sprite->x-SPRITE->safetyx;
    double dy=sprite->y-SPRITE->safetyy;
    SPRITE->safetyx=sprite->x;
    SPRITE->safetyy=sprite->y;
    if (dx*dx+dy*dy>=SAFETY_DISTANCE_2) {
      // We're moving, it's cool.
    } else {
      // We're stuck! Review the entire plan, and find the nearest line.
      // If it happens to be the one we're looking at already, well that sucks, I don't know what else to do.
      int bestp=0;
      double bestdistance=999.0;
      int planp=0;
      for (;planp<g.planc;planp++) {
        double distance=vehicle_distance_to_segment(sprite,planp);
        if (distance<bestdistance) {
          bestp=planp;
          bestdistance=distance;
        }
      }
      SPRITE->planp=bestp;
    }
  }
  
  if (g.planc>0) {
    if ((SPRITE->planp<0)||(SPRITE->planp>=g.planc)) SPRITE->planp=0;
    
    /* (planp) is the start of the segment we're tracking.
     * At each frame, compare my distance to that segment, and the next one.
     * When the next one is nearer, advance (planp).
     * Actually, not just nearer, add a slight bias toward next.
     */
    const double advance_bias=1.0; // m
    double distance_here=vehicle_distance_to_segment(sprite,SPRITE->planp);
    double distance_next=vehicle_distance_to_segment(sprite,SPRITE->planp+1);
    if (distance_next<distance_here+advance_bias) {
      SPRITE->planp++;
      if (SPRITE->planp>=g.planc) SPRITE->planp=0;
      distance_here=distance_next;
    }
    double ax,ay,bx,by; // (a) is the backward point and (b) forward, of the segment we're tracking.
    vehicle_get_segment(&ax,&ay,&bx,&by,SPRITE->planp);
    
    /* If we're beyond some distance from the line, steer toward it, at some angle between perpendicular and forward.
     * But if we're within the barrel, aim parallel and forward. (not at the endpoint exactly; just "forward").
     */
    const double barrel_radius=1.0; // m
    double ttarget;
    if (distance_here>barrel_radius) { // Off track, get closer.
      ttarget=vehicle_angle_toward_line(ax,ay,bx,by,sprite->x,sprite->y,M_PI*0.250);
    } else { // On track, drive forward.
      ttarget=vehicle_angle_toward_line(ax,ay,bx,by,sprite->x,sprite->y,0.0);
    }
    
    /* Set (steer) to approach (ttarget).
     * Doesn't have to be exact; a wee tolerance so we're not constantly jittering.
     * Jitter wouldn't matter visually, but there might be penalties for oversteering.
     * Also, different thresholds for beginning to steer and ending. Again, to reduce jitter.
     */
    const double steer_tolerance_stop =M_PI*0.010; // Stop at this close, when we're already steering.
    const double steer_tolerance_start=M_PI*0.020; // Begin steering when this far off.
    double dt=ttarget-sprite->t;
    if (dt>M_PI) dt-=M_PI*2.0;
    else if (dt<-M_PI) dt+=M_PI*2.0;
    if (sprite->steer) {
      if ((dt>=-steer_tolerance_stop)&&(dt<=steer_tolerance_stop)) sprite->steer=0;
      else if (dt>0.0) sprite->steer=1;
      else if (dt<0.0) sprite->steer=-1;
    } else {
      if (dt<-steer_tolerance_start) sprite->steer=-1;
      else if (dt>steer_tolerance_start) sprite->steer=1;
      else sprite->steer=0;
    }
    
    /* Gas when we're aiming near the target.
     */
    const double gas_tolerance=M_PI*0.125;
    if ((dt>-gas_tolerance)&&(dt<gas_tolerance)) sprite->gas=1;
    else sprite->gas=0;
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
