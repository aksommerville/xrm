#include "xrm.h"

struct g g={0};

void egg_client_quit(int status) {
}

void egg_client_notify(int k,int v) {
}

/* Init.
 */

int egg_client_init() {

  int fbw=0,fbh=0;
  egg_texture_get_size(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Framebuffer size mismatch! metadata=%dx%d header=%dx%d\n",fbw,fbh,FBW,FBH);
    return -1;
  }

  if (xrm_res_init()<0) return -1;

  srand_auto();
  
  egg_play_song(2,RID_song_motors,1,0.333,0.0);

  //TODO
  if (race_begin(1)<0) return -1;

  return 0;
}

/* Update.
 */

void egg_client_update(double elapsed) {
  g.pvinput=g.input;
  g.input=egg_input_get_one(0);
  sprites_update(elapsed);
  camera_update(elapsed);
  race_update(elapsed);
}

/* Render.
 */

void egg_client_render() {
  graf_reset(&g.graf);
  camera_render();
  graf_flush(&g.graf);
}

/* Sound effects.
 */
 
void sfx(int rid) {
  egg_play_sound(rid,1.0,0.0);
}

void bonksfx(double velocity) {
  int rid;
  // Important that the "bonk" sounds be ordered by their apparent order. (RID_sound_bonk2>RID_sound_bonk1).
       if (velocity>= 25.000) rid=RID_sound_bonk4;
  else if (velocity>= 15.000) rid=RID_sound_bonk3;
  else if (velocity>=  8.000) rid=RID_sound_bonk2;
  else if (velocity>=  2.000) rid=RID_sound_bonk1;
  else return;
  double now=egg_time_real();
  double elapsed=now-g.bonktime;
  //fprintf(stderr,"%s velocity=%.03f elapsed=%.03f rid=%d\n",__func__,velocity,elapsed,rid);
  if (elapsed>=0.100) {
    // Previous bonk was far enough back that anything goes.
  } else if (rid<=g.bonkrid) {
    // Within the overlap threshold, only make a sound if it's more significant than the prior.
    return;
  }
  g.bonkrid=rid;
  g.bonktime=now;
  egg_play_sound(rid,1.0,0.0);
}
