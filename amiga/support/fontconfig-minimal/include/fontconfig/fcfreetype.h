#ifndef FC_FREETYPE_H
#define FC_FREETYPE_H

#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef __cplusplus
extern "C" {
#endif

int FcFreeTypeCharIndex(FT_Face face, FcChar32 ucs4);
FcBool FcFreeTypeQueryFace(FT_Face face, const FcChar8 *file, int id, FcCharSet *blanks, FcPattern **pattern);

#ifdef __cplusplus
}
#endif

#endif /* FC_FREETYPE_H */
