#ifndef EGG_GAME_MAIN_H
#define EGG_GAME_MAIN_H

#include "egg/egg.h"
#include "util/stdlib/egg-stdlib.h"
#include "util/graf/graf.h"
#include "util/font/font.h"
#include "util/res/res.h"
#include "util/text/text.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"
#include "sprite/sprite.h"

#define FBW 320
#define FBH 320

#define CHECKPOINT_LIMIT 8
#define COUNTDOWN_TIME 5.0 /* Reported time will round up; "1" is the last thing you see before starting. */
#define COOLDOWN_TIME 3.0
#define LAPTEXT_LIMIT 24
#define LAPTEXT_PERIOD 0.500
#define LAPTEXT_DUTY   0.400
#define LAPTEXT_CYCLEC 6
#define RACE_COUNT 4 /* Could get this automatically from the resources, but it's more convenient to be hard-coded. */

// Modals are entirely bespoke, dispatched from main.
#define MODAL_HELLO 1
#define MODAL_GAMEOVER 2

extern struct g {
  void *rom;
  int romc;
  struct rom_entry *resv;
  int resc,resa;
  struct graf graf;
  int input,pvinput;
  int modal;
  
  // We have just one map resource; it gets loaded here during init.
  int mapw,maph;
  const uint8_t *mapro; // Points into the ROM.
  uint8_t *map; // Volatile. This is the one to use live.
  uint8_t physics[256];
  const void *mapcmd;
  int mapcmdc;
  
  // camera.c
  int camerax,cameray; // Top-left, in map pixels.
  int framec;
  int herospeed;
  int herospeed_clock; // Timed in frames, because render doesn't get a true clock.
  char laptext[LAPTEXT_LIMIT];
  int laptextc;
  double laptext_clock;
  int laptext_cycle; // counts down
  int countdown_pvframe;
  
  // race.c
  int raceid;
  struct checkpoint { int x,y,w,h; } checkpointv[CHECKPOINT_LIMIT];
  int checkpointc;
  double racetime;
  double countdown; // >0 when the race is ready but not started yet.
  double cooldown; // >0 when the race is over but still rendering and updating.
  int lapc;
  struct plan { double x,y; } *planv; // The path for autopilots. Initial position is at the end, start by targetting [0].
  int planc,plana;
  int finishc; // How many cars have finished?
  int player_rank;
  double best_lap_time;
  double overall_time;
  int overall_rank;
  
  struct sprite **spritev;
  int spritec,spritea;
  
  // main.c; for regulating sound effects:
  int bonkrid;
  double bonktime;
  
  /* Times in a scoreboard are "MM:SS.mmm". Leading zeroes should be replaced by spaces, before the seconds' ones.
   * Rank is the sum of one-based ranks per race. So 1..24 currently.
   */
  struct scoreboard {
    char overall_time[9];
    int overall_rank;
    struct racescore {
      char full[9];
      char lap[9];
      int rank;
    } racev[RACE_COUNT];
  } scoreboard;
  struct scoreboard hiscore;
  
} g;

void hello_begin();
void hello_update(double elapsed);
void hello_render();

void gameover_begin();
void gameover_update(double elapsed);
void gameover_render();

// xrm_res.c
int xrm_res_init();
int xrm_res_search(int tid,int rid);
int xrm_res_get(void *dstpp,int tid,int rid);

// camera.c
void camera_update(double elapsed); // Call after updating sprites. Finalizes (g.camerax,y).
void camera_render(); // Overwrites entire framebuffer.

// race.c
int race_begin(int raceid);
int race_check_checkpoint_at_point(int x,int y); // => index in (g.checkpointv), or <0
void race_update(double elapsed);

/* Magnitude for bonksfx() is in m/s, because that's what's readily available after collision detection.
 * I know, I know, it's really supposed to be gm/s**2. But we're not physicists here :)
 */
void sfx(int rid);
void bonksfx(double velocity);

/* High scores store under key "hiscore": KEY=VALUE pairs delimited by semicolon.
 * ot: overall_time: "MM:SS.mmm"
 * or: overall_rank: decimal integer
 * 0f: racev[0].full: "MM:SS.mmm"
 * 0l: racev[0].lap: "MM:SS.mmm"
 * 0r: racev[0].rank: decimal integer
 * 1,2,3, mutatis mutandi
 */
int scoreboard_repr_time(char *dst/*9*/,double sf);
int score_load(); // Always wipes (g.hiscore). Returns >=0 if repopulated.
int scoreboard_cmpcpy(struct scoreboard *dst,const struct scoreboard *src); // Copy better values from (src) to (dst). Returns >0 if anything copied.
int score_save();

#endif
