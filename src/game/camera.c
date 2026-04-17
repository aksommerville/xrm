/* camera.c
 * Our scene rendering is dead simple, so this unit also takes on responsibility for overlays and such.
 */

#include "xrm.h"

/* Find the focus sprite.
 */
 
static struct sprite *camera_find_focus() {
  struct sprite **spritep=g.spritev;
  int i=g.spritec;
  for (;i-->0;spritep++) {
    struct sprite *sprite=*spritep;
    if (sprite->defunct) continue;
    if (sprite->type==&sprite_type_hero) return sprite;
  }
  return 0;
}

/* Update.
 */
 
void camera_update(double elapsed) {

  /* If we have a focus sprite (should always), center on it and then clamp to map edges.
   * The map is guaranteed to be at least as large as the framebuffer.
   */
  struct sprite *focus=camera_find_focus();
  if (focus) {
    int worldw=g.mapw*NS_sys_tilesize;
    int worldh=g.maph*NS_sys_tilesize;
    g.camerax=(int)(focus->x*NS_sys_tilesize)-(FBW>>1);
    g.cameray=(int)(focus->y*NS_sys_tilesize)-(FBH>>1);
    if (g.camerax<0) g.camerax=0;
    else if (g.camerax>worldw-FBW) g.camerax=worldw-FBW;
    if (g.cameray<0) g.cameray=0;
    else if (g.cameray>worldh-FBH) g.cameray=worldh-FBH;
  }
}

/* Render.
 */
 
void camera_render() {
  g.framec++;
  
  // Map.
  graf_set_image(&g.graf,RID_image_terrain);
  int cola=g.camerax/NS_sys_tilesize; if (cola<0) cola=0;
  int rowa=g.cameray/NS_sys_tilesize; if (rowa<0) rowa=0;
  int colz=(g.camerax+FBW-1)/NS_sys_tilesize; if (colz>=g.mapw) colz=g.mapw-1;
  int rowz=(g.cameray+FBH-1)/NS_sys_tilesize; if (rowz>=g.maph) rowz=g.maph-1;
  if ((cola<=colz)&&(rowa<=rowz)) {
    int dstx0=cola*NS_sys_tilesize+(NS_sys_tilesize>>1)-g.camerax;
    int dsty=rowa*NS_sys_tilesize+(NS_sys_tilesize>>1)-g.cameray;
    const uint8_t *maprow=g.map+rowa*g.mapw+cola;
    int row=rowa; for (;row<=rowz;row++,dsty+=NS_sys_tilesize,maprow+=g.mapw) {
      int dstx=dstx0;
      const uint8_t *mapp=maprow;
      int col=cola; for (;col<=colz;col++,dstx+=NS_sys_tilesize,mapp++) {
        graf_tile(&g.graf,dstx,dsty,*mapp,0);
      }
    }
  }
  
  // Sprites.
  sprites_render(g.camerax,g.cameray);
  
  //TODO Overlay? Clock, HP, score, ...?
  
  /* Countdown, if running.
   */
  if (g.countdown>0.0) {
    int ms=(int)(g.countdown*1000.0);
    int sec=(ms+999)/1000;
    if (sec>5) {
      sec=5;
      ms=999;
    } else if (sec<1) {
      sec=1;
      ms=0;
    } else {
      ms%=1000;
    }
    graf_set_image(&g.graf,RID_image_chunks);
    //void graf_decal_rotate(struct graf *graf,int dstx,int dsty,int srcx,int srcy,int w_and_h,double sint,double cost,double scale);
    double scale=0.250+(2.000*ms)/1000.0;
    int alpha=(ms*255)/1000;
    if (alpha>0xff) alpha=0xff; else if (alpha<1) alpha=1;
    graf_set_alpha(&g.graf,alpha);
    int srcx=(5-sec)*48;
    graf_decal_rotate(&g.graf,FBW>>1,FBH>>1,srcx,0,48,0.0,1.0,scale);
    graf_set_alpha(&g.graf,0xff);
    
  /* Right after the countdown completes, say "GO!" for a little bit.
   */
  } else if (g.racetime<2.000) {
    int frame=(int)(g.racetime*15.0)&3;
    int srcx=48*frame;
    int srcy=48;
    double p=sin(g.racetime*20.0); // Higher means faster wobble.
    double t=p*0.250; // TODO extent of wobble, in radians
    if (g.racetime>1.5) {
      int alpha=(int)((2.0-g.racetime)*512.0);
      if (alpha<1) alpha=1; else if (alpha>0xff) alpha=0xff;
      graf_set_alpha(&g.graf,alpha);
    }
    graf_set_image(&g.graf,RID_image_chunks);
    graf_decal_rotate(&g.graf,FBW>>1,(FBH>>1)-16,srcx,srcy,48,sin(t),cos(t),1.0);
    graf_set_alpha(&g.graf,0xff);
  }
}
