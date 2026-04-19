/* shared_symbols.h
 * This file is consumed by eggdev and editor, in addition to compiling in with the game.
 */

#ifndef SHARED_SYMBOLS_H
#define SHARED_SYMBOLS_H

#define EGGDEV_importUtil "res,font,text,graf,stdlib" /* Comma-delimited list of Egg 'util' units to include in the build. */
#define EGGDEV_ignoreData "" /* Comma-delimited glob patterns for editor and builder to ignore under src/data/ */

#define NS_sys_tilesize 16
#define NS_sys_mapw 255
#define NS_sys_maph 255
#define NS_sys_bgcolor 0x000000

#define CMD_map_image      0x20 /* u16:imageid ; We ignore but editor uses. */
#define CMD_map_block      0x40 /* u16:position u8:raceid u8:reserved ; Becomes impassable during the named race. */
#define CMD_map_checkpoint 0x60 /* u32:bounds u8:raceid u8:sequence u16:reserved ; Start at sequence zero. All for the raceid must be entered in order. */
#define CMD_map_sprite     0x61 /* u16:position, u16:spriteid, u32:arg */

#define CMD_sprite_solid    0x01 /* --- */
#define CMD_sprite_image    0x20 /* u16:imageid ; We don't use, only the editor does. */
#define CMD_sprite_tile     0x21 /* u8:tileid, u8:xform */
#define CMD_sprite_type     0x22 /* u16:sprtype */
#define CMD_sprite_layer    0x23 /* u16:layer */
#define CMD_sprite_radius   0x24 /* u16:pixels */
#define CMD_sprite_color    0x40 /* u32:rgba */
#define CMD_sprite_vehicle  0x60 /* (u8:vehicle)car u8:grip u8:topspeed u8:steer_rate u8:accel_time u8:brake_time u8:idle_stop_time u8:bump_penalty ; untagged ones are normal to some sane range */

#define CMD_race_herosprite     0x20 /* u16:spriteid */
#define CMD_race_orient         0x21 /* u8:dir(0x40,0x10,0x08,0x02) u8:reserved ; direction cars should face at start */
#define CMD_race_laps           0x22 /* u8:count u8:reserved */
#define CMD_race_cpusprite      0x23 /* u16:spriteid */
#define CMD_race_cpuc           0x24 /* u16:count */

#define NS_tilesheet_physics 1
#define NS_tilesheet_family 0
#define NS_tilesheet_neighbors 0
#define NS_tilesheet_weight 0

#define NS_physics_vacant 0
#define NS_physics_solid 1
#define NS_physics_water 2
#define NS_physics_overpass 3 /* Vacant above and below, but you can only pass on one axis. */
#define NS_physics_bridge 4 /* Vacant above, water below. */
#define NS_physics_bumpy 5 /* Effectively vacant, but cars get a speed penalty. */

/* "vehicle" is the gross mode of operation, like what kind of tiles can we travel on.
 */
#define NS_vehicle_car     1 /* Travel on vacant, stopped by solid, splashes into water. */
#define NS_vehicle_boat    2 /* Travel on water, all else stops. */
#define NS_vehicle_chopper 3 /* Travel on vacant or water, stopped by solid. */

// Editor uses the comment after a 'sprtype' symbol as a prompt in the new-sprite modal.
// Should match everything after 'spriteid' in the CMD_map_sprite args.
#define NS_sprtype_dummy 0 /* (u32)0 */
#define NS_sprtype_hero 1
#define NS_sprtype_civilian 2
#define NS_sprtype_autopilot 3
#define FOR_EACH_SPRTYPE \
  _(dummy) \
  _(hero) \
  _(civilian) \
  _(autopilot)

#endif
