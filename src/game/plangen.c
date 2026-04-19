#include "xrm.h"

#define CAR_WITH_BUMPIES 12345

/* Helper for a list of contiguous boxes.
 */
 
struct box {
  int x,y,w,h;
};

struct boxpath {
  struct box **v; // Box storage is elsewhere. Size doesn't matter, but using a pointer is helpful for testing identity.
  int c,a;
};

static void boxpath_cleanup(struct boxpath *boxpath) {
  if (boxpath->v) free(boxpath->v);
}

static int boxpath_require(struct boxpath *boxpath) {
  if (boxpath->c<boxpath->a) return 0;
  int na=boxpath->a+16;
  if (na>INT_MAX/sizeof(struct box)) return -1;
  void *nv=realloc(boxpath->v,sizeof(struct box)*na);
  if (!nv) return -1;
  boxpath->v=nv;
  boxpath->a=na;
  return 0;
}

static int boxpath_append(struct boxpath *boxpath,struct box *box_BORROW) {
  if (boxpath_require(boxpath)<0) return -1;
  boxpath->v[boxpath->c++]=box_BORROW;
  return 0;
}

static int boxpath_has(const struct boxpath *boxpath,const struct box *box) {
  struct box **p=boxpath->v;
  int i=boxpath->c;
  for (;i-->0;p++) if (*p==box) return 1;
  return 0;
}

// Safely and efficiently exchange content of two boxpaths.
static void boxpath_swap(struct boxpath *a,struct boxpath *b) {
  void *vswap=a->v;
  a->v=b->v;
  b->v=vswap;
  int iswap=a->c;
  a->c=b->c;
  b->c=iswap;
  iswap=a->a;
  a->a=b->a;
  b->a=iswap;
}

/* Context for plan generator.
 */
 
struct plangen {

  // Every cell reachable from the initial checkpoint is covered by a box.
  struct box *boxv;
  int boxc,boxa;
  
  // Every edge is listed twice, and the list is sorted by (a,b).
  struct edge {
    struct box *a,*b; // WEAK; points into (boxv).
  } *edgev;
  int edgec,edgea;
  
  int vehicle;
  uint32_t solid;
};

static void plangen_cleanup(struct plangen *plangen) {
  if (plangen->boxv) free(plangen->boxv);
  if (plangen->edgev) free(plangen->edgev);
}

static void plangen_reset(struct plangen *plangen,int vehicle) {
  plangen->boxc=0;
  plangen->edgec=0;
  plangen->vehicle=vehicle;
  plangen->solid=0;
  switch (vehicle) {
    case NS_vehicle_car: plangen->solid=(1<<NS_physics_solid)|(1<<NS_physics_water)|(1<<NS_physics_bumpy); break;
    case NS_vehicle_boat: plangen->solid=(1<<NS_physics_vacant)|(1<<NS_physics_bumpy)|(1<<NS_physics_solid); break;
    case NS_vehicle_chopper: plangen->solid=(1<<NS_physics_solid); break;
    case CAR_WITH_BUMPIES: plangen->solid=(1<<NS_physics_solid)|(1<<NS_physics_water); break;
  }
}

/* Is a given cell or box passable?
 * Valid coords only, please.
 */
 
static inline int plangen_passable_cell(const struct plangen *plangen,int x,int y) {
  uint8_t physics=g.physics[g.map[y*g.mapw+x]];
  if (plangen->solid&(1<<physics)) return 0;
  return 1;
}

static inline int plangen_passable(const struct plangen *plangen,int x,int y,int w,int h) {
  const uint8_t *maprow=g.map+y*g.mapw+x;
  for (;h-->0;maprow+=g.mapw) {
    const uint8_t *mapp=maprow;
    int xi=w;
    for (;xi-->0;mapp++) {
      uint8_t physics=g.physics[*mapp];
      if (plangen->solid&(1<<physics)) return 0;
    }
  }
  return 1;
}

/* Nonzero if this cell is already covered by a box.
 * This strikes me as very dirty. Might be better to maintain a bitmap of visited cells in addition to the box list?
 * But not going to over-optimize for now.
 */
 
static int plangen_boxed(const struct plangen *plangen,int x,int y) {
  const struct box *box=plangen->boxv;
  int i=plangen->boxc;
  for (;i-->0;box++) {
    if (x<box->x) continue;
    if (y<box->y) continue;
    if (x>=box->x+box->w) continue;
    if (y>=box->y+box->h) continue;
    return 1;
  }
  return 0;
}

/* Add boxes recursively.
 * Caller asserts that (x,y,w,h) is a valid box and not already known.
 * It's a box rather than a point as a wee optimization: When you find a stretch of passables, you can consume the whole stretch.
 */
 
static int plangen_add_boxes(struct plangen *ctx,int x,int y,int w,int h) {
  if ((w<1)||(h<1)) return -1;
  if ((x<0)||(x+w>g.mapw)||(y<0)||(y+h>g.maph)) return -1;
  
  /* Extend edges as far as they'll go.
   * If it doesn't end up at least 2 meters on both axes, forget it.
   */
  while ((x>0)&&plangen_passable(ctx,x-1,y,1,h)) { x--; w++; }
  while ((y>0)&&plangen_passable(ctx,x,y-1,w,1)) { y--; h++; }
  while ((x+w<g.mapw)&&plangen_passable(ctx,x+w,y,1,h)) w++;
  while ((y+h<g.maph)&&plangen_passable(ctx,x,y+h,w,1)) h++;
  if ((w<2)||(h<2)) return 0;
  
  /* Add to the list.
   */
  if (ctx->boxc>=ctx->boxa) {
    int na=ctx->boxa+64;
    if (na>INT_MAX/sizeof(struct box)) return -1;
    void *nv=realloc(ctx->boxv,sizeof(struct box)*na);
    if (!nv) return -1;
    ctx->boxv=nv;
    ctx->boxa=na;
  }
  ctx->boxv[ctx->boxc++]=(struct box){x,y,w,h};
  
  /* Trace outside the edges.
   * If we find something passable, spawn a new box there.
   * "(c>=2)" instead of 1 because all vehicles are wider than a cell. Don't trace alleys and such, if we can't fit in them.
   */
  if (x>0) {
    int yi=y,ystop=y+h;
    while (yi<ystop) {
      int c=0;
      while ((yi+c<ystop)&&plangen_passable_cell(ctx,x-1,yi+c)&&!plangen_boxed(ctx,x-1,yi+c)) c++;
      if ((c>=2)&&(plangen_add_boxes(ctx,x-1,yi,1,c)<0)) return -1;
      yi+=c+1;
    }
  }
  if (y>0) {
    int xi=x,xstop=x+w;
    while (xi<xstop) {
      int c=0;
      while ((xi+c<xstop)&&plangen_passable_cell(ctx,xi+c,y-1)&&!plangen_boxed(ctx,xi+c,y-1)) c++;
      if ((c>=2)&&(plangen_add_boxes(ctx,xi,y-1,c,1)<0)) return -1;
      xi+=c+1;
    }
  }
  if (x+w<g.mapw) {
    int yi=y,ystop=y+h;
    while (yi<ystop) {
      int c=0;
      while ((yi+c<ystop)&&plangen_passable_cell(ctx,x+w,yi+c)&&!plangen_boxed(ctx,x+w,yi+c)) c++;
      if ((c>=2)&&(plangen_add_boxes(ctx,x+w,yi,1,c)<0)) return -1;
      yi+=c+1;
    }
  }
  if (y+h<g.maph) {
    int xi=x,xstop=x+w;
    while (xi<xstop) {
      int c=0;
      while ((xi+c<xstop)&&plangen_passable_cell(ctx,xi+c,y+h)&&!plangen_boxed(ctx,xi+c,y+h)) c++;
      if ((c>=2)&&(plangen_add_boxes(ctx,xi,y+h,c,1)<0)) return -1;
      xi+=c+1;
    }
  }
  
  return 0;
}

/* Build up (edgev) from (boxv).
 */
 
static int plangen_find_edges(struct plangen *ctx) {
  // Do iterate the entire list at both levels. We want to capture every edge twice, and doing it this way spares us the trouble of inserting.
  struct box *a=ctx->boxv;
  int ai=ctx->boxc;
  for (;ai-->0;a++) {
    struct box *b=ctx->boxv;
    int bi=ctx->boxc;
    for (;bi-->0;b++) {
      if (a==b) continue; // Boxes do overlap themselves of course, but belaboring that point would be in poor taste.
      if (a->x+a->w<=b->x) continue;
      if (b->x+b->w<=a->x) continue;
      if (a->y+a->h<=b->y) continue;
      if (b->y+b->h<=a->y) continue;
      
      if (ctx->edgec>=ctx->edgea) {
        int na=ctx->edgea+64;
        if (na>INT_MAX/sizeof(struct edge)) return -1;
        void *nv=realloc(ctx->edgev,sizeof(struct edge)*na);
        if (!nv) return -1;
        ctx->edgev=nv;
        ctx->edgea=na;
      }
      ctx->edgev[ctx->edgec++]=(struct edge){a,b};
    }
  }
  return 0;
}

/* Return index of the first edge where (box) is (a).
 * Never returns negative. If (>=ctx->edgec) or (a!=edge.a), there are no edges for this box.
 */
 
static int plangen_edgev_search(const struct plangen *ctx,const struct box *a) {
  int lo=0,hi=ctx->edgec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct edge *q=ctx->edgev+ck;
         if (a<q->a) hi=ck;
    else if (a>q->a) lo=ck+1;
    else {
      while ((ck>lo)&&(q[-1].a==a)) { ck--; q--; }
      return ck;
    }
  }
  return lo;
}

/* Add boxes to (boxpath) until it touches (checkpoint).
 * If we return successfully, we've found the shortest path.
 * Caller must prepare (boxpath) with at least one box.
 */
 
static int plangen_complete_boxpath(struct boxpath *boxpath,struct plangen *ctx,const struct checkpoint *checkpoint) {

  /* Ensure (boxpath) is not empty, and capture its last box.
   * If that start box overlaps (checkpoint), we're done.
   */
  int c0=boxpath->c;
  if (c0<1) return -1;
  struct box *startbox=boxpath->v[c0-1];
  if (
    (startbox->x<checkpoint->x+checkpoint->w)&&
    (startbox->y<checkpoint->y+checkpoint->h)&&
    (checkpoint->x<startbox->x+startbox->w)&&
    (checkpoint->y<startbox->y+startbox->h)
  ) return 0;
  
  /* Visit each edge of (startbox) and recur to build out the whole path.
   * Important: Skip boxes already present in (boxpath). Otherwise we'd loop forever.
   * Record the shortest length we find.
   */
  struct box *bestb=0;
  int bestlen=0;
  int edgep=plangen_edgev_search(ctx,startbox);
  for (;(edgep<ctx->edgec)&&(ctx->edgev[edgep].a==startbox);edgep++) {
    struct box *b=ctx->edgev[edgep].b;
    if (boxpath_has(boxpath,b)) continue;
    if (boxpath_append(boxpath,b)<0) return -1;
    if (plangen_complete_boxpath(boxpath,ctx,checkpoint)>=0) {
      if (!bestb||(boxpath->c<bestlen)) {
        bestb=b;
        bestlen=boxpath->c;
      }
    }
    boxpath->c=c0; // Back out this check, ready for the next.
  }
  if (!bestb) return -1; // Can't get there from here!
  
  /* There's a pretty good chance that the best path we found is still present in (boxpath->v).
   * If so, bump the count back up and declare victory.
   */
  if (boxpath->v[boxpath->c]==bestb) {
    boxpath->c=bestlen;
    return 0;
  }
  
  /* Repeat the recursive search at (bestb).
   */
  if (boxpath_append(boxpath,bestb)<0) return -1;
  if (plangen_complete_boxpath(boxpath,ctx,checkpoint)<0) return -1;
  return 0;
}

/* Build up a boxpath from one checkpoint to another.
 * Fails if no path exists, but that shouldn't be possible.
 * (boxpath) must initially be empty.
 */
 
static int plangen_find_boxpath(struct boxpath *boxpath,struct plangen *ctx,const struct checkpoint *a,const struct checkpoint *b) {
  struct boxpath tmp={0};
  struct box *box=ctx->boxv;
  int i=ctx->boxc;
  for (;i-->0;box++) {
    if (box->x+box->w<=a->x) continue;
    if (box->y+box->h<=a->y) continue;
    if (a->x+a->w<=box->x) continue;
    if (a->y+a->h<=box->y) continue;
    
    // (box) touches (a). Rebuild (tmp) from here to reach (b).
    tmp.c=0;
    if (
      (boxpath_append(&tmp,box)<0)||
      (plangen_complete_boxpath(&tmp,ctx,b)<0)
    ) {
      boxpath_cleanup(&tmp);
      return -1;
    }
    
    // If (boxpath) is empty or (tmp) is shorter, swap them.
    if (!boxpath->c||(tmp.c<boxpath->c)) boxpath_swap(&tmp,boxpath);
  }
  boxpath_cleanup(&tmp);
  if (!boxpath->c) return -1; // Oh no we didn't find one
  return 0;
}

/* Add one point to the global plan, at the intersection between these two boxes.
 */
 
static int plangen_add_point(struct plangen *ctx,const struct box *a,const struct box *b) {
  if (a==b) return 0; // This happens at boxpath edges.
  int ar=a->x+a->w;
  int br=b->x+b->w;
  int ab=a->y+a->h;
  int bb=b->y+b->h;
  int ul=(a->x>b->x)?a->x:b->x;
  int ut=(a->y>b->y)?a->y:b->y;
  int ur=(ar<br)?ar:br;
  int ub=(ab<bb)?ab:bb;
  if ((ul>=ur)||(ut>=ub)) return -1; // (a,b) don't overlap!
  double x=ul+((ur-ul)*0.5);
  double y=ut+((ub-ut)*0.5);
  
  /* So far (x,y) is the center of the intersection between the two boxes.
   * That's a fair default and we could just leave it there.
   * But autopilots won't react until they're very close to the target point.
   * At this default, they'll be strongly inclined to overshoot and crash into the far wall.
   * So. If we can guess which side of (a) they're approaching from, bind to the edge of the intersection instead.
   *XXX I think this won't be necessary, once I change from point-oriented to line-oriented navigation.
  #define CLOSE(aa,bb) ({ \
    int _close=0; \
    int _d=(aa)-(bb); \
    if ((_d>=-2)&&(_d<=2)) _close=1; \
    (_close); \
  })
  int al=a->x,at=a->y,bl=b->x,bt=b->y;
       if ( CLOSE(al,bl)&&!CLOSE(ar,br)) x=ur;
  else if (!CLOSE(al,bl)&& CLOSE(ar,br)) x=ul;
  else if ( CLOSE(at,bt)&&!CLOSE(ab,bb)) y=ub;
  else if (!CLOSE(at,bt)&& CLOSE(ab,bb)) y=ut;
  #undef CLOSE
  /**/
  
  if (g.planc>=g.plana) {
    int na=g.plana+32;
    if (na>INT_MAX/sizeof(struct plan)) return -1;
    void *nv=realloc(g.planv,sizeof(struct plan)*na);
    if (!nv) return -1;
    g.planv=nv;
    g.plana=na;
  }
  g.planv[g.planc++]=(struct plan){x,y};
  return 0;
}

/* Having selected a set of boxes for the next leg of the journey, add them to the final plan.
 * (prev) is optional. If present, we'll start by emitting a point for where it meets (boxpath).
 */
 
static int plangen_add_boxpath(struct plangen *ctx,struct boxpath *boxpath,struct box *prev) {
  if (boxpath->c<1) return -1;
  if (prev) {
    if (plangen_add_point(ctx,prev,boxpath->v[0])<0) return -1;
  }
  struct box **p=boxpath->v;
  int i=boxpath->c-1;
  for (;i-->0;p++) {
    if (plangen_add_point(ctx,p[0],p[1])<0) return -1;
  }
  return 0;
}

/* Prepare plan in context.
 * Caller must plangen_reset() first.
 */
 
static int plangen_inner(struct plangen *ctx) {

  /* Discover the boxes, seeding from the center of the first checkpoint.
   * Don't seed with the entire first checkpoint, because it might contain sidewalks.
   */
  struct checkpoint *cp=g.checkpointv;
  if (plangen_add_boxes(ctx,cp->x+(cp->w>>1),cp->y+(cp->h>>1),1,1)<0) return -1;
  
  /* With the box list final, make a list of edges, ie pairs of overlapping boxes.
   */
  if (plangen_find_edges(ctx)<0) return -1;
  
  /* XXX? Try with the first checkpoint as the first plan point.
   */
  if (g.planc>=g.plana) {
    int na=g.plana+32;
    if (na>INT_MAX/sizeof(struct plan)) return -1;
    void *nv=realloc(g.planv,sizeof(struct plan)*na);
    if (!nv) return -1;
    g.planv=nv;
    g.plana=na;
  }
  g.planv[g.planc++]=(struct plan){cp->x+cp->w*0.5,cp->y+cp->h*0.5};
  
  /* For each adjacent pair of checkpoints, and the wraparound pair,
   * find a set of overlapping boxes that the first box touches the first checkpoint and the last the second.
   * We can add these checkpoint legs to the final path as we find them.
   */
  struct box *firstbox=0,*lastbox=0;
  struct boxpath boxpath={0};
  int cpap=0;
  for (;cpap<g.checkpointc;cpap++) {
    int cpbp=cpap+1;
    if (cpbp>=g.checkpointc) cpbp=0;
    struct checkpoint *cpa=g.checkpointv+cpap;
    struct checkpoint *cpb=g.checkpointv+cpbp;
    boxpath.c=0;
    if (plangen_find_boxpath(&boxpath,ctx,cpa,cpb)<0) {
      fprintf(stderr,"!!! Failed to find box path from checkpoint %d to checkpoint %d\n",cpap,cpbp);
      boxpath_cleanup(&boxpath);
      return -2;
    }
    if (plangen_add_boxpath(ctx,&boxpath,lastbox)<0) {
      boxpath_cleanup(&boxpath);
      return -1;
    }
    if (!firstbox) firstbox=boxpath.v[0];
    lastbox=boxpath.v[boxpath.c-1];
  }
  boxpath_cleanup(&boxpath);
  
  /* The one point where the path begins has not been added, because we didn't have (lastbox) when we added the first boxpath.
   * That works out nicely, actually: We'd rather have this starting point at the very end, so autopilots can start at g.planv[0].
   * So glue together our loop's ends now, after everything.
   *
   * NB: We're inserting extra dummy points that align to each checkpoint, due to our clumsy handling the overlap between boxpaths.
   * It doesn't look like a serious problem; the dummy points are collinear to their neighbors. Not bothering to solve that.
   */
  if (!firstbox||!lastbox) return -1;
  if (plangen_add_point(ctx,lastbox,firstbox)<0) return -1;
  
  return 0;
}

/* Prepare plan.
 * We're driving in the city, so paths will tend to be right angles.
 * That makes things much simpler than generic path finding, we'll take advantage of it.
 */
 
int vehicle_prepare_plan(int vehicle) {
  if (g.checkpointc<2) {
    fprintf(stderr,"%s: At least 2 checkpoints required.\n",__func__);
    return -2;
  }
  g.planc=0;
  struct plangen ctx={0};
  plangen_reset(&ctx,vehicle);
  if (plangen_inner(&ctx)<0) {
    // If it was NS_vehicle_car, try again as CAR_WITH_BUMPIES.
    // We try first with sidewalks and lawns forbidden, but if that fails, allow offroad.
    if (ctx.vehicle==NS_vehicle_car) {
      plangen_reset(&ctx,CAR_WITH_BUMPIES);
      if (plangen_inner(&ctx)>=0) {
        fprintf(stderr,"%s: Succeeded after allowing offroad.\n",__func__);
        plangen_cleanup(&ctx);
        return 0;
      }
    }
    plangen_cleanup(&ctx);
    fprintf(stderr,"%s: Failed to generate plan for vehicle=%d.\n",__func__,vehicle);
    return -1;
  }
  plangen_cleanup(&ctx);
  return 0;
}
