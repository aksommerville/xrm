/* camera.c
 * Our scene rendering is dead simple, so this unit also takes on responsibility for overlays and such.
 */

#include "xrm.h"

/* Find the focus sprite.
 */
 
static struct sprite *camera_find_focus() {
  struct sprite *fallback=0;
  struct sprite **spritep=g.spritev;
  int i=g.spritec;
  for (;i-->0;spritep++) {
    struct sprite *sprite=*spritep;
    if (sprite->defunct) continue;
    if (sprite->type==&sprite_type_hero) return sprite;
    if (sprite->type==&sprite_type_autopilot) fallback=sprite;
  }
  return fallback;
}

/* Update.
 */
 
void camera_update(double elapsed) {

  if (g.laptextc) {
    if ((g.laptext_clock-=elapsed)<=0.0) {
      g.laptext_clock+=LAPTEXT_PERIOD;
      if (--(g.laptext_cycle)<=0) g.laptextc=0;
    }
  }

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

/* Render overlay, when race running.
 */
 
static void camera_render_race_overlay() {

  /* Clock in the upper left.
   */
  {
    int ms=(int)(g.racetime*1000.0);
    int sec=ms/1000; ms%=1000;
    int min=sec/60; sec%=60;
    if (min>99) { // oh come on
      min=sec=99;
      ms=999;
    }
    int x=7;
    int y=7;
    graf_set_image(&g.graf,RID_image_fonttiles);
    graf_tile(&g.graf,x,y,'0'+min/10,0); x+=8;
    graf_tile(&g.graf,x,y,'0'+min%10,0); x+=8;
    graf_tile(&g.graf,x,y,':',0); x+=8;
    graf_tile(&g.graf,x,y,'0'+sec/10,0); x+=8;
    graf_tile(&g.graf,x,y,'0'+sec%10,0); x+=8;
    graf_tile(&g.graf,x,y,'.',0); x+=8;
    graf_tile(&g.graf,x,y,'0'+ms/100,0); x+=8;
    graf_tile(&g.graf,x,y,'0'+(ms/10)%10,0); x+=8;
    graf_tile(&g.graf,x,y,'0'+ms%10,0);
  }
  
  /* Find the hero sprite for the rest.
   * For now we're not doing multiplayer, and there's no way yet to distinguish which player it is.
   */
  struct sprite *hero=0;
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    if ((*p)->type!=&sprite_type_hero) continue;
    hero=*p;
    break;
  }
  if (!hero) return;
  
  /* Speed in the upper right.
   * The hero's car has a top speed around 75 km/h by the pedantic formula.
   * Pretty wimpy for a race car, so let's double it. It's all just made-up numbers anyway.
   * This number tends to fluctuate, so sample it at a slower rate.
   */
  {
    if (--(g.herospeed_clock)<=0) {
      g.herospeed_clock+=5; // 12 hz or so
      double scale=3600.0/1000.0; // Mathematically correct, if a "meter" was really a meter.
      scale*=2.0; // Call them faster so it sounds more muscley.
      double mps=sqrt(hero->vx*hero->vx+hero->vy*hero->vy);
      double kmph=mps*scale;
      g.herospeed=(int)kmph;
      if (g.herospeed<0) g.herospeed=0;
      else if (g.herospeed>999) g.herospeed=999;
    }
    int x=FBW-8;
    int y=7;
    graf_tile(&g.graf,x,y,'h',0); x-=8;
    graf_tile(&g.graf,x,y,'/',0); x-=8;
    graf_tile(&g.graf,x,y,'m',0); x-=8;
    graf_tile(&g.graf,x,y,'k',0); x-=16;
    graf_tile(&g.graf,x,y,'0'+g.herospeed%10,0); x-=8;
    if (g.herospeed>=10) {
      graf_tile(&g.graf,x,y,'0'+(g.herospeed/10)%10,0); x-=8;
      if (g.herospeed>=100) {
        graf_tile(&g.graf,x,y,'0'+g.herospeed/100,0);
      }
    }
  }
  
  /* Lap indicator dead center.
   * No race will have a double-digit lap count.
   */
  {
    int p=hero->lapid;
    if (p<=g.lapc) { // Disappear when finished.
      if (p<1) p=1;
      int y=7;
      int x=(FBW>>1)-(4*7)+4;
      graf_tile(&g.graf,x,y,'L',0); x+=8;
      graf_tile(&g.graf,x,y,'a',0); x+=8;
      graf_tile(&g.graf,x,y,'p',0); x+=16;
      graf_tile(&g.graf,x,y,'0'+p,0); x+=8;
      graf_tile(&g.graf,x,y,'/',0); x+=8;
      graf_tile(&g.graf,x,y,'0'+g.lapc,0);
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
  
  // XXX TEMP: Show the autopilots' plan.
  if (0) {
    if (g.planc>1) {
      uint32_t linecolor=0xff0000ff;
      graf_set_image(&g.graf,0);
      graf_line_strip_begin(&g.graf,(int)(g.planv[0].x*NS_sys_tilesize)-g.camerax,(int)(g.planv[0].y*NS_sys_tilesize)-g.cameray,linecolor);
      struct plan *plan=g.planv+1;
      int i=g.planc-1;
      for (;i-->0;plan++) {
        graf_line_strip_more(&g.graf,(int)(plan->x*NS_sys_tilesize)-g.camerax,(int)(plan->y*NS_sys_tilesize)-g.cameray,linecolor);
      }
      graf_line_strip_more(&g.graf,(int)(g.planv[0].x*NS_sys_tilesize)-g.camerax,(int)(g.planv[0].y*NS_sys_tilesize)-g.cameray,linecolor);
    }
  }
  
  // Sprites.
  sprites_render(g.camerax,g.cameray);
  
  /* When the race is on, show some vitals.
   */
  if (g.racetime>0.0) camera_render_race_overlay();
  
  /* Blinking lap time after you complete one.
   */
  if (g.laptextc&&(g.laptext_clock<LAPTEXT_DUTY)) {
    graf_set_image(&g.graf,RID_image_fonttiles);
    int y=(FBH>>1)+NS_sys_tilesize*2;
    int x=(FBW>>1)-(g.laptextc*4)+4;
    const char *src=g.laptext;
    int i=g.laptextc;
    for (;i-->0;src++,x+=8) graf_tile(&g.graf,x,y,*src,0);
  }
  
  /* After completion, show a big cup indicating your rank.
   */
  if (g.player_rank) {
    int srcy=96;
    int srcx=(g.player_rank<4)?((g.player_rank-1)*64):192;
    int dstx=(FBW>>1)-32;
    int dsty=(FBH>>2)-32;
    graf_set_image(&g.graf,RID_image_chunks);
    graf_decal(&g.graf,dstx,dsty,srcx,srcy,64,64);
  }
  
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
