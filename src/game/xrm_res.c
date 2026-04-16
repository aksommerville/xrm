#include "xrm.h"

/* Load the map.
 */
 
static int xrm_load_map(const void *src,int srcc) {
  struct map_res res;
  if (map_res_decode(&res,src,srcc)<0) return -1;
  g.mapw=res.w;
  g.maph=res.h;
  g.map=res.v;
  g.mapcmd=res.cmd;
  g.mapcmdc=res.cmdc;
  return 0;
}

/* Load the tilesheet.
 */
 
static int xrm_load_tilesheet(const void *src,int srcc) {
  struct tilesheet_reader reader;
  if (tilesheet_reader_init(&reader,src,srcc)<0) return -1;
  struct tilesheet_entry entry;
  while (tilesheet_reader_next(&entry,&reader)>0) {
    if (entry.tableid==NS_tilesheet_physics) {
      memcpy(g.physics+entry.tileid,entry.v,entry.c);
    }
  }
  return 0;
}

/* Receive resource during initial pass.
 * The TOC is not fully populated yet, but anything with a lower tid is available.
 * Return >0 to record in TOC.
 */
 
static int xrm_rcvres(int tid,int rid,const void *v,int c) {
  switch (tid) {
  
    // No need to TOC these.
    case EGG_TID_metadata:
    case EGG_TID_code:
    case EGG_TID_image:
    case EGG_TID_song:
    case EGG_TID_sound:
      return 0;
      
    // There must be just one map, id 1.
    case EGG_TID_map: {
        if (rid==1) {
          if (xrm_load_map(v,c)<0) return -1;
        }
      } return 0;
      
    // There must be one tilesheet, id 2.
    case EGG_TID_tilesheet: {
        if (rid==RID_image_terrain) {
          if (xrm_load_tilesheet(v,c)<0) return -1;
        }
      } return 0;
    
    default: return 1;
  }
}

/* Init.
 */
 
int xrm_res_init() {

  g.romc=egg_rom_get(0,0);
  if (!(g.rom=malloc(g.romc))) return -1;
  egg_rom_get(g.rom,g.romc);
  text_set_rom(g.rom,g.romc);
  
  struct rom_reader reader;
  if (rom_reader_init(&reader,g.rom,g.romc)<0) return -1;
  struct rom_entry res;
  while (rom_reader_next(&res,&reader)>0) {
    int err=xrm_rcvres(res.tid,res.rid,res.v,res.c);
    if (err<0) return err;
    if (err>0) {
      if (g.resc>=g.resa) {
        int na=g.resa+64;
        if (na>INT_MAX/sizeof(struct rom_entry)) return -1;
        void *nv=realloc(g.resv,sizeof(struct rom_entry)*na);
        if (!nv) return -1;
        g.resv=nv;
        g.resa=na;
      }
      g.resv[g.resc++]=res;
    }
  }
  
  return 0;
}

/* Search res toc.
 */
 
int xrm_res_search(int tid,int rid) {
  int lo=0,hi=g.resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct rom_entry *q=g.resv+ck;
         if (tid<q->tid) hi=ck;
    else if (tid>q->tid) lo=ck+1;
    else if (rid<q->rid) hi=ck;
    else if (rid>q->rid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int xrm_res_get(void *dstpp,int tid,int rid) {
  int p=xrm_res_search(tid,rid);
  if (p<0) return 0;
  const struct rom_entry *res=g.resv+p;
  *(const void**)dstpp=res->v;
  return res->c;
}
