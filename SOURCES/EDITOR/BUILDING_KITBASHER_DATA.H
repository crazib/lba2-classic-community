#ifndef BUILDING_KITBASHER_DATA_H
#define BUILDING_KITBASHER_DATA_H

#include <SYSTEM/ADELINE_TYPES.H>

typedef struct KitbashFacePoint
{
    S32 x;
    S32 y;
    S32 z;
    U16 u;
    U16 v;
} KitbashFacePoint;

typedef struct KitbashFaceTemplate
{
    const char *group;
    S32 lm2_id;
    S32 face_index;
    const char *tag;
    U16 texture;
    S32 point_count;
    KitbashFacePoint p[4];
} KitbashFaceTemplate;

extern const KitbashFaceTemplate *KitbashData_FindFace(const char *tag, S32 ordinal);
extern S32 KitbashData_CountFaces(const char *tag);

#endif /* BUILDING_KITBASHER_DATA_H */
