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
  
  // race.c
  int raceid;
  struct checkpoint { int x,y,w,h; } checkpointv[CHECKPOINT_LIMIT];
  int checkpointc;
  double racetime;
  double countdown; // >0 when the race is ready but not started yet.
  int lapc;
  struct plan { double x,y; } *planv; // The path for autopilots. Initial position is at the end, start by targetting [0].
  int planc,plana;
  
  struct sprite **spritev;
  int spritec,spritea;
  
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

/* With the map and race loaded except sprites, examine map and generate the plan for autopilots.
 * You must tell us what kind of vehicle we're planning for, since they all treat cell physics differently.
 * Paths are weighed based on turn count, not length. So if you see autopilots turning around unexpected, add more checkpoints around the twisty parts.
 * The plan, and autopilot's execution of it, are both decidedly suboptimal. That's good tho: We want the human to win without working too hard for it.
 * plangen.c
 *
 * XXX CONSIDERED HARMFUL
 * This generates a set of navigable boxes starting at checkpoint 0, and finds a path across overlapping boxes to reach sucessive checkpoints.
 * The strategy overall feels sound, but it's massively recursive and I fucked something up; in maps with lots of open space, it gets stuck in a loop somehow.
 * Since the game jam clock is ticking, forget this and just lay out the cpu's paths explicitly with map commands.
 */
int vehicle_prepare_plan(int vehicle);

#endif
