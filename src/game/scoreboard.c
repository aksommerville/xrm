#include "xrm.h"

/* Repr time.
 */
 
int scoreboard_repr_time(char *dst/*9*/,double sf) {
  int ms=(int)(sf*1000.0);
  if (ms<0) ms=0;
  int sec=ms/1000; ms%=1000;
  int min=sec/60; sec%=60;
  if (min>99) {
    min=99;
    sec=99;
    ms=999;
  }
  dst[0]='0'+min/10;
  dst[1]='0'+min%10;
  dst[2]=':';
  dst[3]='0'+sec/10;
  dst[4]='0'+sec%10;
  dst[5]='.';
  dst[6]='0'+ms/100;
  dst[7]='0'+(ms/10)%10;
  dst[8]='0'+ms%10;
  
  // And then lop off leading zeroes...
  if ((min<1)&&(sec<10)) memset(dst,' ',4);
  else if (min<1) memset(dst,' ',3);
  else if (min<10) dst[0]=' ';
  
  return 9;
}

/* Copy and compare.
 */

// Quietly tolerates malformed strings.
static int scoreboard_read_time(const char *src) {
  int min=0,sec=0,ms=0;
  if ((src[0]>='0')&&(src[0]<='9')) min+=(src[0]-'0')*10;
  if ((src[1]>='0')&&(src[1]<='9')) min+=src[1]-'0';
  if ((src[3]>='0')&&(src[3]<='9')) sec+=(src[3]-'0')*10;
  if ((src[4]>='0')&&(src[4]<='9')) sec+=src[4]-'0';
  if ((src[6]>='0')&&(src[6]<='9')) ms+=(src[6]-'0')*100;
  if ((src[7]>='0')&&(src[7]<='9')) ms+=(src[7]-'0')*10;
  if ((src[8]>='0')&&(src[8]<='9')) ms+=(src[8]-'0');
  return min*60000+sec*1000+ms;
}
 
static int scoreboard_cmpcpy_time(char *dst,const char *src) {
  int dstms=scoreboard_read_time(dst);
  int srcms=scoreboard_read_time(src);
  if (!dstms||(srcms<dstms)) {
    memcpy(dst,src,9);
    return 1;
  }
  return 1;
}

static int scoreboard_cmpcpy_rank(int *dst,int src) {
  if (!*dst||(src<*dst)) { *dst=src; return 1; }
  return 0;
}
 
int scoreboard_cmpcpy(struct scoreboard *dst,const struct scoreboard *src) {
  int betterc=0;
  betterc+=scoreboard_cmpcpy_time(dst->overall_time,src->overall_time);
  betterc+=scoreboard_cmpcpy_rank(&dst->overall_rank,src->overall_rank);
  struct racescore *dstrace=dst->racev;
  const struct racescore *srcrace=src->racev;
  int i=RACE_COUNT;
  for (;i-->0;dstrace++,srcrace++) {
    betterc+=scoreboard_cmpcpy_time(dstrace->full,srcrace->full);
    betterc+=scoreboard_cmpcpy_time(dstrace->lap,srcrace->lap);
    betterc+=scoreboard_cmpcpy_rank(&dstrace->rank,srcrace->rank);
  }
  return betterc;
}

/* Load high scores.
 */
 
static int score_load_time(char *dst/*9*/,const char *src,int srcc) {
  while (srcc&&((unsigned char)src[0]<=0x20)) { srcc--; src++; }
  if (srcc>9) return -1;
  char norm[9]="    0.000";
  memcpy(norm+9-srcc,src,srcc);
  if ((norm[0]!=' ')&&((norm[0]<'0')||(norm[0]>'9'))) return -1;
  if ((norm[1]!=' ')&&((norm[1]<'0')||(norm[1]>'9'))) return -1;
  if ((norm[2]!=' ')&&(norm[2]!=':')) return -1;
  if ((norm[3]!=' ')&&((norm[3]<'0')||(norm[3]>'9'))) return -1;
  if ((norm[4]<'0')||(norm[4]>'9')) return -1;
  if (norm[5]!='.') return -1;
  if ((norm[6]<'0')||(norm[6]>'9')) return -1;
  if ((norm[7]<'0')||(norm[7]>'9')) return -1;
  if ((norm[8]<'0')||(norm[8]>'9')) return -1;
  memcpy(dst,norm,9);
  return 0;
}

static int score_load_int(int *dst,const char *src,int srcc) {
  *dst=0;
  for (;srcc-->0;src++) {
    if ((*src<'0')||(*src>'9')) return -1;
    (*dst)*=10;
    (*dst)+=(*src)-'0';
    if (*dst>999999) return -1;
  }
  return 0;
}
 
static int score_load_1(const char *k,int kc,const char *v,int vc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  if (kc<1) return 0;
  if ((kc==2)&&!memcmp(k,"ot",2)) return score_load_time(g.hiscore.overall_time,v,vc);
  if ((kc==2)&&!memcmp(k,"or",2)) return score_load_int(&g.hiscore.overall_rank,v,vc);
  if ((k[0]>='0')&&(k[0]<='9')) {
    int raceid=k[0]-'0';
    if ((raceid>=0)&&(raceid<RACE_COUNT)) {
      struct racescore *race=g.hiscore.racev+raceid;
      switch (k[1]) {
        case 'f': return score_load_time(race->full,v,vc);
        case 'l': return score_load_time(race->lap,v,vc);
        case 'r': return score_load_int(&race->rank,v,vc);
      }
    }
  }
  return 0;
}
 
int score_load() {
  memset(&g.hiscore,0,sizeof(struct scoreboard));
  char src[256];
  int srcc=egg_store_get(src,sizeof(src),"hiscore",7);
  if ((srcc<1)||(srcc>sizeof(src))) return -1;
  int srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (src[srcp]==';') { srcp++; continue; }
    const char *k=src+srcp;
    int kc=0;
    while ((srcp<srcc)&&(src[srcp++]!='=')) kc++;
    while (kc&&((unsigned char)k[kc-1]<=0x20)) kc--;
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    const char *v=src+srcp;
    int vc=0;
    while ((srcp<srcc)&&(src[srcp++]!=';')) vc++;
    while (vc&&((unsigned char)v[vc-1]<=0x20)) vc--;
    if (score_load_1(k,kc,v,vc)<0) {
      memset(&g.hiscore,0,sizeof(struct scoreboard));
      return -1;
    }
  }
  return 1;
}

/* Save high scores.
 */
 
static int score_add_string(char *dst,int dsta,const char *k,int kc,const char *v,int vc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  int dstc=0;
  if (dstc<=dsta-kc) memcpy(dst+dstc,k,kc);
  dstc+=kc;
  if (dstc<dsta) dst[dstc]='=';
  dstc++;
  if (dstc<=dsta-vc) memcpy(dst+dstc,v,vc);
  dstc+=vc;
  if (dstc<dsta) dst[dstc]=';';
  dstc++;
  return dstc;
}

static int score_add_int(char *dst,int dsta,const char *k,int kc,int n) {
  char v[6];
  int vc=0;
  if (n<0) n=0; else if (n>999999) n=999999;
  if (n>=100000) v[vc++]='0'+n/100000;
  if (n>= 10000) v[vc++]='0'+(n/10000)%10;
  if (n>=  1000) v[vc++]='0'+(n/1000)%10;
  if (n>=   100) v[vc++]='0'+(n/100)%10;
  if (n>=    10) v[vc++]='0'+(n/10)%10;
  v[vc++]='0'+n%10;
  return score_add_string(dst,dsta,k,kc,v,vc);
}
 
int score_save() {
  char tmp[256];
  int tmpc=0;
  tmpc+=score_add_string(tmp+tmpc,(int)sizeof(tmp)-tmpc,"ot",2,g.hiscore.overall_time,9);
  tmpc+=score_add_int(tmp+tmpc,(int)sizeof(tmp)-tmpc,"or",2,g.hiscore.overall_rank);
  const struct racescore *race=g.hiscore.racev;
  int i=0;
  for (;i<RACE_COUNT;i++,race++) {
    char key[2]={'0'+i};
    key[1]='f'; tmpc+=score_add_string(tmp+tmpc,(int)sizeof(tmp)-tmpc,key,2,race->full,9);
    key[1]='l'; tmpc+=score_add_string(tmp+tmpc,(int)sizeof(tmp)-tmpc,key,2,race->lap,9);
    key[1]='r'; tmpc+=score_add_int(tmp+tmpc,(int)sizeof(tmp)-tmpc,key,2,race->rank);
  }
  if (tmpc>sizeof(tmp)) return -1;
  return egg_store_set("hiscore",7,tmp,tmpc);
}
