#include "game/xrm.h"

/* Primitives for colliding against static geometry.
 * (collision) is zeroed in advance, or null.
 * These may return 0; it's not certain yet whether any collision exists.
 */
 
static int sprite_collide_point(struct collision *collision,struct sprite *sprite,double x,double y) {
  double dx=sprite->x-x;
  double dy=sprite->y-y;
  double d2=dx*dx+dy*dy;
  if (d2>=sprite->radius*sprite->radius) return 0;
  if (collision) {
    double distance=sqrt(d2);
    double escdistance=sprite->radius-distance;
    collision->dx=(dx*escdistance)/distance;
    collision->dy=(dy*escdistance)/distance;
  }
  return 1;
}
 
static int sprite_collide_vert(struct collision *collision,struct sprite *sprite,double x) {
  double l=sprite->x-sprite->radius;
  double r=sprite->x+sprite->radius;
  if (x<=l) return 0;
  if (x>=r) return 0;
  if (collision) {
    if (x>=sprite->x) collision->dx=x-r;
    else collision->dx=x-l;
  }
  return 1;
}
 
static int sprite_collide_horz(struct collision *collision,struct sprite *sprite,double y) {
  double t=sprite->y-sprite->radius;
  double b=sprite->y+sprite->radius;
  if (y<=t) return 0;
  if (y>=b) return 0;
  if (collision) {
    if (y>=sprite->y) collision->dy=y-b;
    else collision->dy=y-t;
  }
  return 1;
}

/* Find collision.
 */
 
int sprite_find_collision(
  struct collision *collision,
  struct sprite *sprite,
  uint32_t physicsmask
) {
  if (!sprite) return 0;
  if (collision) {
    collision->other=0;
    collision->dx=0.0;
    collision->dy=0.0;
  }
  
  /* Check the map first if we're doing that.
   */
  if (physicsmask) {
    #define CANDIDATE_LIMIT 8
    struct collision candidatev[CANDIDATE_LIMIT]={0};
    int candidatec=0;
    int cola=(int)(sprite->x-sprite->radius); if (cola<0) cola=0;
    int colz=(int)(sprite->x+sprite->radius); if (colz>=g.mapw) colz=g.mapw-1;
    int rowa=(int)(sprite->y-sprite->radius); if (rowa<0) rowa=0;
    int rowz=(int)(sprite->y+sprite->radius); if (rowz>=g.maph) rowz=g.maph-1;
    const uint8_t *maprow=g.map+rowa*g.mapw+cola;
    int row=rowa; for (;row<=rowz;row++,maprow+=g.mapw) {
      const uint8_t *mapp=maprow;
      int col=cola; for (;col<=colz;col++,mapp++) {
        if (candidatec>=CANDIDATE_LIMIT) break;
        uint8_t physics=g.physics[*mapp];
        if (physicsmask&(1<<physics)) {
          // The formula is a little different depending on which of the cell's 9 regions our center is in.
          double l=col;
          double r=l+1.0;
          double t=row;
          double b=t+1.0;
          if (sprite->x<=l) {
            if (sprite->y<=t) {
              if (sprite_collide_point(candidatev+candidatec,sprite,l,t)) candidatec++;
            } else if (sprite->y>=b) {
              if (sprite_collide_point(candidatev+candidatec,sprite,l,b)) candidatec++;
            } else {
              if (sprite_collide_vert(candidatev+candidatec,sprite,l)) candidatec++;
            }
          } else if (sprite->x>=r) {
            if (sprite->y<=t) {
              if (sprite_collide_point(candidatev+candidatec,sprite,r,t)) candidatec++;
            } else if (sprite->y>=b) {
              if (sprite_collide_point(candidatev+candidatec,sprite,r,b)) candidatec++;
            } else {
              if (sprite_collide_vert(candidatev+candidatec,sprite,r)) candidatec++;
            }
          } else if (sprite->y<=t) {
            if (sprite_collide_horz(candidatev+candidatec,sprite,t)) candidatec++;
          } else if (sprite->y>=b) {
            if (sprite_collide_horz(candidatev+candidatec,sprite,b)) candidatec++;
          } else { // Sprite's midpoint is within the cell. I expect this will be very rare.
            if (!collision) return 1;
            double ld=sprite->x-l;
            double rd=r-sprite->x;
            double td=sprite->y-t;
            double bd=b-sprite->y;
            if ((ld<=rd)&&(ld<=td)&&(ld<=bd)) {
              candidatev[candidatec].dx=-ld-sprite->radius;
            } else if ((rd<=td)&&(td<=bd)) {
              candidatev[candidatec].dx=rd+sprite->radius;
            } else if (td<=bd) {
              candidatev[candidatec].dy=-td-sprite->radius;
            } else {
              candidatev[candidatec].dy=bd+sprite->radius;
            }
            candidatec++;
          }
        }
      }
    }
    // so.... "take the first collision we find" proved to be inadequate.
    // Instead, we're collecting map collisions, and now we'll return the most significant of them.
    // Without this, you'll sometimes like bounce sideways on striking a wall head-on.
    if (candidatec>0) {
      if (!collision) return 1;
      double bestd2=0.0;
      struct collision *best=candidatev;
      struct collision *q=candidatev;
      int i=candidatec;
      for (;i-->0;q++) {
        double d2=q->dx*q->dx+q->dy*q->dy;
        if (!best||(d2>bestd2)) {
          best=q;
          bestd2=d2;
        }
      }
      *collision=*best;
      return 1;
    }
  }
  
  /* Other sprites.
   */
  struct sprite **otherp=g.spritev;
  int i=g.spritec;
  for (;i-->0;otherp++) {
    struct sprite *other=*otherp;
    if (other->defunct) continue;
    if (!other->solid) continue;
    if (other==sprite) continue;
    double rsum=sprite->radius+other->radius;
    double rsum2=rsum*rsum;
    double dx=sprite->x-other->x;
    double dy=sprite->y-other->y;
    double d2=dx*dx+dy*dy;
    if (d2>=rsum2) continue;
    if (!collision) return 1;
    double distance=sqrt(d2);
    double pen=rsum-distance;
    collision->other=other;
    collision->dx=(dx*pen)/distance;
    collision->dy=(dy*pen)/distance;
    return 1;
  }
  
  return 0;
}
