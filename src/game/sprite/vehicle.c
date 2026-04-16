#include "game/xrm.h"

/* Update vehicle.
 */
 
void sprite_vehicle_update(struct sprite *sprite,double elapsed) {

  //TODO Constants that might need extracted for config.
  double steer_rate=4.000; // rad/s
  double drive_min=5.0; // m/s. Minimum speed when gas applied. To jackrabbit up the starts.
  double accel_time=2.000; // s, how long to reach (topspeed) from standstill.
  double brake_time=0.500; // s, how long to reach standstill from (topspeed).
  double idle_stop_time=2.0; // s. Idling is also a brake, but a much slower one.
  double grip_decel=30.0; // m/s**2. How much inertia to discard at perfect grip.
  double sync_rate_hi=100.0; // m/s**2. Maximum rate of acceleration or deceleration.
  double sync_rate_lo=  2.0; // ...according to (grip).
  double bumpy_grip_penalty=0.500; // Grip multiplier.
  double bumpy_speed_penalty=0.500; // Topspeed multiplier.

  /* Direction adjusts naively according to dpad.
   */
  if (sprite->steer<0) {
    sprite->t-=steer_rate*elapsed;
    if (sprite->t<-M_PI) sprite->t+=M_PI*2.0;
  } else if (sprite->steer>0) {
    sprite->t+=steer_rate*elapsed;
    if (sprite->t>M_PI) sprite->t-=M_PI*2.0;
  }
  
  /* Determine effective grip.
   * If we're a car, check for suboptimal terrain and reduce if so.
   * Boats and helicopters, grip is constant. (and should be very low).
   * Bumpy terrain also reduces top speed.
   */
  double grip=sprite->grip;
  double topspeed=sprite->topspeed;
  if (sprite->vehicle==NS_vehicle_car) {
    int x=(int)sprite->x;
    int y=(int)sprite->y;
    if ((x>=0)&&(x<g.mapw)&&(y>=0)&&(y<g.maph)) {
      uint8_t physics=g.physics[g.map[y*g.mapw+x]];
      if (physics==NS_physics_bumpy) {
        grip*=bumpy_grip_penalty;
        topspeed*=bumpy_speed_penalty;
      }
    }
  }
  
  /* For just a couple frames after releasing the gas, we artificially boost grip.
   */
  if (sprite->gas!=sprite->pvgas) {
    if (sprite->pvgas>0) sprite->gripbonus=1.0;
    sprite->pvgas=sprite->gas;
  }
  if (sprite->gripbonus>0.0) {
    grip+=sprite->gripbonus*1.000;
    if (grip>1.0) grip=1.0;
    sprite->gripbonus-=elapsed*5.000;
  }
  
  /* Reduce inertia per grip.
   * If we reduce it all the way to zero, your tires are perfect, and the car goes exactly in the direction it's facing always.
   * If we don't reduce at all, you will eventually converge on the target vector but it could take a very long time.
   */
  double velocity=sqrt(sprite->vx*sprite->vx+sprite->vy*sprite->vy);
  double nvelocity=velocity-grip*grip_decel*elapsed;
  if (nvelocity<=0.0) {
    sprite->vx=0.0;
    sprite->vy=0.0;
    velocity=0.0;
  } else {
    sprite->vx=(sprite->vx*nvelocity)/velocity;
    sprite->vy=(sprite->vy*nvelocity)/velocity;
    velocity=nvelocity;
  }
  
  /* If velocity is below some hair-splitty threshold and gas not applied, get out.
   */
  if ((sprite->gas<=0)&&(velocity<1.0)) {
    return;
  }
  
  /* Calculate acceleration due to gas or brake.
   */
  double accel=0.0; // m/s**2
  if (sprite->gas>0) accel=topspeed/accel_time;
  else if (sprite->gas<0) accel=-topspeed/brake_time;
  else accel=-topspeed/idle_stop_time;
  
  /* (drive) accelerates by that much, and clamps to 0..topspeed.
   */
  sprite->drive+=accel*elapsed;
  if (sprite->drive>topspeed) sprite->drive=topspeed;
  else if ((sprite->gas>0)&&(sprite->drive<drive_min)) sprite->drive=drive_min;
  else if (sprite->drive<0.0) sprite->drive=0.0;
  
  /* Turn (drive) and (t) into our target velocity vector.
   */
  double nx=sin(sprite->t);
  double ny=-cos(sprite->t);
  double targetx=sprite->drive*nx; // m/s axiswise
  double targety=sprite->drive*ny;
  
  /* Slide velocity toward drive.
   */
  double syncdx=targetx-sprite->vx;
  double syncdy=targety-sprite->vy;
  double syncd2=syncdx*syncdx+syncdy*syncdy;
  if (syncd2>0.0) {
    double syncdistance=sqrt(syncd2);
    double sync_rate=sync_rate_lo*(1.0-grip)+sync_rate_hi*grip;
    sprite->vx+=(syncdx*sync_rate*elapsed)/syncdistance;
    sprite->vy+=(syncdy*sync_rate*elapsed)/syncdistance;
  }
  
  /* Apply velocity.
   */
  sprite->x+=sprite->vx*elapsed;
  sprite->y+=sprite->vy*elapsed;
  
  /* Detect and resolve collisions.
   */
  //TODO
}
