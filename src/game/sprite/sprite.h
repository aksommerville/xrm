/* sprite.h
 * This is a quick jam game so I'll try to keep it simple.
 * One global list of sprites, no groups.
 * All sprites render as a single fancy, all from RID_image_sprites.
 * Any sprite with physics is circular.
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;

/* Sprites are drawn at 32 pixels and rendered at 24.
 * The map's tile size is 16, and that is our reference, "the meter".
 * So the default radius is 0.750.
 */
#define SPRITE_TILESIZE 24

/* Generic instances.
 ******************************************************************************/

struct sprite {
  const struct sprite_type *type;
  int defunct;
  double x,y; // In map meters.
  double radius;
  uint8_t tileid;
  uint8_t xform;
  double t; // radians
  uint32_t color;
  uint32_t tint;
  int solid;
  int layer; // Hero at 100, and that's the default.
  const void *cmd; // Includes sprite resource header.
  int cmdc;
  uint8_t arg[4];
  
  // The model for moving vehicles is all here at the generic level, so in theory CPU-driven cars can share it.
  // Controller sets these:
  int vehicle; // Zero if not participating, or NS_vehicle_*. Constant.
  double grip; // 0..1, essentially "how grippy are your tires?". Do not adjust per floor conditions; that will be done generically.
  double topspeed; // m/s
  int steer; // -1,0,1, current input state of dpad.
  int gas; // -1,0,1 = brake,idle,gas. Input state.
  // Internal use by vehicle:
  double vx,vy; // m/s, actual velocity.
  double drive; // m/s target forward.
  int pvgas;
  double gripbonus;
};

/* Only sprites_update() should call sprite_del(), and only sprite_spawn() should call sprite_new().
 * To delete a sprite safely, set its (defunct) nonzero.
 */
void sprite_del(struct sprite *sprite);
struct sprite *sprite_new(double x,double y,const struct sprite_type *type,const void *arg,int argc,const void *cmd,int cmdc);

/* Updates all that need it, then removes defunct.
 */
void sprites_update(double elapsed);

/* Sort all sprites, then render to main framebuffer, with camera's top-left at (camerax,cameray).
 * Note that (g.camerax,y) is the center but we want the corner.
 */
void sprites_render(int camerax,int cameray);

/* Create a new sprite and register it.
 * Returns WEAK on success.
 * (arg) is up to 4 bytes of per-instance params, normally provided by the map. If longer than 4, we trim it.
 * (cmd) must be constant and immortal, we're going to reference it throughout the sprite's life.
 */
struct sprite *sprite_spawn(
  double x,double y,
  const struct sprite_type *type,
  const void *arg,int argc,
  const void *cmd,int cmdc
);
struct sprite *sprite_spawn_id(
  double x,double y,
  int rid,
  const void *arg,int argc
);

/* Types.
 ***********************************************************************/

struct sprite_type {
  const char *name;
  int objlen;
  // It's possible for (del) to be called without (init), in error cases.
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite);
  void (*update)(struct sprite *sprite,double elapsed);
  
  /* Must restore graf_set_image(&g.graf,RID_image_sprites) before returning.
   * Also restore graf_set_filter(&g.graf,1).
   * (x,y) are the center of the sprite in framebuffer pixels.
   */
  void (*render)(struct sprite *sprite,int x,int y);
};

const struct sprite_type *sprite_type_by_id(int id);
int sprite_type_id_from_res(const void *cmd,int cmdc);
const struct sprite_type *sprite_type_from_res(const void *cmd,int cmdc);

#define _(tag) extern const struct sprite_type sprite_type_##tag;
FOR_EACH_SPRTYPE
#undef _

/* API for specific types.
 ********************************************************************/
 
int sprite_hero_is_offroad(const struct sprite *sprite);
double sprite_hero_get_speed(const struct sprite *sprite);

/* Physics and motors.
 *********************************************************************/
 
void sprite_vehicle_update(struct sprite *sprite,double elapsed);

#endif
