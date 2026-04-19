#include "game/xrm.h"

/* Delete.
 */
 
void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->type->del) sprite->type->del(sprite);
  free(sprite);
}

/* Generic sprite commands.
 */
 
static int sprite_apply_generic_commands(struct sprite *sprite,const void *src,int srcc) {
  if (!srcc) return 0;
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,src,srcc)<0) return -1;
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
    
      case CMD_sprite_solid: {
          sprite->solid=1;
        } break;
    
      case CMD_sprite_tile: {
          sprite->tileid=cmd.arg[0];
          sprite->xform=cmd.arg[1];
        } break;
        
      case CMD_sprite_layer: {
          sprite->layer=(cmd.arg[0]<<8)|cmd.arg[1];
        } break;
        
      case CMD_sprite_radius: {
          sprite->radius=((cmd.arg[0]<<8)|cmd.arg[1])/(double)NS_sys_tilesize;
        } break;
        
      case CMD_sprite_vehicle: {
          sprite->vehicle=cmd.arg[0];
          sprite->grip=0.500;//TODO
          sprite->topspeed=20.0;//TODO
        } break;
        
      case CMD_sprite_color: {
          sprite->color=(cmd.arg[0]<<24)|(cmd.arg[1]<<16)|(cmd.arg[2]<<8)|cmd.arg[3];
        } break;
    
    }
  }
  return 0;
}

/* New.
 */
 
struct sprite *sprite_new(
  double x,double y,
  const struct sprite_type *type,
  const void *arg,int argc,
  const void *cmd,int cmdc
) {
  if ((cmdc<0)||(cmdc&&!cmd)) return 0;
  if ((argc<0)||(argc&&!arg)) return 0;
  if (!type||(type->objlen<(int)sizeof(struct sprite))) return 0;
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  
  sprite->type=type;
  sprite->x=x;
  sprite->y=y;
  sprite->cmd=cmd;
  sprite->cmdc=cmdc;
  if (argc>=4) memcpy(sprite->arg,arg,4);
  else memcpy(sprite->arg,arg,argc);
  sprite->layer=100;
  sprite->radius=0.750;
  sprite->color=0x808080ff;
  sprite->vehicle=0; // Not a vehicle initially. But the others will be set sensibly, so one can just toggle this field.
  sprite->grip=0.500;
  sprite->topspeed=0.500;
  sprite->steer_rate=0.500;
  sprite->accel_time=0.500;
  sprite->brake_time=0.500;
  sprite->idle_stop_time=0.500;
  sprite->bump_penalty=0.500;
  
  if (sprite_apply_generic_commands(sprite,cmd,cmdc)<0) {
    sprite_del(sprite);
    return 0;
  }
  
  if (type->init&&(type->init(sprite)<0)) {
    sprite_del(sprite);
    return 0;
  }
  
  return sprite;
}

/* Update all.
 */

void sprites_update(double elapsed) {
  struct sprite **spritep=g.spritev;
  int i=g.spritec;
  for (;i-->0;spritep++) {
    struct sprite *sprite=*spritep;
    if (sprite->defunct) continue;
    if (sprite->type->update) {
      sprite->type->update(sprite,elapsed);
    }
  }
  for (spritep=g.spritev+g.spritec-1,i=g.spritec;i-->0;spritep--) {
    struct sprite *sprite=*spritep;
    if (!sprite->defunct) continue;
    g.spritec--;
    memmove(spritep,spritep+1,sizeof(void*)*(g.spritec-i));
    sprite_del(sprite);
  }
}

/* Compare sprites for render.
 */
 
static int sprite_rendercmp(const void *a,const void *b) {
  const struct sprite *A=*(const struct sprite**)a;
  const struct sprite *B=*(const struct sprite**)b;
  if (A->layer<B->layer) return -1;
  if (A->layer>B->layer) return 1;
  if (A->y<B->y) return -1;
  if (A->y>B->y) return 1;
  return 0;
}

/* Render all.
 */

void sprites_render(int camerax,int cameray) {
  qsort(g.spritev,g.spritec,sizeof(void*),sprite_rendercmp);
  graf_set_image(&g.graf,RID_image_sprites);
  graf_set_filter(&g.graf,1);
  struct sprite **spritep=g.spritev;
  int i=g.spritec;
  for (;i-->0;spritep++) {
    struct sprite *sprite=*spritep;
    if (sprite->defunct) continue;
    int x=(int)(sprite->x*NS_sys_tilesize)-camerax;
    int y=(int)(sprite->y*NS_sys_tilesize)-cameray;
    if (sprite->type->render) {
      sprite->type->render(sprite,x,y);
    } else {
      int8_t rot=(int8_t)((sprite->t*128.0)/M_PI); // Careful! Must perform the cast signed or Wasm will clamp it.
      graf_fancy(&g.graf,x,y,sprite->tileid,sprite->xform,rot,SPRITE_TILESIZE,sprite->tint,sprite->color);
    }
  }
}

/* Spawn sprite.
 */

struct sprite *sprite_spawn(
  double x,double y,
  const struct sprite_type *type,
  const void *arg,int argc,
  const void *cmd,int cmdc
) {
  struct sprite *sprite=sprite_new(x,y,type,arg,argc,cmd,cmdc);
  if (!sprite) return 0;
  
  if (g.spritec>=g.spritea) {
    int na=g.spritea+64;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(g.spritev,sizeof(void*)*na);
    if (!nv) return 0;
    g.spritev=nv;
    g.spritea=na;
  }
  g.spritev[g.spritec++]=sprite;
  
  return sprite;
}

/* Convenience to spawn sprite with ID instead of resolved type.
 */
 
struct sprite *sprite_spawn_id(
  double x,double y,
  int rid,
  const void *arg,int argc
) {
  const void *cmd=0;
  int cmdc=xrm_res_get(&cmd,EGG_TID_sprite,rid);
  if (cmdc<1) return 0;
  const struct sprite_type *type=sprite_type_from_res(cmd,cmdc);
  if (!type) return 0;
  return sprite_spawn(x,y,type,arg,argc,cmd,cmdc);
}

/* Mark all sprites defunct.
 */
 
void sprites_defunct_all() {
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) (*p)->defunct=1;
}

/* Type registry.
 */
 
const struct sprite_type *sprite_type_by_id(int id) {
  switch (id) {
    #define _(tag) case NS_sprtype_##tag: return &sprite_type_##tag;
    FOR_EACH_SPRTYPE
    #undef _
  }
  return 0;
}

int sprite_type_id_from_res(const void *src,int srcc) {
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,src,srcc)<0) return 0;
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    if (cmd.opcode==CMD_sprite_type) {
      return (cmd.arg[0]<<8)|cmd.arg[1];
    }
  }
  return 0;
}

const struct sprite_type *sprite_type_from_res(const void *cmd,int cmdc) {
  return sprite_type_by_id(sprite_type_id_from_res(cmd,cmdc));
}
