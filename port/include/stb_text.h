/*
 * stb_text.h - self-contained text rasteriser for the Conspiracy Linux port.
 *
 * Replaces the cairo/pango text path. Renders white-on-black intensity into a
 * caller-provided 8-bit buffer using stb_truetype and the embedded, subsetted
 * fonts (see embedded_fonts.h). No system fonts, no fontconfig.
 *
 * The TU that includes this must first provide the stb_truetype implementation:
 *     #define STB_TRUETYPE_IMPLEMENTATION
 *     #include "stb_truetype.h"
 *     #include "stb_text.h"
 *
 * Faithfulness: em size matches pango_font_description_set_absolute_size (the
 * original passed cell*0.83 as the em). Italic for faces with no real italic
 * file (Tahoma, Impact) is synthesized by shearing, exactly as GDI/Pango did.
 */
#ifndef STB_TEXT_H
#define STB_TEXT_H

#include "stb_truetype.h"
#include "embedded_fonts.h"
#include <math.h>

/* Synthetic-oblique shear for faces with no real italic (matches cairo/Pango's
 * oblique matrix of ~0.2). */
#define STBTEXT_SHEAR 0.20

/* Render `text` white-on-black into gray[W*H] (row-major, caller zeroes it).
 * pen_x/pen_y is the top-left of the text box; px_em is the em size in pixels;
 * spacing is extra pixels added to each advance (GDI SetTextCharacterExtra). */
static void stbtext_render(unsigned char *gray, int W, int H,
                           const char *family, int bold, int italic,
                           double px_em, int spacing, int pen_x, int pen_y,
                           const char *text)
{
    const EmbeddedFace *face = find_embedded_face(family, bold, italic);
    stbtt_fontinfo info;
    int ascent, descent, lineGap, baseline, prev = 0;
    double penx, shear;
    const unsigned char *p;

    if (!face || !stbtt_InitFont(&info, face->data,
                                 stbtt_GetFontOffsetForIndex(face->data, 0)))
        return;

    float scale = stbtt_ScaleForMappingEmToPixels(&info, (float)px_em);
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    baseline = pen_y + (int)(ascent * scale + 0.5f);
    shear = face->shear ? STBTEXT_SHEAR : 0.0;
    penx = pen_x;

    for (p = (const unsigned char *)text; *p; ++p) {
        int cp = *p;                 /* latin1 / ascii codepoint */
        int adv = 0, lsb = 0, g = stbtt_FindGlyphIndex(&info, cp);

        stbtt_GetCodepointHMetrics(&info, cp, &adv, &lsb);
        if (prev)
            penx += stbtt_GetCodepointKernAdvance(&info, prev, cp) * scale;

        if (g && cp != ' ') {
            int gw, gh, xoff, yoff, gx, gy;
            float sx = (float)(penx - floor(penx));
            unsigned char *bmp = stbtt_GetCodepointBitmapSubpixel(
                &info, scale, scale, sx, 0.0f, cp, &gw, &gh, &xoff, &yoff);
            if (bmp) {
                int ox = (int)floor(penx) + xoff;
                int oy = baseline + yoff;
                for (gy = 0; gy < gh; ++gy) {
                    int iy = oy + gy;
                    int sh;
                    if (iy < 0 || iy >= H) continue;
                    sh = (int)(shear * (baseline - iy) + 0.5);
                    for (gx = 0; gx < gw; ++gx) {
                        int ix = ox + gx + sh;
                        unsigned char v;
                        if (ix < 0 || ix >= W) continue;
                        v = bmp[gy * gw + gx];
                        if (v > gray[iy * W + ix]) gray[iy * W + ix] = v;
                    }
                }
                stbtt_FreeBitmap(bmp, 0);
            }
        }
        penx += adv * scale + spacing;
        prev = cp;
    }
}

#endif /* STB_TEXT_H */
