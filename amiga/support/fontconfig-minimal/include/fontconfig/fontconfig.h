#ifndef FC_FONTCONFIG_H
#define FC_FONTCONFIG_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int FcBool;
#define FcFalse 0
#define FcTrue 1
#define FcDontCare (-1)

typedef unsigned char FcChar8;
typedef unsigned short FcChar16;
typedef unsigned int FcChar32;

typedef int FcResult;
typedef int FcType;
typedef int FcMatchKind;
typedef int FcLangResult;

#define FcTypeUnknown 0
#define FcTypeVoid 1
#define FcTypeInteger 2
#define FcTypeDouble 3
#define FcTypeString 4
#define FcTypeBool 5
#define FcTypeMatrix 6
#define FcTypeCharSet 7
#define FcTypeFTFace 8
#define FcTypeLangSet 9
#define FcTypeRange 10

#define FcResultMatch 0
#define FcResultNoMatch 1
#define FcResultTypeMismatch 2
#define FcResultNoId 3
#define FcResultOutOfMemory 4

#define FcMatchPattern 0
#define FcMatchFont 1

#define FcLangDifferentLang 0
#define FcLangDifferentCountry 1
#define FcLangEqual 2

typedef struct _FcPattern FcPattern;
typedef struct _FcObjectSet FcObjectSet;
typedef struct _FcCharSet FcCharSet;
typedef struct _FcLangSet FcLangSet;
typedef struct _FcConfig FcConfig;
typedef struct _FcMatrix FcMatrix;
typedef struct _FcValue FcValue;

struct _FcFontSet {
    int nfont;
    FcPattern **fonts;
};
typedef struct _FcFontSet FcFontSet;

struct _FcMatrix {
    double xx, xy, yx, yy;
};

struct _FcValue {
    FcType type;
    union {
        int i;
        double d;
        FcChar8 *s;
        FcBool b;
        FcMatrix *m;
        FcCharSet *c;
        FcLangSet *ls;
    } u;
};

#define FC_FAMILY "family"
#define FC_STYLE "style"
#define FC_FULLNAME "fullname"
#define FC_SLANT "slant"
#define FC_WEIGHT "weight"
#define FC_SPACING "spacing"
#define FC_FILE "file"
#define FC_INDEX "index"
#define FC_LANG "lang"
#define FC_CHARSET "charset"
#define FC_OUTLINE "outline"
#define FC_SCALABLE "scalable"
#define FC_EMBOLDEN "embolden"
#define FC_PIXEL_SIZE "pixelsize"
#define FC_SIZE "size"
#define FC_ANTIALIAS "antialias"
#define FC_HINTING "hinting"
#define FC_HINT_STYLE "hintstyle"
#define FC_RGBA "rgba"
#define FC_MINSPACE "minspace"
#define FC_NAMELANG "namelang"
#define FC_MATRIX "matrix"
#define FC_DPI "dpi"
#define FC_WIDTH "width"
#define FC_FT_FACE "ftface"
#define FC_AUTOHINT "autohint"
#define FC_VERTICAL_LAYOUT "verticallayout"

#define FC_SLANT_ROMAN 0
#define FC_SLANT_ITALIC 100
#define FC_SLANT_OBLIQUE 110

#define FC_WEIGHT_THIN 0
#define FC_WEIGHT_EXTRALIGHT 40
#define FC_WEIGHT_ULTRALIGHT FC_WEIGHT_EXTRALIGHT
#define FC_WEIGHT_LIGHT 50
#define FC_WEIGHT_DEMILIGHT 55
#define FC_WEIGHT_BOOK 75
#define FC_WEIGHT_REGULAR 80
#define FC_WEIGHT_MEDIUM 80
#define FC_WEIGHT_DEMIBOLD 180
#define FC_WEIGHT_SEMIBOLD FC_WEIGHT_DEMIBOLD
#define FC_WEIGHT_BOLD 200
#define FC_WEIGHT_EXTRABOLD 205
#define FC_WEIGHT_BLACK 210
#define FC_WEIGHT_EXTRABLACK 215
#define FC_WEIGHT_ULTRABLACK FC_WEIGHT_EXTRABLACK

#define FC_PROPORTIONAL 0
#define FC_DUAL 90
#define FC_MONO 100
#define FC_CHARCELL 110

#define FC_RGBA_UNKNOWN 0
#define FC_RGBA_RGB 1
#define FC_RGBA_BGR 2
#define FC_RGBA_VRGB 3
#define FC_RGBA_VBGR 4
#define FC_RGBA_NONE 5

#define FC_HINT_NONE 0
#define FC_HINT_SLIGHT 1
#define FC_HINT_MEDIUM 2
#define FC_HINT_FULL 3

FcBool FcInit(void);
int FcVersion(void);

FcConfig *FcConfigCreate(void);
FcConfig *FcConfigReference(FcConfig *config);
void FcConfigDestroy(FcConfig *config);
FcBool FcConfigSubstitute(FcConfig *config, FcPattern *pattern, FcMatchKind kind);
FcBool FcConfigAppFontAddPath(FcConfig *config, const FcChar8 *path);
FcBool FcConfigAppFontAddFile(FcConfig *config, const FcChar8 *file);
/* No real config-file lifecycle here (fixed family table, see fontconfig.c), so these two are
 * trivial: the "current config" is a lazily-created singleton that's always considered current. */
FcConfig *FcConfigGetCurrent(void);
FcBool FcInitBringUptoDate(void);

void FcDefaultSubstitute(FcPattern *pattern);

FcPattern *FcPatternCreate(void);
FcPattern *FcPatternReference(FcPattern *pattern);
void FcPatternDestroy(FcPattern *pattern);
FcPattern *FcPatternDuplicate(const FcPattern *pattern);
FcBool FcPatternAddBool(FcPattern *pattern, const char *object, FcBool value);
FcBool FcPatternAddDouble(FcPattern *pattern, const char *object, double value);
FcBool FcPatternAddInteger(FcPattern *pattern, const char *object, int value);
FcBool FcPatternAddString(FcPattern *pattern, const char *object, const FcChar8 *value);
FcBool FcPatternAddMatrix(FcPattern *pattern, const char *object, const FcMatrix *value);
FcBool FcPatternAddCharSet(FcPattern *pattern, const char *object, const FcCharSet *value);
FcBool FcPatternAddLangSet(FcPattern *pattern, const char *object, const FcLangSet *value);
FcBool FcPatternDel(FcPattern *pattern, const char *object);
FcBool FcPatternRemove(FcPattern *pattern, const char *object, int id);
FcBool FcPatternEqual(const FcPattern *a, const FcPattern *b);
#define FcPatternEquals FcPatternEqual
FcBool FcPatternIsWeak(const FcPattern *pattern);
FcPattern *FcPatternFilter(const FcPattern *pattern, const FcObjectSet *object_set);
FcChar8 *FcPatternFormat(FcPattern *pattern, const FcChar8 *format);

FcResult FcPatternGet(const FcPattern *pattern, const char *object, int id, FcValue *value);
FcResult FcPatternGetBool(const FcPattern *pattern, const char *object, int id, FcBool *value);
FcResult FcPatternGetDouble(const FcPattern *pattern, const char *object, int id, double *value);
FcResult FcPatternGetInteger(const FcPattern *pattern, const char *object, int id, int *value);
FcResult FcPatternGetString(const FcPattern *pattern, const char *object, int id, FcChar8 **value);
FcResult FcPatternGetMatrix(const FcPattern *pattern, const char *object, int id, FcMatrix **value);
FcResult FcPatternGetCharSet(const FcPattern *pattern, const char *object, int id, FcCharSet **value);
FcResult FcPatternGetLangSet(const FcPattern *pattern, const char *object, int id, FcLangSet **value);
/* Nothing ever stores an FcTypeFTFace value in a pattern here (no FcPatternAddFTFace exists),
 * so this always reports no match -- callers (e.g. cairo-ft-font.c) fall back to FC_FILE/FC_INDEX,
 * which the fixed family table in fontconfig.c does populate. */
#ifndef FREETYPE_H_
typedef struct FT_FaceRec_ *FT_Face;
#endif
FcResult FcPatternGetFTFace(const FcPattern *pattern, const char *object, int id, FT_Face *value);

FcObjectSet *FcObjectSetCreate(void);
void FcObjectSetDestroy(FcObjectSet *object_set);
FcObjectSet *FcObjectSetBuild(const char *first, ...);

FcFontSet *FcFontSetCreate(void);
void FcFontSetDestroy(FcFontSet *font_set);
FcBool FcFontSetAdd(FcFontSet *font_set, FcPattern *pattern);

FcPattern *FcFontMatch(FcConfig *config, FcPattern *pattern, FcResult *result);
FcPattern *FcFontSetMatch(FcConfig *config, FcFontSet **sets, int nsets, FcPattern *pattern, FcResult *result);
FcFontSet *FcFontList(FcConfig *config, FcPattern *pattern, FcObjectSet *object_set);

FcPattern *FcFontRenderPrepare(FcConfig *config, FcPattern *pattern, FcPattern *font);

FcCharSet *FcCharSetCreate(void);
FcCharSet *FcCharSetReference(FcCharSet *char_set);
void FcCharSetDestroy(FcCharSet *char_set);
FcBool FcCharSetAddChar(FcCharSet *char_set, FcChar32 ucs4);
FcBool FcCharSetHasChar(const FcCharSet *char_set, FcChar32 ucs4);

FcLangSet *FcLangSetCreate(void);
FcLangSet *FcLangSetReference(FcLangSet *lang_set);
void FcLangSetDestroy(FcLangSet *lang_set);
FcBool FcLangSetAdd(FcLangSet *lang_set, const FcChar8 *lang);
FcLangResult FcLangSetHasLang(const FcLangSet *lang_set, const FcChar8 *lang);
FcBool FcLangSetEqual(const FcLangSet *a, const FcLangSet *b);

void FcMatrixInit(FcMatrix *matrix);
void FcMatrixMultiply(FcMatrix *result, const FcMatrix *a, const FcMatrix *b);

#ifdef __cplusplus
}
#endif

#endif /* FC_FONTCONFIG_H */
