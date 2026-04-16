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
    
    // If the hero is riding offroad, bump up by a pixel periodically.
    if (g.framec&8) {
      if (sprite_hero_is_offroad(focus)&&(sprite_hero_get_speed(focus)>=1.0)) {
        if (g.cameray) g.cameray-=2;
        else g.cameray+=2;
      }
    }
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
}
