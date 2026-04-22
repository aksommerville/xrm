#include "xrm.h"

/* Private globals.
 */
 
#define VTX_LIMIT 256
 
static struct {
  struct egg_render_tile vtxv[VTX_LIMIT];
  int vtxc;
} gameover={0};

/* Add row of vertices.
 */
 
static void gameover_add_row_text(int y,const char *label,int labelc,const char *now,int nowc,const char *best,int bestc) {
  int x=40;
  int i=labelc;
  for (;i-->0;label++,x+=8) {
    if ((*label)<=0x20) continue;
    if ((*label)>=0x7f) continue;
    if (gameover.vtxc>=VTX_LIMIT) return;
    gameover.vtxv[gameover.vtxc++]=(struct egg_render_tile){x,y,*label,0};
  }
  for (x=190,i=nowc;i-->0;x-=8) {
    char ch=now[i];
    if ((ch<=0x20)||(ch>=0x7f)) continue;
    if (gameover.vtxc>=VTX_LIMIT) return;
    gameover.vtxv[gameover.vtxc++]=(struct egg_render_tile){x,y,ch,0};
  }
  for (x=280,i=bestc;i-->0;x-=8) {
    char ch=best[i];
    if ((ch<=0x20)||(ch>=0x7f)) continue;
    if (gameover.vtxc>=VTX_LIMIT) return;
    gameover.vtxv[gameover.vtxc++]=(struct egg_render_tile){x,y,ch,0};
  }
  if (gameover.vtxc>=VTX_LIMIT) return;
  
  /* A brittle fact of our formatting is that in every case, shorter or lexical first is better.
   * So we can generically decide where to put stars.
   */
  if (labelc) { // Don't put a star in the labels row!
    if ((nowc<bestc)||((nowc==bestc)&&(memcmp(now,best,nowc)<=0))) {
      gameover.vtxv[gameover.vtxc++]=(struct egg_render_tile){206,y,0x80,0};
    }
  }
}

static void gameover_add_row_int(int y,const char *label,int labelc,int now,int best) {
  if (now<0) now=0; else if (now>99) now=99;
  if (best<0) best=0; else if (best>99) best=99;
  char nows[2],bests[2];
  if (now>=10) nows[0]='0'+now/10; else nows[0]=' ';
  nows[1]='0'+now%10;
  if (best>=10) bests[0]='0'+best/10; else bests[0]=' ';
  bests[1]='0'+best%10;
  gameover_add_row_text(y,label,labelc,nows,2,bests,2);
}

/* Begin.
 */
 
void gameover_begin() {
  g.modal=MODAL_GAMEOVER;
  g.pvinput=g.input; // Ignore any input strokes on our first update.
  sprites_del_all();
  
  scoreboard_repr_time(g.scoreboard.overall_time,g.overall_time);
  g.scoreboard.overall_rank=g.overall_rank;
  
  /* Commit new high scores.
   */
  if (scoreboard_cmpcpy(&g.hiscore,&g.scoreboard)>0) {
    score_save();
  }
  
  /* Generate the report text, as a vertex list.
   */
  gameover.vtxc=0;
  int y=30;
  gameover_add_row_text(y,"",0,"Now",3,"Best",4); y+=16;
  gameover_add_row_text(y,"Time",4,g.scoreboard.overall_time,9,g.hiscore.overall_time,9); y+=8;
  gameover_add_row_int (y,"Rank",4,g.scoreboard.overall_rank,g.hiscore.overall_rank); y+=16;
  gameover_add_row_text(y,"1: Total",8,g.scoreboard.racev[0].full,9,g.hiscore.racev[0].full,9); y+=8;
  gameover_add_row_text(y,"1: Lap",  6,g.scoreboard.racev[0].lap,9, g.hiscore.racev[0].lap,9); y+=8;
  gameover_add_row_int (y,"1: Rank", 7,g.scoreboard.racev[0].rank,  g.hiscore.racev[0].rank); y+=16;
  gameover_add_row_text(y,"2: Total",8,g.scoreboard.racev[1].full,9,g.hiscore.racev[1].full,9); y+=8;
  gameover_add_row_text(y,"2: Lap",  6,g.scoreboard.racev[1].lap,9, g.hiscore.racev[1].lap,9); y+=8;
  gameover_add_row_int (y,"2: Rank", 7,g.scoreboard.racev[1].rank,  g.hiscore.racev[1].rank); y+=16;
  gameover_add_row_text(y,"3: Total",8,g.scoreboard.racev[2].full,9,g.hiscore.racev[2].full,9); y+=8;
  gameover_add_row_text(y,"3: Lap",  6,g.scoreboard.racev[2].lap,9, g.hiscore.racev[2].lap,9); y+=8;
  gameover_add_row_int (y,"3: Rank", 7,g.scoreboard.racev[2].rank,  g.hiscore.racev[2].rank); y+=16;
  gameover_add_row_text(y,"4: Total",8,g.scoreboard.racev[3].full,9,g.hiscore.racev[3].full,9); y+=8;
  gameover_add_row_text(y,"4: Lap",  6,g.scoreboard.racev[3].lap,9, g.hiscore.racev[3].lap,9); y+=8;
  gameover_add_row_int (y,"4: Rank", 7,g.scoreboard.racev[3].rank,  g.hiscore.racev[3].rank); y+=16;
}

/* Activate.
 */
 
static void gameover_activate() {
  hello_begin();
  hello_update(0.016); // When changing modes, the new mode would miss its first update.
}

/* Update.
 */
 
void gameover_update(double elapsed) {
  if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) gameover_activate();
}

/* Render.
 */
 
void gameover_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x1020c0ff);
  graf_set_image(&g.graf,RID_image_fonttiles);
  graf_tile_batch(&g.graf,gameover.vtxv,gameover.vtxc);
}
