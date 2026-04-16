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

extern struct g {
  void *rom;
  int romc;
  struct rom_entry *resv;
  int resc,resa;
  struct graf graf;
  int input,pvinput;
  
  // We have just one map resource; it gets loaded here during init.
  int mapw,maph;
  const uint8_t *map;
  uint8_t physics[256];
  const void *mapcmd;
  int mapcmdc;
  
  int camerax,cameray; // Center, in map pixels.
  
  struct sprite **spritev;
  int spritec,spritea;
  
} g;

int xrm_res_init();
int xrm_res_search(int tid,int rid);
int xrm_res_get(void *dstpp,int tid,int rid);

#endif
