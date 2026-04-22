#include "xrm.h"

//TODO

/* Begin.
 */
 
void hello_begin() {
  g.modal=MODAL_HELLO;
  egg_play_song(1,RID_song_start_your_engines,1,0.500,0.0);
  g.pvinput=g.input; // Ignore any input strokes on our first update.
}

/* Activate.
 */
 
static void hello_activate() {
  egg_play_song(1,0,1,0.500,0.0);
  g.modal=0;
  g.overall_time=0.0;
  g.overall_rank=0;
  race_begin(1);
  race_update(0.016); // When changing modes, the new mode would miss its first update.
}

/* Update.
 */
 
void hello_update(double elapsed) {
  if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) hello_activate();
}

/* Render.
 */
 
void hello_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
}
