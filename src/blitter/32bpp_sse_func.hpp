/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_sse_func.hpp Functions related to SSE 32 bpp blitter. */

#ifndef BLITTER_32BPP_SSE_FUNC_HPP
#define BLITTER_32BPP_SSE_FUNC_HPP

#ifdef WITH_SSE

#if (SSE_VERSION == 2)
#define BLITTER Blitter_32bppSSE2
#define SSE SSE2
#elif (SSE_VERSION == 3)
#define BLITTER Blitter_32bppSSSE3
#define SSE SSE3
#elif (SSE_VERSION == 4)
#define BLITTER Blitter_32bppSSE4
#define SSE SSE4
#endif

/**
 * Draws a sprite to a (screen) buffer. It is templated to allow faster operation.
 *
 * @tparam mode blitter mode
 * @param bp further blitting parameters
 * @param zoom zoom level at which we are drawing
 */
IGNORE_UNINITIALIZED_WARNING_START
template <BlitterMode mode, Blitter_32bppSSE2::ReadMode read_mode, Blitter_32bppSSE2::BlockType bt_last, bool translucent>
inline void BLITTER::Draw (const Blitter::BlitterParams *bp, ZoomLevel zoom)
{
	const byte * const remap = bp->remap;
	Colour *dst_line = (Colour *) bp->dst + bp->top * bp->pitch + bp->left;
	int effective_width = bp->width;

	/* Find where to start reading in the source sprite. */
	const Sprite * const sd = static_cast<const Sprite*> (bp->sprite);
	const SpriteInfo * const si = &sd->infos[zoom];
	const MapValue *src_mv_line = (const MapValue *) &sd->data[si->mv_offset] + bp->skip_top * si->sprite_width;
	const Colour *src_rgba_line = (const Colour *) ((const byte *) &sd->data[si->sprite_offset] + bp->skip_top * si->sprite_line_size);

	if (read_mode != RM_WITH_MARGIN) {
		src_rgba_line += bp->skip_left;
		src_mv_line += bp->skip_left;
	}
	const MapValue *src_mv = src_mv_line;

	/* Load these variables into register before loop. */
#if (SSE_VERSION == 2)
	const __m128i clear_hi    = CLEAR_HIGH_BYTE_MASK;
	#define ALPHA_BLEND_PARAM_1 clear_hi
	#define ALPHA_BLEND_PARAM_2 clear_hi
	#define DARKEN_PARAM_1      tr_nom_base
	#define DARKEN_PARAM_2      tr_nom_base
#else
	const __m128i a_cm        = ALPHA_CONTROL_MASK;
	const __m128i pack_low_cm = PACK_LOW_CONTROL_MASK;
	#define ALPHA_BLEND_PARAM_1 a_cm
	#define ALPHA_BLEND_PARAM_2 pack_low_cm
	#define DARKEN_PARAM_1      a_cm
	#define DARKEN_PARAM_2      tr_nom_base
#endif
	const __m128i tr_nom_base = TRANSPARENT_NOM_BASE;

	for (int y = bp->height; y != 0; y--) {
		Colour *dst = dst_line;
		const Colour *src = src_rgba_line + META_LENGTH;
		if (mode == BM_COLOUR_REMAP || mode == BM_CRASH_REMAP) src_mv = src_mv_line;

		if (read_mode == RM_WITH_MARGIN) {
			assert(bt_last == BT_NONE); // or you must ensure block type is preserved
			src += src_rgba_line[0].data;
			dst += src_rgba_line[0].data;
			if (mode == BM_COLOUR_REMAP || mode == BM_CRASH_REMAP) src_mv += src_rgba_line[0].data;
			const int width_diff = si->sprite_width - bp->width;
			effective_width = bp->width - (int) src_rgba_line[0].data;
			const int delta_diff = (int) src_rgba_line[1].data - width_diff;
			const int new_width = effective_width - delta_diff;
			effective_width = delta_diff > 0 ? new_width : effective_width;
			if (effective_width <= 0) goto next_line;
		}

		switch (mode) {
			default:
				if (!translucent) {
					for (uint x = (uint) effective_width; x > 0; x--) {
						if (src->a) *dst = *src;
						src++;
						dst++;
					}
					break;
				}

				for (uint x = (uint) effective_width / 2; x > 0; x--) {
					__m128i srcABCD = _mm_loadl_epi64((const __m128i*) src);
					__m128i dstABCD = _mm_loadl_epi64((__m128i*) dst);
					_mm_storel_epi64 ((__m128i*) dst, AlphaBlendTwoPixels<SSE> (srcABCD, dstABCD, ALPHA_BLEND_PARAM_1, ALPHA_BLEND_PARAM_2));
					src += 2;
					dst += 2;
				}

				if ((bt_last == BT_NONE && effective_width & 1) || bt_last == BT_ODD) {
					__m128i srcABCD = _mm_cvtsi32_si128(src->data);
					__m128i dstABCD = _mm_cvtsi32_si128(dst->data);
					dst->data = _mm_cvtsi128_si32 (AlphaBlendTwoPixels<SSE> (srcABCD, dstABCD, ALPHA_BLEND_PARAM_1, ALPHA_BLEND_PARAM_2));
				}
				break;

			case BM_COLOUR_REMAP:
#if (SSE_VERSION >= 3)
				for (uint x = (uint) effective_width / 2; x > 0; x--) {
					__m128i srcABCD = _mm_loadl_epi64((const __m128i*) src);
					__m128i dstABCD = _mm_loadl_epi64((__m128i*) dst);
					uint32 mvX2 = *((const uint32 *) src_mv);

					/* Remap colours. */
					if (mvX2 & 0x00FF00FF) {
						#define CMOV_REMAP(m_colour, m_colour_init, m_src, m_m) \
							/* Written so the compiler uses CMOV. */ \
							Colour m_colour = m_colour_init; \
							{ \
							const Colour srcm = (Colour) (m_src); \
							const uint m = (byte) (m_m); \
							const uint r = remap[m]; \
							const Colour cmap = (this->LookupColourInPalette(r).data & 0x00FFFFFF) | (srcm.data & 0xFF000000); \
							m_colour = r == 0 ? m_colour : cmap; \
							m_colour = m != 0 ? m_colour : srcm; \
							}
#ifdef _SQ64
						uint64 srcs = _mm_cvtsi128_si64(srcABCD);
						uint64 remapped_src = 0;
						CMOV_REMAP(c0, 0, srcs, mvX2);
						remapped_src = c0.data;
						CMOV_REMAP(c1, 0, srcs >> 32, mvX2 >> 16);
						remapped_src |= (uint64) c1.data << 32;
						srcABCD = _mm_cvtsi64_si128(remapped_src);
#else
						Colour remapped_src[2];
						CMOV_REMAP(c0, 0, _mm_cvtsi128_si32(srcABCD), mvX2);
						remapped_src[0] = c0.data;
						CMOV_REMAP(c1, 0, src[1], mvX2 >> 16);
						remapped_src[1] = c1.data;
						srcABCD = _mm_loadl_epi64((__m128i*) &remapped_src);
#endif

						if ((mvX2 & 0xFF00FF00) != 0x80008000) srcABCD = AdjustBrightnessOfTwoPixels<SSE> (srcABCD, mvX2);
					}

					/* Blend colours. */
					_mm_storel_epi64 ((__m128i *) dst, AlphaBlendTwoPixels<SSE> (srcABCD, dstABCD, ALPHA_BLEND_PARAM_1, ALPHA_BLEND_PARAM_2));
					dst += 2;
					src += 2;
					src_mv += 2;
				}

				if ((bt_last == BT_NONE && effective_width & 1) || bt_last == BT_ODD) {
#else
				for (uint x = (uint) effective_width; x > 0; x--) {
#endif
					/* In case the m-channel is zero, do not remap this pixel in any way. */
					__m128i srcABCD;
					if (src_mv->m) {
						const uint r = remap[src_mv->m];
						if (r != 0) {
							Colour remapped_colour = AdjustBrightneSSE<SSE> (this->LookupColourInPalette(r), src_mv->v);
							if (src->a == 255) {
								*dst = remapped_colour;
							} else {
								remapped_colour.a = src->a;
								srcABCD = _mm_cvtsi32_si128(remapped_colour.data);
								goto bmcr_alpha_blend_single;
							}
						}
					} else {
						srcABCD = _mm_cvtsi32_si128(src->data);
						if (src->a < 255) {
bmcr_alpha_blend_single:
							__m128i dstABCD = _mm_cvtsi32_si128(dst->data);
							srcABCD = AlphaBlendTwoPixels<SSE> (srcABCD, dstABCD, ALPHA_BLEND_PARAM_1, ALPHA_BLEND_PARAM_2);
						}
						dst->data = _mm_cvtsi128_si32(srcABCD);
					}
#if (SSE_VERSION == 2)
					src_mv++;
					dst++;
					src++;
#endif
				}
				break;

			case BM_TRANSPARENT:
				/* Make the current colour a bit more black, so it looks like this image is transparent. */
				for (uint x = (uint) bp->width / 2; x > 0; x--) {
					__m128i srcABCD = _mm_loadl_epi64((const __m128i*) src);
					__m128i dstABCD = _mm_loadl_epi64((__m128i*) dst);
					_mm_storel_epi64 ((__m128i *) dst, DarkenTwoPixels<SSE> (srcABCD, dstABCD, DARKEN_PARAM_1, DARKEN_PARAM_2));
					src += 2;
					dst += 2;
				}

				if ((bt_last == BT_NONE && bp->width & 1) || bt_last == BT_ODD) {
					__m128i srcABCD = _mm_cvtsi32_si128(src->data);
					__m128i dstABCD = _mm_cvtsi32_si128(dst->data);
					dst->data = _mm_cvtsi128_si32 (DarkenTwoPixels<SSE> (srcABCD, dstABCD, DARKEN_PARAM_1, DARKEN_PARAM_2));
				}
				break;

			case BM_CRASH_REMAP:
				for (uint x = (uint) bp->width; x > 0; x--) {
					if (src_mv->m == 0) {
						if (src->a != 0) {
							uint8 g = MakeDark(src->r, src->g, src->b);
							*dst = ComposeColourRGBA(g, g, g, src->a, *dst);
						}
					} else {
						uint r = remap[src_mv->m];
						if (r != 0) *dst = ComposeColourPANoCheck(this->AdjustBrightness(this->LookupColourInPalette(r), src_mv->v), src->a, *dst);
					}
					src_mv++;
					dst++;
					src++;
				}
				break;

			case BM_BLACK_REMAP:
				for (uint x = (uint) bp->width; x > 0; x--) {
					if (src->a != 0) {
						*dst = Colour(0, 0, 0);
					}
					src_mv++;
					dst++;
					src++;
				}
				break;
		}

next_line:
		if (mode == BM_COLOUR_REMAP || mode == BM_CRASH_REMAP) src_mv_line += si->sprite_width;
		src_rgba_line = (const Colour*) ((const byte*) src_rgba_line + si->sprite_line_size);
		dst_line += bp->pitch;
	}
}
IGNORE_UNINITIALIZED_WARNING_STOP

/**
 * Draws a sprite to a (screen) buffer. Calls adequate templated function.
 *
 * @param bp further blitting parameters
 * @param mode blitter mode
 * @param zoom zoom level at which we are drawing
 */
void BLITTER::Draw (Blitter::BlitterParams *bp, BlitterMode mode, ZoomLevel zoom)
{
	switch (mode) {
		default: {
			if (bp->skip_left != 0 || bp->width <= MARGIN_NORMAL_THRESHOLD) {
bm_normal:
				const BlockType bt_last = (BlockType) (bp->width & 1);
				switch (bt_last) {
					default:     Draw<BM_NORMAL, RM_WITH_SKIP, BT_EVEN, true>(bp, zoom); return;
					case BT_ODD: Draw<BM_NORMAL, RM_WITH_SKIP, BT_ODD, true>(bp, zoom); return;
				}
			} else {
				if (static_cast<const Sprite*>(bp->sprite)->flags & SF_TRANSLUCENT) {
					Draw<BM_NORMAL, RM_WITH_MARGIN, BT_NONE, true>(bp, zoom);
				} else {
					Draw<BM_NORMAL, RM_WITH_MARGIN, BT_NONE, false>(bp, zoom);
				}
				return;
			}
			break;
		}
		case BM_COLOUR_REMAP:
			if (static_cast<const Sprite*>(bp->sprite)->flags & SF_NO_REMAP) goto bm_normal;
			if (bp->skip_left != 0 || bp->width <= MARGIN_REMAP_THRESHOLD) {
				Draw<BM_COLOUR_REMAP, RM_WITH_SKIP, BT_NONE, true>(bp, zoom); return;
			} else {
				Draw<BM_COLOUR_REMAP, RM_WITH_MARGIN, BT_NONE, true>(bp, zoom); return;
			}
		case BM_TRANSPARENT:  Draw<BM_TRANSPARENT, RM_NONE, BT_NONE, true>(bp, zoom); return;
		case BM_CRASH_REMAP:  Draw<BM_CRASH_REMAP, RM_NONE, BT_NONE, true>(bp, zoom); return;
		case BM_BLACK_REMAP:  Draw<BM_BLACK_REMAP, RM_NONE, BT_NONE, true>(bp, zoom); return;
	}
}

#endif /* WITH_SSE */
#endif /* BLITTER_32BPP_SSE_FUNC_HPP */
