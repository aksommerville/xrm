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

extern struct g {
  void *rom;
  int romc;
  struct rom_entry *resv;
  int resc,resa;
  struct graf graf;
  int input,pvinput;
  
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
  
  struct sprite **spritev;
  int spritec,spritea;
  
  // main.c; for regulating sound effects:
  int bonkrid;
  double bonktime;
  
} g;

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

#endif
