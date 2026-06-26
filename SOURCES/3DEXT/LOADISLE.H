#ifndef LOADISLE_H
#define LOADISLE_H

/*--------------------------------------------------------------------------*/
extern S32 InitExternalView(void);
/*--------------------------------------------------------------------------*/
extern void ClearExternalView(void);
/*--------------------------------------------------------------------------*/
extern void LoadIsland(void);
/*--------------------------------------------------------------------------*/
extern T_HQR_HEADER *GetDecorLibraryHQR(S32 library);
/*--------------------------------------------------------------------------*/
extern S32 LoadCube(S32 x, S32 y);
/*--------------------------------------------------------------------------*/
typedef void (*ExteriorFogOverrideFn)(void);
extern void SetExteriorFogOverride(ExteriorFogOverrideFn fn);
/*--------------------------------------------------------------------------*/
typedef S32 (*ExteriorWideViewActiveFn)(void);
extern void SetExteriorWideViewActive(ExteriorWideViewActiveFn fn);
extern S32 ExteriorWideViewActive(void);
/*--------------------------------------------------------------------------*/
extern void InitSeaCube(void);
/*--------------------------------------------------------------------------*/

#endif // LOADISLE_H
