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
  
  g.camerax=(g.mapw*NS_sys_tilesize)>>1;//XXX
  g.cameray=(g.maph*NS_sys_tilesize)>>1;
  
  if (!sprite_spawn_id(133.0,142.0,RID_sprite_hero,0,0)) return -1;

  //TODO

  return 0;
}

/* Update.
 */

void egg_client_update(double elapsed) {
  g.pvinput=g.input;
  g.input=egg_input_get_one(0);
  sprites_update(elapsed);
  camera_update(elapsed);
}

/* Render.
 */

void egg_client_render() {
  graf_reset(&g.graf);
  camera_render();
  graf_flush(&g.graf);
}
