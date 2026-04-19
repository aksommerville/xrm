/* race.c
 * We manage modifying the map for a given race, and tracking time and completion.
 */
 
#include "xrm.h"

/* Begin new race.
 */
 
int race_begin(int raceid) {
  fprintf(stderr,"%s(%d)\n",__func__,raceid);
  
  /* Eagerly clear global state.
   */
  g.raceid=raceid;
  g.checkpointc=0;
  memset(g.checkpointv,0,sizeof(g.checkpointv)); // We're going to populate sparsely.
  g.racetime=0.0;
  g.countdown=COUNTDOWN_TIME;
  g.lapc=1;
  g.planc=0;
  sprites_defunct_all();
  memcpy(g.map,g.mapro,g.mapw*g.maph);
  
  /* Get the race metadata resource and load it into globals.
   * And some things are transient, we only need them during race_begin().
   */
  uint8_t orient=0x40;
  int hero_spriteid=RID_sprite_hero;
  int cpu_spriteid=RID_sprite_self_driving_car;
  {
    const void *serial=0;
    int serialc=xrm_res_get(&serial,EGG_TID_race,raceid);
    if (serialc<1) {
      fprintf(stderr,"race:%d not found\n",raceid);
      return -1;
    }
    struct cmdlist_reader reader={.v=serial,.c=serialc};
    struct cmdlist_entry cmd;
    while (cmdlist_reader_next(&cmd,&reader)>0) {
      switch (cmd.opcode) {
        case CMD_race_herosprite: {
            hero_spriteid=(cmd.arg[0]<<8)|cmd.arg[1];
          } break;
        case CMD_race_orient: {
            orient=cmd.arg[0];
          } break;
        case CMD_race_laps: {
            g.lapc=cmd.arg[0];
          } break;
        case CMD_race_cpusprite: {
            cpu_spriteid=(cmd.arg[0]<<8)|cmd.arg[1];
          } break;
      }
    }
  }
  
  /* Scan the map for checkpoint and block commands, apply them.
   */
  struct cmdlist_reader reader={.v=g.mapcmd,.c=g.mapcmdc};
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_map_checkpoint: { // Fill (checkpointv) sparsely; we'll validate after.
          if (cmd.arg[4]!=raceid) break;
          if (cmd.arg[5]>=CHECKPOINT_LIMIT) {
            fprintf(stderr,"Invalid checkpoint id %d in race %d, at (%d,%d). Limit %d.\n",cmd.arg[5],raceid,cmd.arg[0],cmd.arg[1],CHECKPOINT_LIMIT);
            return -2;
          }
          if (cmd.arg[5]>=g.checkpointc) g.checkpointc=cmd.arg[5]+1;
          struct checkpoint *cp=g.checkpointv+cmd.arg[5];
          if (cp->w||cp->h) {
            fprintf(stderr,"Multiple checkpoint id %d in race %d (%d,%d) and (%d,%d)\n",cmd.arg[5],raceid,cmd.arg[0],cmd.arg[1],cp->x,cp->y);
            return -2;
          }
          cp->x=cmd.arg[0];
          cp->y=cmd.arg[1];
          cp->w=cmd.arg[2];
          cp->h=cmd.arg[3];
        } break;
      case CMD_map_block: {
          if (cmd.arg[2]!=raceid) break;
          int x=cmd.arg[0];
          int y=cmd.arg[1];
          if ((x>=g.mapw)||(y>=g.maph)) break; // oy!
          int p=y*g.mapw+x;
          if ((g.mapro[p]<4)||((g.mapro[p]>=0x50)&&(g.mapro[p]<0x54))) { // Asphalt.
            g.map[p]=0x54;
          } else if (
            ((g.mapro[p]>=0x15)&&(g.mapro[p]<=0x19))||
            ((g.mapro[p]>=0x25)&&(g.mapro[p]<=0x29))||
            ((g.mapro[p]>=0x35)&&(g.mapro[p]<=0x39))
          ) { // Water.
            g.map[p]=g.mapro[p]+5;
          } else { // Assume sidewalk.
            g.map[p]=0x55;
          }
        } break;
    }
  }
  
  /* Validate checkpoints.
   */
  if (g.checkpointc<2) {
    fprintf(stderr,"Race %d needs at least two checkpoints.\n",raceid);
    return -1;
  }
  struct checkpoint *cp=g.checkpointv;
  int i=g.checkpointc,seq=0;
  for (;i-->0;cp++,seq++) {
    if (!cp->w||!cp->h) { // Would come up if you miss one in the sequence. Or an explicit zero, that would be weird.
      fprintf(stderr,"Race %d, checkpoint %d has null size.\n",raceid,seq);
      return -1;
    }
    // Anything with nonzero bounds, assume it's kosher. No sense verifying it's passable and in bounds.
    // If we mess that up, it will be evident on the first test run, and won't start a fire or anything.
  }
  
  /* Generate vehicles at the starting grid, ie checkpoint zero.
   * Produce them in pairs, with the human last.
   */
  cp=g.checkpointv;
  double midx=cp->x+cp->w*0.5;
  double midy=cp->y+cp->h*0.5;
  double ax=midx,ay=midy,bx=midx,by=midy;
  if (orient&0x42) { ax-=1.0; bx+=1.0; }
  else { ay-=1.0; by+=1.0; }
  double speed_adjust=1.000; // All sprites get their topspeed attenuated slightly. More attenuation the further back we go.
  const double speed_adjust_adjust=0.980;//XXX Instead of this, use a bunch of different CPU driver sprites with their own configs.
  double t=0.0;
  switch (orient) {
    case 0x40: break; // Natural orientation is Up for all vehicle sprites.
    case 0x10: t=M_PI*-0.5; break;
    case 0x08: t=M_PI*0.5; break;
    case 0x02: t=M_PI; break;
  }
  int cpu_racerc=5;
  for (i=cpu_racerc;i-->0;) {
    struct sprite *sprite;
    double x,y;
    if (i&1) { x=bx; y=by; }
    else { x=ax; y=ay; }
    if (!(sprite=sprite_spawn_id(x,y,cpu_spriteid,0,0))) return -1;
    sprite->topspeed*=speed_adjust;
    speed_adjust*=speed_adjust_adjust;
    sprite->t=t;
    if (i&1) switch (orient) {
      case 0x40: midy+=2.0; ay+=2.0; by+=2.0; break;
      case 0x10: midx+=2.0; ax+=2.0; bx+=2.0; break;
      case 0x08: midx-=2.0; ax-=2.0; bx-=2.0; break;
      case 0x02: midy-=2.0; ay-=2.0; by-=2.0; break;
    }
  }
  // Then the hero.
  double x,y;
  if (cpu_racerc&1) { x=bx; y=by; }
  else { x=midx; y=midy; }
  struct sprite *hero=sprite_spawn_id(x,y,hero_spriteid,0,0);
  if (!hero) return -1;
  hero->t=t;
  
  /* Prepare autopilot plan.
   */
  if (vehicle_prepare_plan(hero->vehicle)<0) {
    fprintf(stderr,"vehicle_prepare_plan failed at race:%d, vehicle=%d\n",raceid,hero->vehicle);
    return -1;
  }
  
  return 0;
}

/* Is a given point within a checkpoint?
 */
 
int race_check_checkpoint_at_point(int x,int y) {
  struct checkpoint *cp=g.checkpointv;
  int i=0;
  for (;i<g.checkpointc;i++,cp++) {
    if (x<cp->x) continue;
    if (y<cp->y) continue;
    if (x>=cp->x+cp->w) continue;
    if (y>=cp->y+cp->h) continue;
    return i;
  }
  return -1;
}

/* Update.
 */
 
void race_update(double elapsed) {

  /* If the countdown is running, tick it and that's all.
   * Do not update on the frame that escapes it; vehicles update before us.
   */
  if (g.countdown>0.0) {
    g.countdown-=elapsed;
    return;
  }
  
  //TODO cooldown clock too?
  
  /* If the countdown is complete, the main clock is running.
   */
  g.racetime+=elapsed;
  
  /* The race is complete when the first vehicle completes its final lap.
   * In the vanishingly unlikely case that two vehicles cross on the same frame, we use whichever we find first.
   * (sprite->lapid) is one-based.
   */
  struct sprite **spritep=g.spritev;
  int i=g.spritec;
  for (;i-->0;spritep++) {
    struct sprite *sprite=*spritep;
    if (sprite->defunct) continue;
    if (!sprite->vehicle) continue;
    if (sprite->lapid>g.lapc) {
      fprintf(stderr,"!!! Race completed in %.03f s !!!\n",g.racetime);
      /* TODO Ultimately, completing a race should first enter a cooldown period, then return to the menu or whatever, how they started it.
       * For now, run them all sequentially. You can set the first raceid in main.c:egg_client_init().
       */
      if (g.raceid==3) race_begin(1);
      else race_begin(g.raceid+1);
    }
  }
}
