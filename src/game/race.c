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
  sprites_defunct_all();
  memcpy(g.map,g.mapro,g.mapw*g.maph);
  
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
          if ((g.mapro[p]<4)||((g.mapro[p]>=0x50)&&(g.mapro[p]<0x54))) g.map[p]=0x54; // Asphalt.
          else g.map[p]=0x55; // Assume sidewalk.
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
   * TODO CPU and multiplayer vehicles. For now, making just one.
   * TODO Declare vehicle type per race. For now, assuming car.
   * TODO Initial orientation. They're UP by default, but we need to declare per race.
   */
  cp=g.checkpointv;
  struct sprite *sprite=sprite_spawn_id(cp->x+cp->w*0.5,cp->y+cp->h*0.5,RID_sprite_hero,0,0);
  if (!sprite) return -1;
  sprite->t=M_PI*0.5;//XXX This needs to be generically declared by the race. For now, my scratch race starts Rightward.
  
  //TODO Schedule count-in.
  
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
  int lapc=2;//TODO
  struct sprite **spritep=g.spritev;
  int i=g.spritec;
  for (;i-->0;spritep++) {
    struct sprite *sprite=*spritep;
    if (sprite->defunct) continue;
    if (!sprite->vehicle) continue;
    if (sprite->lapid>lapc) {
      fprintf(stderr,"!!! Race completed in %.03f s !!!\n",g.racetime);
      race_begin(1);
    }
  }
}
