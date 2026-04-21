/* race.c
 * We manage modifying the map for a given race, and tracking time and completion.
 */
 
#include "xrm.h"

static const uint32_t cpucolorv[]={
  0xff0000ff,
  0xff8000ff,
  0xffff00ff,
  0x00c040ff,
  0x3080ffff,
  0x708090ff,
};

/* Begin new race.
 */
 
int race_begin(int raceid) {
  
  /* Eagerly clear global state.
   */
  g.raceid=raceid;
  g.checkpointc=0;
  memset(g.checkpointv,0,sizeof(g.checkpointv)); // We're going to populate sparsely.
  g.racetime=0.0;
  g.countdown=COUNTDOWN_TIME;
  g.cooldown=0.0;
  g.lapc=1;
  g.planc=0;
  g.laptextc=0;
  g.finishc=0;
  g.player_rank=0;
  sprites_del_all();
  memcpy(g.map,g.mapro,g.mapw*g.maph);
  
  /* Get the race metadata resource and load it into globals.
   * And some things are transient, we only need them during race_begin().
   */
  uint8_t orient=0x40;
  int hero_spriteid=RID_sprite_hero_car;
  int cpu_spriteid=RID_sprite_cpu_car;
  int cpu_racerc=0;
  const int CPU_LIMIT=15;
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
        case CMD_race_cpuc: {
            cpu_racerc=(cmd.arg[0]<<8)|cmd.arg[1];
            if (cpu_racerc>CPU_LIMIT) cpu_racerc=CPU_LIMIT; // 64k is clearly too many...
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
          int cpraceid=(cmd.arg[4]<<8)|cmd.arg[5];
          if (cpraceid!=raceid) break;
          int seq=cmd.arg[6];
          if (seq>=CHECKPOINT_LIMIT) {
            fprintf(stderr,"Invalid checkpoint id %d in race %d, at (%d,%d). Limit %d.\n",seq,raceid,cmd.arg[0],cmd.arg[1],CHECKPOINT_LIMIT);
            return -2;
          }
          if (seq>=g.checkpointc) g.checkpointc=seq+1;
          struct checkpoint *cp=g.checkpointv+seq;
          if (cp->w||cp->h) {
            fprintf(stderr,"Multiple checkpoint id %d in race %d (%d,%d) and (%d,%d)\n",seq,raceid,cmd.arg[0],cmd.arg[1],cp->x,cp->y);
            return -2;
          }
          cp->x=cmd.arg[0];
          cp->y=cmd.arg[1];
          cp->w=cmd.arg[2];
          cp->h=cmd.arg[3];
        } break;
      case CMD_map_block: {
          int braceid=(cmd.arg[2]<<8)|cmd.arg[3];
          if (braceid!=raceid) break;
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
          } else if ((g.mapro[p]>=0x0a)&&(g.mapro[p]<=0x0f)) { // Grass.
            g.map[p]=0x5a;
          } else { // Assume sidewalk.
            g.map[p]=0x55;
          }
        } break;
      case CMD_map_path: { // Order matters.
          int praceid=(cmd.arg[2]<<8)|cmd.arg[3];
          if (praceid!=raceid) break;
          double x=cmd.arg[0]+0.5;
          double y=cmd.arg[1]+0.5;
          if (g.planc>=g.plana) {
            int na=g.plana+32;
            if (na>INT_MAX/sizeof(struct plan)) return -1;
            void *nv=realloc(g.planv,sizeof(struct plan)*na);
            if (!nv) return -1;
            g.planv=nv;
            g.plana=na;
          }
          g.planv[g.planc++]=(struct plan){x,y};
        } break;
    }
  }
  
  /* Validate checkpoints.
   */
  if (g.planc<2) {
    fprintf(stderr,"Race %d needs at least two plan points.\n",raceid);
    return -2;
  }
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
  
  /* Create the finishline sprite.
   * Its natural orientation is vertical.
   * Should be 8 pixels offset from the checkpoint's edge.
   */
  cp=g.checkpointv;
  struct sprite *sprite=sprite_spawn(0.0,0.0,&sprite_type_finishline,0,0,0,0);
  if (!sprite) return -1;
  sprite_finishline_setup(sprite,cp,orient);
  
  /* Generate vehicles at the starting grid, ie checkpoint zero.
   * Produce them in pairs, with the human last.
   */
  double midx=cp->x+cp->w*0.5;
  double midy=cp->y+cp->h*0.5;
  switch (orient) { // Start cars *behind* the finish line, which is the *rear* end of the checkpoint.
    case 0x40: midy+=cp->h*0.6; break;
    case 0x10: midx+=cp->w*0.6; break;
    case 0x08: midx-=cp->w*0.6; break;
    case 0x02: midy-=cp->h*0.6; break;
  }
  double ax=midx,ay=midy,bx=midx,by=midy;
  if (orient&0x42) { ax-=1.0; bx+=1.0; }
  else { ay-=1.0; by+=1.0; }
  double speed_adjust=1.000,accel_adjust=1.000; // All CPU sprites get their topspeed attenuated slightly. More attenuation the further back we go.
  const double speed_adjust_adjust=0.900;
  const double accel_adjust_adjust=1.250;
  double t=0.0;
  switch (orient) {
    case 0x40: break; // Natural orientation is Up for all vehicle sprites.
    case 0x10: t=M_PI*-0.5; break;
    case 0x08: t=M_PI*0.5; break;
    case 0x02: t=M_PI; break;
  }
  int colorc=sizeof(cpucolorv)/sizeof(uint32_t);
  int p=0;
  for (i=cpu_racerc;i-->0;p++) {
    struct sprite *sprite;
    double x,y;
    if (p&1) { x=bx; y=by; }
    else { x=ax; y=ay; }
    if (!(sprite=sprite_spawn_id(x,y,cpu_spriteid,0,0))) return -1;
    if (p>=colorc) sprite->color=cpucolorv[colorc-1];
    else sprite->color=cpucolorv[p];
    sprite->topspeed*=speed_adjust;
    speed_adjust*=speed_adjust_adjust;
    sprite->accel_time*=accel_adjust;
    accel_adjust*=accel_adjust_adjust;
    sprite->t=t;
    if (p&1) switch (orient) {
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
    if (g.countdown<=0.0) {
      egg_play_song(1,RID_song_go_go_go,1,1.0,0.0);
    }
    return;
  }
  
  /* If the cooldown is running, tick it, terminate at zero, and don't update anything else.
   */
  if (g.cooldown>0.0) {
    if ((g.cooldown-=elapsed)<=0.0) {
      //TODO I don't like hard-coding the map count here, and starting over is not correct either. Should have a "game over" modal.
      if (g.raceid==4) race_begin(1);
      else race_begin(g.raceid+1);
    }
    return;
  }
  
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
    if (sprite->rank) continue; // He's already done.
    if (sprite->lapid>g.lapc) {
      g.finishc++; // Increment first, so the rest is one-based.
      sprite->rank=g.finishc;
      if (sprite->type==&sprite_type_hero) {
        egg_play_song(1,RID_song_gotcha_cup,0,1.0,0.0);
        g.cooldown=COOLDOWN_TIME;
        g.player_rank=sprite->rank;
      }
    }
  }
}
