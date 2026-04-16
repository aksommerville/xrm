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

#define CMD_map_image     0x20 /* u16:imageid */
#define CMD_map_sprite    0x61 /* u16:position, u16:spriteid, u32:arg */
#define CMD_map_door      0x62 /* u16:position, u16:mapid, u16:dstposition, u16:arg */

#define CMD_sprite_image 0x20 /* u16:imageid */
#define CMD_sprite_tile  0x21 /* u8:tileid, u8:xform */
#define CMD_sprite_type  0x22 /* u16:sprtype */

#define NS_tilesheet_physics 1
#define NS_tilesheet_family 0
#define NS_tilesheet_neighbors 0
#define NS_tilesheet_weight 0

#define NS_physics_vacant 0
#define NS_physics_solid 1
#define NS_physics_water 2
#define NS_physics_overpass 3 /* Vacant above and below, but you can only pass on one axis. */
#define NS_physics_bridge 4 /* Vacant above, water below. */

// Editor uses the comment after a 'sprtype' symbol as a prompt in the new-sprite modal.
// Should match everything after 'spriteid' in the CMD_map_sprite args.
#define NS_sprtype_dummy 0 /* (u32)0 */
#define FOR_EACH_SPRTYPE \
  _(dummy)

#endif
