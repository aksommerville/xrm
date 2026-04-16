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
  
  g.camerax=(g.mapw*NS_sys_tilesize)>>1;
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
  
  const int speed=2;//XXX
  switch (g.input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: g.camerax-=speed; break;
    case EGG_BTN_RIGHT: g.camerax+=speed; break;
  }
  switch (g.input&(EGG_BTN_UP|EGG_BTN_DOWN)) {
    case EGG_BTN_UP: g.cameray-=speed; break;
    case EGG_BTN_DOWN: g.cameray+=speed; break;
  }
  
  sprites_update(elapsed);
  //TODO
}

/* Render.
 */

void egg_client_render() {
  graf_reset(&g.graf);
  
  // Draw the background. TODO organized camera
  graf_set_image(&g.graf,RID_image_terrain);
  int camerax=g.camerax-(FBW>>1);
  int cameray=g.cameray-(FBH>>1);
  int cola=camerax/NS_sys_tilesize; if (cola<0) cola=0;
  int rowa=cameray/NS_sys_tilesize; if (rowa<0) rowa=0;
  int colz=(camerax+FBW-1)/NS_sys_tilesize; if (colz>=g.mapw) colz=g.mapw-1;
  int rowz=(cameray+FBH-1)/NS_sys_tilesize; if (rowz>=g.maph) rowz=g.maph-1;
  if ((cola<=colz)&&(rowa<=rowz)) {
    int dstx0=cola*NS_sys_tilesize+(NS_sys_tilesize>>1)-camerax;
    int dsty=rowa*NS_sys_tilesize+(NS_sys_tilesize>>1)-cameray;
    const uint8_t *maprow=g.map+rowa*g.mapw+cola;
    int row=rowa; for (;row<=rowz;row++,dsty+=NS_sys_tilesize,maprow+=g.mapw) {
      int dstx=dstx0;
      const uint8_t *mapp=maprow;
      int col=cola; for (;col<=colz;col++,dstx+=NS_sys_tilesize,mapp++) {
        graf_tile(&g.graf,dstx,dsty,*mapp,0);
      }
    }
  }
  sprites_render(camerax,cameray);
  
  //TODO
  graf_flush(&g.graf);
}
