#include "game/xrm.h"

/* Configure sprite.
 */
 
int vehicle_acquire_config(struct sprite *sprite) {

  /* Pull config from the sprite's resource.
   * sprite_new() sets some initial defaults, and type->init has a chance to set other defaults.
   * But if the "vehicle" command is present, it overrides all of them. That should be so for all vehicle sprites.
   */
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,sprite->cmd,sprite->cmdc)<0) return -1;
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_sprite_vehicle: {
          sprite->vehicle=cmd.arg[0];
          sprite->grip=cmd.arg[1]/255.0;
          sprite->topspeed=cmd.arg[2]/255.0;
          sprite->steer_rate=cmd.arg[3]/255.0;
          sprite->accel_time=cmd.arg[4]/255.0;
          sprite->brake_time=cmd.arg[5]/255.0;
          sprite->idle_stop_time=cmd.arg[6]/255.0;
          sprite->bump_penalty=cmd.arg[7]/255.0;
        } break;
    }
  }
  
  /* Take those normalized values and apply a sensible range, in place.
   */
  #define SCALE(fld,lo,hi) { \
    if (sprite->fld<0.0) sprite->fld=0.0; \
    else if (sprite->fld>1.0) sprite->fld=1.0; \
    sprite->fld=(lo)*(1.0-sprite->fld)+(hi)*sprite->fld; \
  }
  // grip remains normalized.
  SCALE(topspeed,     10.000,40.000)
  SCALE(steer_rate,    2.000, 6.000)
  SCALE(accel_time,    1.000, 5.000)
  SCALE(brake_time,    0.250, 1.000)
  SCALE(idle_stop_time,1.000, 8.000)
  // bump_penalty remains normalized.
  #undef SCALE
  
  return 0;
}

/* Check position.
 * Update checkpoints etc.
 */
 
static void vehicle_check_position(struct sprite *sprite,double elapsed) {
  int qx=(int)sprite->x;
  int qy=(int)sprite->y;
  if ((qx!=sprite->qx)||(qy!=sprite->qy)) {
    sprite->qx=qx;
    sprite->qy=qy;
    int cpid=race_check_checkpoint_at_point(qx,qy);
    if ((cpid>=0)&&(cpid==sprite->next_checkpoint)) {
      fprintf(stderr,"%p CHECKPOINT %d at %d,%d\n",sprite,cpid,qx,qy);
      sprite->next_checkpoint++;
      if (sprite->next_checkpoint>=g.checkpointc) {
        sprite->next_checkpoint=0;
      } else if (sprite->next_checkpoint==1) { // Just crossed the finish line.
        if (sprite->lapid) fprintf(stderr,"%p LAP %d TIME %.03f\n",sprite,sprite->lapid,g.racetime-sprite->lapstarttime);
        sprite->lapid++;
        fprintf(stderr,"%p BEGIN LAP %d\n",sprite,sprite->lapid);
        //TODO Record and report lap time.
        sprite->lapstarttime=g.racetime;
      }
    }
  }
}

/* Update vehicle.
 */
 
void sprite_vehicle_update(struct sprite *sprite,double elapsed) {

  /* No playing during the countdown.
   * TODO Maybe we do want to allow them to rev the engine or turn?
   * TODO Maybe a Mario Kart style boost if you start your engine at the right moment, and spin out if you hit it early?
   */
  if (g.countdown>0.0) {
    vehicle_check_position(sprite,elapsed);
    return;
  }

  double steer_rate=sprite->steer_rate; // rad/s
  double drive_min=5.0; // m/s. Minimum speed when gas applied. To jackrabbit up the starts.
  double accel_time=sprite->accel_time; // s, how long to reach (topspeed) from standstill.
  double brake_time=sprite->brake_time; // s, how long to reach standstill from (topspeed).
  double idle_stop_time=sprite->idle_stop_time; // s. Idling is also a brake, but a much slower one.
  double grip_decel=30.0; // m/s**2. How much inertia to discard at perfect grip.
  double sync_rate_hi=100.0; // m/s**2. Maximum rate of acceleration or deceleration.
  double sync_rate_lo=  2.0; // ...according to (grip).
  double bumpy_grip_penalty=sprite->bump_penalty*0.500; // Grip multiplier.
  double bumpy_speed_penalty=sprite->bump_penalty; // Topspeed multiplier.

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
   *XXX This was intended to supply a boost if you strobe the gas during a turn. (which is unrealistic but feels good)
   * But in practice, you can strobe at any time and defeat traction. It's apparent in a boat or chopper.
   * I think nix it.
  if (sprite->gas!=sprite->pvgas) {
    if (sprite->pvgas>0) sprite->gripbonus=1.0;
    sprite->pvgas=sprite->gas;
  }
  if (sprite->gripbonus>0.0) {
    grip+=sprite->gripbonus*1.000;
    if (grip>1.0) grip=1.0;
    sprite->gripbonus-=elapsed*5.000;
  }
  /**/
  
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
   * Do still perform all the positional checks.
   */
  if ((sprite->gas<=0)&&(velocity<1.0)) {
    vehicle_check_position(sprite,elapsed);
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
    velocity=sqrt(sprite->vx*sprite->vx+sprite->vy*sprite->vy);
  }
  
  /* Apply velocity.
   */
  sprite->x+=sprite->vx*elapsed;
  sprite->y+=sprite->vy*elapsed;
  
  /* Clamp hard to the world's edge, and don't bounce or anything.
   */
  double worldw=g.mapw,worldh=g.maph;
  if (sprite->x<sprite->radius) sprite->x=sprite->radius;
  else if (sprite->x>worldw-sprite->radius) sprite->x=worldw-sprite->radius;
  if (sprite->y<sprite->radius) sprite->y=sprite->radius;
  else if (sprite->y>worldh-sprite->radius) sprite->y=worldh-sprite->radius;
  
  /* Detect and resolve collisions.
   */
  uint32_t physicsmask=1<<NS_physics_solid;
  switch (sprite->vehicle) {
    case NS_vehicle_car: physicsmask|=1<<NS_physics_water; break;
    case NS_vehicle_boat: physicsmask|=(1<<NS_physics_vacant)|(1<<NS_physics_bumpy); break;
  }
  struct collision collision;
  int panic=2;
  while (panic-->0) {
    if (sprite_find_collision(&collision,sprite,physicsmask)<=0) break;
    double esclen=sqrt(collision.dx*collision.dx+collision.dy*collision.dy);
    double nx=collision.dx/esclen;
    double ny=collision.dy/esclen;
    double proj=(sprite->vx*nx)+(sprite->vy*ny);
    if (collision.other&&collision.other->vehicle) {
      double ashare=0.500; // TODO Compare masses? ...I think we won't need to; all cars within a race should be the same thing.
      double bshare=1.0-ashare;
      double oproj=(collision.other->vx*nx)+(collision.other->vy*ny);
      proj-=oproj;
      proj*=1.500; // Bounce.
      sprite->x+=collision.dx*1.100*ashare;
      sprite->y+=collision.dy*1.100*ashare;
      sprite->vx-=(proj*nx);/*/esclen;*/
      sprite->vy-=(proj*ny);/*/esclen;*/
      collision.other->x-=collision.dx*1.100*bshare;
      collision.other->y-=collision.dy*1.100*bshare;
      collision.other->vx+=(proj*nx);/*/esclen;*/
      collision.other->vy+=(proj*ny);/*/esclen;*/
      
    } else {
      proj*=1.500; // Bounce.
      sprite->x+=collision.dx*1.100;
      sprite->y+=collision.dy*1.100;
      sprite->vx-=(proj*nx);/*/esclen;*/
      sprite->vy-=(proj*ny);/*/esclen;*/
    }
  }
  
  vehicle_check_position(sprite,elapsed);
}
