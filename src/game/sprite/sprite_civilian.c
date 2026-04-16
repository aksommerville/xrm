#include "game/xrm.h"

struct sprite_civilian {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_civilian*)sprite)

static int _civilian_init(struct sprite *sprite) {
  return 0;
}

static void _civilian_update(struct sprite *sprite,double elapsed) {
  sprite_vehicle_update(sprite,elapsed);
}

const struct sprite_type sprite_type_civilian={
  .name="civilian",
  .objlen=sizeof(struct sprite_civilian),
  .init=_civilian_init,
  .update=_civilian_update,
};
