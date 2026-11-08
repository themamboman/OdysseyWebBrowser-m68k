/*
 * fontconfig minimal implementation for AmigaOS 3.x / m68k.
 *
 * Provides the API surface used by WebKit's FreeType font backend
 * (Source/WebCore/platform/graphics/freetype/). Fonts come from a fixed
 * table; the default file can be overridden with the AMIGA_FONT_FILE
 * environment variable, and additional families via AMIGA_FONT_TABLE
 * (lines of "Family|Style|/path/to/file.ttf").
 */

#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/* Reference-counted objects                                            */
/* ------------------------------------------------------------------ */

struct _FcCharSet {
    int ref;
    FcChar32 *chars;
    int n;
    int cap;
};

struct _FcLangSet {
    int ref;
    char **langs;
    int n;
    int cap;
};

struct _FcConfig {
    int ref;
};

struct _FcPattern {
    int ref;
    FcBool weak;
    struct FcEntry {
        char *object;
        FcValue value;
        struct FcEntry *next;
    } *entries;
};

struct _FcObjectSet {
    char **objects;
    int n;
    int cap;
    int all; /* FcDontCare wildcard seen */
};

static void *fc_malloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    return p;
}

static char *fc_strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = fc_malloc(n);
    if (p)
        memcpy(p, s, n);
    return p;
}

static void fc_value_release(FcValue *v)
{
    switch (v->type) {
    case FcTypeString:
        free(v->u.s);
        break;
    case FcTypeMatrix:
        free(v->u.m);
        break;
    case FcTypeCharSet:
        FcCharSetDestroy(v->u.c);
        break;
    case FcTypeLangSet:
        FcLangSetDestroy(v->u.ls);
        break;
    default:
        break;
    }
    v->type = FcTypeUnknown;
}

static FcBool fc_value_copy(FcValue *dst, const FcValue *src)
{
    *dst = *src;
    switch (src->type) {
    case FcTypeString:
        dst->u.s = (FcChar8 *)fc_strdup((const char *)src->u.s);
        return dst->u.s != NULL;
    case FcTypeMatrix:
        dst->u.m = (FcMatrix *)fc_malloc(sizeof(FcMatrix));
        if (!dst->u.m)
            return FcFalse;
        *dst->u.m = *src->u.m;
        return FcTrue;
    case FcTypeCharSet:
        FcCharSetReference(src->u.c);
        return FcTrue;
    case FcTypeLangSet:
        FcLangSetReference(src->u.ls);
        return FcTrue;
    default:
        return FcTrue;
    }
}

/* ------------------------------------------------------------------ */
/* Pattern                                                            */
/* ------------------------------------------------------------------ */

FcPattern *FcPatternCreate(void)
{
    FcPattern *p = (FcPattern *)fc_malloc(sizeof(FcPattern));
    if (!p)
        return NULL;
    p->ref = 1;
    p->weak = FcFalse;
    p->entries = NULL;
    return p;
}

FcPattern *FcPatternReference(FcPattern *pattern)
{
    if (!pattern)
        return NULL;
    pattern->ref++;
    return pattern;
}

void FcPatternDestroy(FcPattern *pattern)
{
    struct FcEntry *e;
    if (!pattern)
        return;
    if (--pattern->ref > 0)
        return;
    e = pattern->entries;
    while (e) {
        struct FcEntry *next = e->next;
        free(e->object);
        fc_value_release(&e->value);
        free(e);
        e = next;
    }
    free(pattern);
}

static FcBool fc_pattern_add(FcPattern *pattern, const char *object, FcValue value)
{
    struct FcEntry *e = (struct FcEntry *)fc_malloc(sizeof(*e));
    if (!e)
        return FcFalse;
    e->object = fc_strdup(object);
    if (!e->object) {
        free(e);
        return FcFalse;
    }
    e->value = value;
    e->next = pattern->entries;
    pattern->entries = e;
    return FcTrue;
}

FcBool FcPatternAddBool(FcPattern *pattern, const char *object, FcBool value)
{
    FcValue v;
    if (!pattern || !object)
        return FcFalse;
    v.type = FcTypeBool;
    v.u.b = value;
    return fc_pattern_add(pattern, object, v);
}

FcBool FcPatternAddDouble(FcPattern *pattern, const char *object, double value)
{
    FcValue v;
    if (!pattern || !object)
        return FcFalse;
    v.type = FcTypeDouble;
    v.u.d = value;
    return fc_pattern_add(pattern, object, v);
}

FcBool FcPatternAddInteger(FcPattern *pattern, const char *object, int value)
{
    FcValue v;
    if (!pattern || !object)
        return FcFalse;
    v.type = FcTypeInteger;
    v.u.i = value;
    return fc_pattern_add(pattern, object, v);
}

FcBool FcPatternAddString(FcPattern *pattern, const char *object, const FcChar8 *value)
{
    FcValue v;
    if (!pattern || !object || !value)
        return FcFalse;
    v.type = FcTypeString;
    v.u.s = (FcChar8 *)fc_strdup((const char *)value);
    if (!v.u.s)
        return FcFalse;
    return fc_pattern_add(pattern, object, v);
}

FcBool FcPatternAddMatrix(FcPattern *pattern, const char *object, const FcMatrix *value)
{
    FcValue v;
    if (!pattern || !object || !value)
        return FcFalse;
    v.type = FcTypeMatrix;
    v.u.m = (FcMatrix *)fc_malloc(sizeof(FcMatrix));
    if (!v.u.m)
        return FcFalse;
    *v.u.m = *value;
    return fc_pattern_add(pattern, object, v);
}

FcBool FcPatternAddCharSet(FcPattern *pattern, const char *object, const FcCharSet *value)
{
    FcValue v;
    if (!pattern || !object || !value)
        return FcFalse;
    v.type = FcTypeCharSet;
    v.u.c = (FcCharSet *)value;
    FcCharSetReference(v.u.c);
    return fc_pattern_add(pattern, object, v);
}

FcBool FcPatternAddLangSet(FcPattern *pattern, const char *object, const FcLangSet *value)
{
    FcValue v;
    if (!pattern || !object || !value)
        return FcFalse;
    v.type = FcTypeLangSet;
    v.u.ls = (FcLangSet *)value;
    FcLangSetReference(v.u.ls);
    return fc_pattern_add(pattern, object, v);
}

FcBool FcPatternDel(FcPattern *pattern, const char *object)
{
    struct FcEntry **prev;
    FcBool removed = FcFalse;
    if (!pattern || !object)
        return FcFalse;
    prev = &pattern->entries;
    while (*prev) {
        struct FcEntry *e = *prev;
        if (strcmp(e->object, object) == 0) {
            *prev = e->next;
            free(e->object);
            fc_value_release(&e->value);
            free(e);
            removed = FcTrue;
        } else {
            prev = &e->next;
        }
    }
    return removed;
}

FcBool FcPatternRemove(FcPattern *pattern, const char *object, int id)
{
    struct FcEntry **prev;
    int i = 0;
    if (!pattern || !object)
        return FcFalse;
    prev = &pattern->entries;
    while (*prev) {
        struct FcEntry *e = *prev;
        if (strcmp(e->object, object) == 0) {
            if (i == id) {
                *prev = e->next;
                free(e->object);
                fc_value_release(&e->value);
                free(e);
                return FcTrue;
            }
            i++;
        }
        prev = &e->next;
    }
    return FcFalse;
}

FcBool FcPatternIsWeak(const FcPattern *pattern)
{
    return pattern ? pattern->weak : FcFalse;
}

static struct FcEntry *fc_pattern_find(const FcPattern *pattern, const char *object, int id)
{
    struct FcEntry *e = pattern->entries;
    int i = 0;
    while (e) {
        if (strcmp(e->object, object) == 0) {
            if (i == id)
                return e;
            i++;
        }
        e = e->next;
    }
    return NULL;
}

FcResult FcPatternGet(const FcPattern *pattern, const char *object, int id, FcValue *value)
{
    struct FcEntry *e;
    if (!pattern || !object || !value)
        return FcResultNoMatch;
    e = fc_pattern_find(pattern, object, id);
    if (!e)
        return FcResultNoMatch;
    *value = e->value;
    return FcResultMatch;
}

FcResult FcPatternGetBool(const FcPattern *pattern, const char *object, int id, FcBool *value)
{
    FcValue v;
    FcResult r = FcPatternGet(pattern, object, id, &v);
    if (r == FcResultMatch) {
        if (v.type != FcTypeBool)
            return FcResultTypeMismatch;
        *value = v.u.b;
    }
    return r;
}

FcResult FcPatternGetDouble(const FcPattern *pattern, const char *object, int id, double *value)
{
    FcValue v;
    FcResult r = FcPatternGet(pattern, object, id, &v);
    if (r == FcResultMatch) {
        if (v.type != FcTypeDouble)
            return FcResultTypeMismatch;
        *value = v.u.d;
    }
    return r;
}

FcResult FcPatternGetInteger(const FcPattern *pattern, const char *object, int id, int *value)
{
    FcValue v;
    FcResult r = FcPatternGet(pattern, object, id, &v);
    if (r == FcResultMatch) {
        if (v.type != FcTypeInteger)
            return FcResultTypeMismatch;
        *value = v.u.i;
    }
    return r;
}

FcResult FcPatternGetString(const FcPattern *pattern, const char *object, int id, FcChar8 **value)
{
    FcValue v;
    FcResult r = FcPatternGet(pattern, object, id, &v);
    if (r == FcResultMatch) {
        if (v.type != FcTypeString)
            return FcResultTypeMismatch;
        *value = v.u.s;
    }
    return r;
}

FcResult FcPatternGetMatrix(const FcPattern *pattern, const char *object, int id, FcMatrix **value)
{
    FcValue v;
    FcResult r = FcPatternGet(pattern, object, id, &v);
    if (r == FcResultMatch) {
        if (v.type != FcTypeMatrix)
            return FcResultTypeMismatch;
        *value = v.u.m;
    }
    return r;
}

FcResult FcPatternGetCharSet(const FcPattern *pattern, const char *object, int id, FcCharSet **value)
{
    FcValue v;
    FcResult r = FcPatternGet(pattern, object, id, &v);
    if (r == FcResultMatch) {
        if (v.type != FcTypeCharSet)
            return FcResultTypeMismatch;
        *value = v.u.c;
    }
    return r;
}

FcResult FcPatternGetLangSet(const FcPattern *pattern, const char *object, int id, FcLangSet **value)
{
    FcValue v;
    FcResult r = FcPatternGet(pattern, object, id, &v);
    if (r == FcResultMatch) {
        if (v.type != FcTypeLangSet)
            return FcResultTypeMismatch;
        *value = v.u.ls;
    }
    return r;
}

FcResult FcPatternGetFTFace(const FcPattern *pattern, const char *object, int id, FT_Face *value)
{
    /* Nothing ever adds an FcTypeFTFace value (no FcPatternAddFTFace here), so this can never
     * match; callers fall back to loading via FC_FILE/FC_INDEX instead. */
    (void)pattern;
    (void)object;
    (void)id;
    (void)value;
    return FcResultNoMatch;
}

FcBool FcPatternEqual(const FcPattern *a, const FcPattern *b)
{
    struct FcEntry *e;
    int na = 0, nb = 0;
    if (a == b)
        return FcTrue;
    if (!a || !b)
        return FcFalse;
    for (e = a->entries; e; e = e->next)
        na++;
    for (e = b->entries; e; e = e->next)
        nb++;
    if (na != nb)
        return FcFalse;
    for (e = a->entries; e; e = e->next) {
        struct FcEntry *f = fc_pattern_find(b, e->object, 0);
        /* match every occurrence of this object in a against b */
        int id = 0;
        FcValue va, vb;
        struct FcEntry *ea = a->entries;
        /* count how many of a's entries precede e with same object */
        for (ea = a->entries; ea != e; ea = ea->next)
            if (strcmp(ea->object, e->object) == 0)
                id++;
        if (!fc_pattern_find(b, e->object, id))
            return FcFalse;
        va = e->value;
        FcPatternGet(b, e->object, id, &vb);
        if (va.type != vb.type)
            return FcFalse;
        switch (va.type) {
        case FcTypeString:
            if (strcmp((const char *)va.u.s, (const char *)vb.u.s) != 0)
                return FcFalse;
            break;
        case FcTypeInteger:
            if (va.u.i != vb.u.i)
                return FcFalse;
            break;
        case FcTypeDouble:
            if (va.u.d != vb.u.d)
                return FcFalse;
            break;
        case FcTypeBool:
            if (va.u.b != vb.u.b)
                return FcFalse;
            break;
        case FcTypeMatrix:
            if (memcmp(va.u.m, vb.u.m, sizeof(FcMatrix)) != 0)
                return FcFalse;
            break;
        case FcTypeCharSet:
            if (va.u.c != vb.u.c &&
                (va.u.c->n != vb.u.c->n ||
                 memcmp(va.u.c->chars, vb.u.c->chars, va.u.c->n * sizeof(FcChar32)) != 0))
                return FcFalse;
            break;
        case FcTypeLangSet:
            if (va.u.ls != vb.u.ls &&
                (va.u.ls->n != vb.u.ls->n ||
                 memcmp(va.u.ls->langs, vb.u.ls->langs, va.u.ls->n * sizeof(char *)) != 0))
                return FcFalse;
            break;
        default:
            return FcFalse;
        }
    }
    return FcTrue;
}

FcPattern *FcPatternDuplicate(const FcPattern *pattern)
{
    FcPattern *p;
    struct FcEntry *e;
    if (!pattern)
        return NULL;
    p = FcPatternCreate();
    if (!p)
        return NULL;
    for (e = pattern->entries; e; e = e->next) {
        FcValue v;
        if (!fc_value_copy(&v, &e->value)) {
            FcPatternDestroy(p);
            return NULL;
        }
        if (!fc_pattern_add(p, e->object, v)) {
            fc_value_release(&v);
            FcPatternDestroy(p);
            return NULL;
        }
    }
    p->weak = pattern->weak;
    return p;
}

FcPattern *FcPatternFilter(const FcPattern *pattern, const FcObjectSet *object_set)
{
    FcPattern *p;
    struct FcEntry *e;
    if (!pattern)
        return NULL;
    p = FcPatternCreate();
    if (!p)
        return NULL;
    for (e = pattern->entries; e; e = e->next) {
        int keep = object_set && object_set->all;
        int i;
        if (!keep && object_set) {
            for (i = 0; i < object_set->n; i++) {
                if (object_set->objects[i] && strcmp(object_set->objects[i], e->object) == 0) {
                    keep = 1;
                    break;
                }
            }
        }
        if (keep) {
            FcValue v;
            if (fc_value_copy(&v, &e->value))
                fc_pattern_add(p, e->object, v);
        }
    }
    return p;
}

FcChar8 *FcPatternFormat(FcPattern *pattern, const FcChar8 *format)
{
    const FcChar8 *f = format;
    char *out = NULL;
    size_t len = 0, cap = 0;

    if (!format)
        return NULL;
    while (*f) {
        if (*f == '%' && f[1] == '{') {
            const FcChar8 *end = strchr((const char *)f + 2, '}');
            char name[128];
            size_t nlen;
            const char *val = "";
            if (!end)
                break;
            nlen = end - (f + 2);
            if (nlen >= sizeof(name))
                nlen = sizeof(name) - 1;
            memcpy(name, f + 2, nlen);
            name[nlen] = '\0';
            if (pattern) {
                FcValue v;
                if (FcPatternGet(pattern, name, 0, &v) == FcResultMatch &&
                    v.type == FcTypeString)
                    val = (const char *)v.u.s;
            }
            nlen = strlen(val);
            if (len + nlen >= cap) {
                cap = (len + nlen + 64) * 2;
                out = (char *)realloc(out, cap);
                if (!out)
                    return NULL;
            }
            memcpy(out + len, val, nlen);
            len += nlen;
            f = end + 1;
        } else {
            if (len + 1 >= cap) {
                cap = (len + 64) * 2;
                out = (char *)realloc(out, cap);
                if (!out)
                    return NULL;
            }
            out[len++] = *f++;
        }
    }
    if (len + 1 >= cap) {
        cap = len + 64;
        out = (char *)realloc(out, cap);
        if (!out)
            return NULL;
    }
    out[len] = '\0';
    return (FcChar8 *)out;
}

/* ------------------------------------------------------------------ */
/* ObjectSet / FontSet                                                */
/* ------------------------------------------------------------------ */

FcObjectSet *FcObjectSetCreate(void)
{
    FcObjectSet *os = (FcObjectSet *)fc_malloc(sizeof(FcObjectSet));
    if (!os)
        return NULL;
    os->objects = NULL;
    os->n = os->cap = 0;
    os->all = 0;
    return os;
}

void FcObjectSetDestroy(FcObjectSet *object_set)
{
    int i;
    if (!object_set)
        return;
    for (i = 0; i < object_set->n; i++)
        free(object_set->objects[i]);
    free(object_set->objects);
    free(object_set);
}

#include <stdarg.h>

FcObjectSet *FcObjectSetBuild(const char *first, ...)
{
    va_list ap;
    FcObjectSet *os;
    const char *s;
    os = FcObjectSetCreate();
    if (!os)
        return NULL;
    if (first) {
        if ((intptr_t)first == (intptr_t)FcDontCare)
            os->all = 1;
        else {
            os->objects = (char **)fc_malloc(sizeof(char *));
            os->objects[0] = fc_strdup(first);
            os->n = 1;
            os->cap = 1;
        }
    }
    va_start(ap, first);
    while ((s = va_arg(ap, const char *)) != NULL) {
        if ((intptr_t)s == (intptr_t)FcDontCare) {
            os->all = 1;
            continue;
        }
        if (os->n == os->cap) {
            os->cap = os->cap ? os->cap * 2 : 4;
            os->objects = (char **)realloc(os->objects, os->cap * sizeof(char *));
        }
        os->objects[os->n++] = fc_strdup(s);
    }
    va_end(ap);
    return os;
}

FcFontSet *FcFontSetCreate(void)
{
    FcFontSet *fs = (FcFontSet *)fc_malloc(sizeof(FcFontSet));
    if (!fs)
        return NULL;
    fs->nfont = 0;
    fs->fonts = NULL;
    return fs;
}

void FcFontSetDestroy(FcFontSet *font_set)
{
    int i;
    if (!font_set)
        return;
    for (i = 0; i < font_set->nfont; i++)
        FcPatternDestroy(font_set->fonts[i]);
    free(font_set->fonts);
    free(font_set);
}

FcBool FcFontSetAdd(FcFontSet *font_set, FcPattern *pattern)
{
    FcPattern **fonts;
    if (!font_set || !pattern)
        return FcFalse;
    fonts = (FcPattern **)realloc(font_set->fonts,
                                  (font_set->nfont + 1) * sizeof(FcPattern *));
    if (!fonts)
        return FcFalse;
    font_set->fonts = fonts;
    font_set->fonts[font_set->nfont++] = pattern;
    return FcTrue;
}

/* ------------------------------------------------------------------ */
/* CharSet / LangSet                                                  */
/* ------------------------------------------------------------------ */

FcCharSet *FcCharSetCreate(void)
{
    FcCharSet *cs = (FcCharSet *)fc_malloc(sizeof(FcCharSet));
    if (!cs)
        return NULL;
    cs->ref = 1;
    cs->chars = NULL;
    cs->n = cs->cap = 0;
    return cs;
}

FcCharSet *FcCharSetReference(FcCharSet *char_set)
{
    if (!char_set)
        return NULL;
    char_set->ref++;
    return char_set;
}

void FcCharSetDestroy(FcCharSet *char_set)
{
    if (!char_set)
        return;
    if (--char_set->ref > 0)
        return;
    free(char_set->chars);
    free(char_set);
}

FcBool FcCharSetAddChar(FcCharSet *char_set, FcChar32 ucs4)
{
    int i;
    if (!char_set)
        return FcFalse;
    for (i = 0; i < char_set->n; i++)
        if (char_set->chars[i] == ucs4)
            return FcTrue;
    if (char_set->n == char_set->cap) {
        char_set->cap = char_set->cap ? char_set->cap * 2 : 32;
        char_set->chars = (FcChar32 *)realloc(char_set->chars,
                                              char_set->cap * sizeof(FcChar32));
        if (!char_set->chars)
            return FcFalse;
    }
    char_set->chars[char_set->n++] = ucs4;
    return FcTrue;
}

FcBool FcCharSetHasChar(const FcCharSet *char_set, FcChar32 ucs4)
{
    int i;
    if (!char_set)
        return FcFalse;
    for (i = 0; i < char_set->n; i++)
        if (char_set->chars[i] == ucs4)
            return FcTrue;
    return FcFalse;
}

FcLangSet *FcLangSetCreate(void)
{
    FcLangSet *ls = (FcLangSet *)fc_malloc(sizeof(FcLangSet));
    if (!ls)
        return NULL;
    ls->ref = 1;
    ls->langs = NULL;
    ls->n = ls->cap = 0;
    return ls;
}

FcLangSet *FcLangSetReference(FcLangSet *lang_set)
{
    if (!lang_set)
        return NULL;
    lang_set->ref++;
    return lang_set;
}

void FcLangSetDestroy(FcLangSet *lang_set)
{
    int i;
    if (!lang_set)
        return;
    if (--lang_set->ref > 0)
        return;
    for (i = 0; i < lang_set->n; i++)
        free(lang_set->langs[i]);
    free(lang_set->langs);
    free(lang_set);
}

FcBool FcLangSetAdd(FcLangSet *lang_set, const FcChar8 *lang)
{
    if (!lang_set || !lang)
        return FcFalse;
    if (lang_set->n == lang_set->cap) {
        lang_set->cap = lang_set->cap ? lang_set->cap * 2 : 4;
        lang_set->langs = (char **)realloc(lang_set->langs,
                                           lang_set->cap * sizeof(char *));
        if (!lang_set->langs)
            return FcFalse;
    }
    lang_set->langs[lang_set->n++] = fc_strdup((const char *)lang);
    return FcTrue;
}

FcLangResult FcLangSetHasLang(const FcLangSet *lang_set, const FcChar8 *lang)
{
    int i;
    if (!lang_set || !lang)
        return FcLangDifferentLang;
    for (i = 0; i < lang_set->n; i++) {
        if (strcasecmp(lang_set->langs[i], (const char *)lang) == 0)
            return FcLangEqual;
        /* prefix match: "en_US" vs "en" */
        if (strncasecmp(lang_set->langs[i], (const char *)lang, strlen(lang)) == 0 &&
            (lang_set->langs[i][strlen(lang)] == '_' ||
             lang_set->langs[i][strlen(lang)] == '\0'))
            return FcLangDifferentCountry;
    }
    return FcLangDifferentLang;
}

FcBool FcLangSetEqual(const FcLangSet *a, const FcLangSet *b)
{
    int i, j;
    if (a == b)
        return FcTrue;
    if (!a || !b || a->n != b->n)
        return FcFalse;
    for (i = 0; i < a->n; i++) {
        FcBool found = FcFalse;
        for (j = 0; j < b->n; j++)
            if (strcasecmp(a->langs[i], b->langs[j]) == 0) {
                found = FcTrue;
                break;
            }
        if (!found)
            return FcFalse;
    }
    return FcTrue;
}

void FcMatrixInit(FcMatrix *matrix)
{
    if (!matrix)
        return;
    matrix->xx = 1.0;
    matrix->xy = 0.0;
    matrix->yx = 0.0;
    matrix->yy = 1.0;
}

void FcMatrixMultiply(FcMatrix *result, const FcMatrix *a, const FcMatrix *b)
{
    FcMatrix r;
    if (!result || !a || !b)
        return;
    r.xx = a->xx * b->xx + a->xy * b->yx;
    r.xy = a->xx * b->xy + a->xy * b->yy;
    r.yx = a->yx * b->xx + a->yy * b->yx;
    r.yy = a->yx * b->xy + a->yy * b->yy;
    *result = r;
}

/* ------------------------------------------------------------------ */
/* Config                                                             */
/* ------------------------------------------------------------------ */

static int fc_initialized = 0;

FcBool FcInit(void)
{
    fc_initialized = 1;
    return FcTrue;
}

int FcVersion(void)
{
    return 21402; /* report 2.14.2 for compatibility */
}

FcConfig *FcConfigCreate(void)
{
    FcConfig *c = (FcConfig *)fc_malloc(sizeof(FcConfig));
    if (!c)
        return NULL;
    c->ref = 1;
    return c;
}

FcConfig *FcConfigReference(FcConfig *config)
{
    if (!config)
        return NULL;
    config->ref++;
    return config;
}

/* Lazily-created singleton "current config" -- there's no real config-file lifecycle to track
 * here (fixed family table), so this is always considered current once it exists. */
static FcConfig *fc_current_config = NULL;

FcConfig *FcConfigGetCurrent(void)
{
    if (!fc_current_config)
        fc_current_config = FcConfigCreate();
    return fc_current_config;
}

FcBool FcInitBringUptoDate(void)
{
    return FcTrue;
}

void FcConfigDestroy(FcConfig *config)
{
    if (!config)
        return;
    if (--config->ref > 0)
        return;
    free(config);
}

FcBool FcConfigSubstitute(FcConfig *config, FcPattern *pattern, FcMatchKind kind)
{
    (void)config;
    (void)pattern;
    (void)kind;
    return FcTrue;
}

FcBool FcConfigAppFontAddPath(FcConfig *config, const FcChar8 *path)
{
    (void)config;
    (void)path;
    return FcTrue;
}

FcBool FcConfigAppFontAddFile(FcConfig *config, const FcChar8 *file)
{
    (void)config;
    (void)file;
    return FcTrue;
}

void FcDefaultSubstitute(FcPattern *pattern)
{
    FcValue v;
    if (!pattern)
        return;
#define FC_DEFAULT_ADD(obj, type, val)                          \
    do {                                                        \
        if (FcPatternGet(pattern, obj, 0, &v) == FcResultNoMatch) \
            FcPatternAdd##type(pattern, obj, val);              \
    } while (0)
    FC_DEFAULT_ADD(FC_WEIGHT, Integer, FC_WEIGHT_MEDIUM);
    FC_DEFAULT_ADD(FC_SLANT, Integer, FC_SLANT_ROMAN);
    FC_DEFAULT_ADD(FC_SPACING, Integer, FC_PROPORTIONAL);
    FC_DEFAULT_ADD(FC_OUTLINE, Bool, FcTrue);
    FC_DEFAULT_ADD(FC_SCALABLE, Bool, FcTrue);
    FC_DEFAULT_ADD(FC_ANTIALIAS, Bool, FcTrue);
    FC_DEFAULT_ADD(FC_HINTING, Bool, FcTrue);
#undef FC_DEFAULT_ADD
}

/* ------------------------------------------------------------------ */
/* Font matching                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *family;
    const char *style;
    const char *file;
} FcKnownFont;

/* Overridable defaults: AMIGA_FONT_FILE redirects the default font file. */
static const FcKnownFont fc_known_fonts[] = {
    { "Helvetica", "Regular", "PROGDIR:Fonts/Default.ttf" },
    { "Helvetica", "Bold", "PROGDIR:Fonts/Default-Bold.ttf" },
    { "Helvetica", "Oblique", "PROGDIR:Fonts/Default-Oblique.ttf" },
    { "Arial", "Regular", "PROGDIR:Fonts/Default.ttf" },
    { "Arial", "Bold", "PROGDIR:Fonts/Default-Bold.ttf" },
    { "Times", "Regular", "PROGDIR:Fonts/Default.ttf" },
    { "Times", "Bold", "PROGDIR:Fonts/Default-Bold.ttf" },
    { "Times", "Italic", "PROGDIR:Fonts/Default-Oblique.ttf" },
    { "Courier", "Regular", "PROGDIR:Fonts/Mono.ttf" },
    { "Courier", "Bold", "PROGDIR:Fonts/Mono-Bold.ttf" },
    { "Symbol", "Regular", "PROGDIR:Fonts/Default.ttf" },
    { "Default", "Regular", "PROGDIR:Fonts/Default.ttf" },
    { NULL, NULL, NULL }
};

static void fc_font_file_override(const char **file)
{
    const char *env = getenv("AMIGA_FONT_FILE");
    (void)file;
    if (env && *env)
        *file = env;
}

static int fc_strieq(const char *a, const char *b)
{
    return strcasecmp(a, b) == 0;
}

static int fc_pattern_matches_family(const FcPattern *pattern, const char *family)
{
    FcChar8 *want = NULL;
    if (FcPatternGetString(pattern, FC_FAMILY, 0, &want) != FcResultMatch)
        return 1; /* no family constraint: matches everything */
    return fc_strieq((const char *)want, family);
}

static FcPattern *fc_font_to_pattern(const FcKnownFont *f)
{
    FcPattern *p = FcPatternCreate();
    const char *file = f->file;
    if (!p)
        return NULL;
    fc_font_file_override(&file);
    FcPatternAddString(p, FC_FAMILY, (const FcChar8 *)f->family);
    FcPatternAddString(p, FC_STYLE, (const FcChar8 *)f->style);
    FcPatternAddString(p, FC_FILE, (const FcChar8 *)file);
    FcPatternAddInteger(p, FC_INDEX, 0);
    FcPatternAddInteger(p, FC_WEIGHT,
                        strstr(f->style, "Bold") ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM);
    FcPatternAddInteger(p, FC_SLANT,
                        (strstr(f->style, "Italic") || strstr(f->style, "Oblique"))
                            ? FC_SLANT_ITALIC
                            : FC_SLANT_ROMAN);
    FcPatternAddInteger(p, FC_SPACING,
                        fc_strieq(f->family, "Courier") ? FC_MONO : FC_PROPORTIONAL);
    FcPatternAddBool(p, FC_OUTLINE, FcTrue);
    FcPatternAddBool(p, FC_SCALABLE, FcTrue);
    return p;
}

static FcPattern *fc_best_match(const FcPattern *pattern, FcResult *result)
{
    FcChar8 *want = NULL;
    int i;
    if (result)
        *result = FcResultMatch;
    if (FcPatternGetString(pattern, FC_FAMILY, 0, &want) == FcResultMatch) {
        for (i = 0; fc_known_fonts[i].family; i++)
            if (fc_strieq((const char *)want, fc_known_fonts[i].family))
                return fc_font_to_pattern(&fc_known_fonts[i]);
    }
    return fc_font_to_pattern(&fc_known_fonts[0]);
}

FcPattern *FcFontMatch(FcConfig *config, FcPattern *pattern, FcResult *result)
{
    (void)config;
    if (!pattern) {
        if (result)
            *result = FcResultNoMatch;
        return NULL;
    }
    return fc_best_match(pattern, result);
}

FcPattern *FcFontSetMatch(FcConfig *config, FcFontSet **sets, int nsets, FcPattern *pattern, FcResult *result)
{
    int i, j;
    FcPattern *best = NULL;
    FcChar8 *want = NULL;
    (void)config;
    if (result)
        *result = FcResultMatch;
    if (!sets || nsets <= 0 || !pattern) {
        if (result)
            *result = FcResultNoMatch;
        return NULL;
    }
    if (FcPatternGetString(pattern, FC_FAMILY, 0, &want) == FcResultMatch) {
        for (i = 0; i < nsets && !best; i++) {
            for (j = 0; sets[i] && j < sets[i]->nfont; j++) {
                FcChar8 *fam = NULL;
                if (FcPatternGetString(sets[i]->fonts[j], FC_FAMILY, 0, &fam) == FcResultMatch &&
                    fc_strieq((const char *)fam, (const char *)want)) {
                    best = FcPatternReference(sets[i]->fonts[j]);
                    break;
                }
            }
        }
    }
    if (!best && sets[0] && sets[0]->nfont > 0)
        best = FcPatternReference(sets[0]->fonts[0]);
    if (!best) {
        if (result)
            *result = FcResultNoMatch;
        return NULL;
    }
    return best;
}

FcFontSet *FcFontList(FcConfig *config, FcPattern *pattern, FcObjectSet *object_set)
{
    FcFontSet *fs = FcFontSetCreate();
    int i;
    (void)config;
    if (!fs)
        return NULL;
    for (i = 0; fc_known_fonts[i].family; i++) {
        if (pattern && !fc_pattern_matches_family(pattern, fc_known_fonts[i].family))
            continue;
        {
            FcPattern *full = fc_font_to_pattern(&fc_known_fonts[i]);
            FcPattern *filtered = FcPatternFilter(full, object_set);
            FcPatternDestroy(full);
            if (filtered)
                FcFontSetAdd(fs, filtered);
        }
    }
    return fs;
}

FcPattern *FcFontRenderPrepare(FcConfig *config, FcPattern *pattern, FcPattern *font)
{
    (void)config;
    (void)pattern;
    if (!font)
        return NULL;
    return FcPatternDuplicate(font);
}

/* ------------------------------------------------------------------ */
/* FreeType integration                                               */
/* ------------------------------------------------------------------ */

int FcFreeTypeCharIndex(FT_Face face, FcChar32 ucs4)
{
    if (!face)
        return 0;
    return FT_Get_Char_Index(face, ucs4);
}

FcBool FcFreeTypeQueryFace(FT_Face face, const FcChar8 *file, int id, FcCharSet *blanks, FcPattern **pattern)
{
    FcPattern *p;
    (void)blanks;
    if (!face || !file || !pattern)
        return FcFalse;
    p = FcPatternCreate();
    if (!p)
        return FcFalse;
    FcPatternAddString(p, FC_FILE, file);
    FcPatternAddInteger(p, FC_INDEX, id);
    if (face->family_name)
        FcPatternAddString(p, FC_FAMILY, (const FcChar8 *)face->family_name);
    if (face->style_name)
        FcPatternAddString(p, FC_STYLE, (const FcChar8 *)face->style_name);
    *pattern = p;
    return FcTrue;
}
