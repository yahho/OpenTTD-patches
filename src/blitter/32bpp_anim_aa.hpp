/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_anim_aa.hpp A 32 bpp blitter with animation support and supersample antialiasing. */

#ifndef BLITTER_32BPP_ANIM_AA_HPP
#define BLITTER_32BPP_ANIM_AA_HPP

#include "32bpp_optimized.hpp"
#include "../thread/thread.h"
#include "../core/endian_type.hpp"

#define LX2_ABE_ALIGN 4
#define LX2_CONDENSED_AP 1

/** The optimised 32 bpp blitter with palette animation. */
class Blitter_32bppAnimAA FINAL : public Blitter_32bppOptimized {
private:

#if defined(_MSC_VER) || defined(__WATCOMC__)
#pragma pack(push, 1)
#endif

#if LX2_CONDENSED_AP == 1
	union IndexedPixel {
		uint8 data;
		struct {
#if TTD_ENDIAN == TTD_BIG_ENDIAN
			uint8 v : 3;
			uint8 c : 5;
#else
			uint8 c : 5;
			uint8 v : 3;
#endif /* TTD_ENDIAN == TTD_BIG_ENDIAN */
		};
	} GCC_PACK;
	//assert_compile(sizeof(Blitter_32bppAnimAA::IndexedPixel) == 1);
	//assert_compile(PALETTE_ANIM_SIZE < ((1 << 5) - 2));
#else
	union IndexedPixel {
		uint16 data;
		struct {
#if TTD_ENDIAN == TTD_BIG_ENDIAN
			uint8 v, c;
#else
			uint8 c, v;
#endif /* TTD_ENDIAN == TTD_BIG_ENDIAN */
		};
	} GCC_PACK;
	assert_compile(sizeof(Blitter_32bppAnimAA::IndexedPixel) == 2);
#endif

	struct AnimBufferEntry {
		Colour pixel;
		uint8 mask_samples;
		uint8 pixel_samples;
		union IndexedPixel mask[];			///< Expected count of elements is this->aa_anim_slots
	} GCC_PACK;

#if defined(_MSC_VER) || defined(__WATCOMC__)
#pragma pack(pop)
#endif

	AnimBufferEntry *anim_buf;				///< This is anim buffer that would be dynamically-allocated as needed.
	size_t anim_buf_entry_size;				///< Helds the size of the single anim buffer entry, aligned on LX2_ABE_ALIGN boundary
#define LX2_MAX_PS (((int32)1 << (sizeof(this->anim_buf->pixel_samples) * 8))-1)

	int anim_buf_width;  					///< The width of the animation buffer.
	int anim_buf_height; 					///< The height of the animation buffer.
	Palette palette;     					///< The current palette.
#if LX2_CONDENSED_AP == 1
	Colour cached_palette[256];				///< Holds pre-calculated cached copy of the current palette for use in the doPaletteAnimate().
#else
	Colour cached_palette[256*256];			///< Holds pre-calculated cached copy of the current palette for use in the doPaletteAnimate().
#endif

	struct AnimThreadInfo {
		Blitter_32bppAnimAA * self;			///< Pointer to the instance of the blitter this thread belongs to. Used internally by _stDrawAnimThreadProc().
		uint top, left, width, height;		///< Per-thread bounds determining screen area to be processed with by given thread.
		ThreadMutex *th_mutex_in;
	} *anim_ti;								///< Dynamically allocated array that holds thread-specific data for palette-animated threads.

	bool anim_aa_continue_animate;			///< This is a variable holding a "terminate and exit" condition for threaded palette animation.
	bool anim_threaded;						///< This is a flag indicating whether blitter should use threaded palette animation internally of fall back into non-threaded mode.
	uint8 anim_threads_qty;					///< Specifies count of threads used for palette animation, if threading is enabled. Value is undefined for non-threaded mome.
	uint8 aa_level;							///< Stores the dimension of the side of the square matrix that's being used as as sub-samples storage to comprise one destination pixel.
	uint8 aa_anim_slots;					///< Stores the maximum amount of "palette-animated" type sub-samples that we store per single destination pixel.
	ThreadMutex *th_mutex_out;				///< Mutex object that is used to protect some vars like the "jobs left to be done" counter and also acts as a signaling pipe.
	int anim_threads_jobs_active;			///< Count of palette animating threads that are known to be currently active doing their jobs.

	const SpriteLoader::CommonPixel **remap_pixels;
	Colour *temp_pixels;

	/***
	 * Handy proc to blend together sub-samples using alpha-channels as weights.
	 * @param pixels array holding sub-sample to perform blending for
	 * @param count quantity specifier for sub-samples to process
	 * @return RGBA value of the sub-samples blended together in uint32 form as stored in Colour::data
	 */
	inline uint32 BlendPixels(const Colour * pixels, uint count);
	static void _stDrawAnimThreadProc(void * data);					///< "Real" entry point for palette animating threads.
	void DrawAnimThread(const AnimThreadInfo * ath);
	void PaletteAnimateThreaded();
	void doPaletteAnimate(uint left, uint top, uint width, uint height);
	void UpdateCachedPalette(uint8 first_dirty, int16 count_dirty);

public:
	struct SpriteData {
		uint32 offset[ZOOM_LVL_COUNT]; ///< Offsets (from .data) to streams for different zoom levels, and the normal and remap image information.
		uint32 data[];                   ///< Data, all zoomlevels.
	};

	Blitter_32bppAnimAA();

	virtual ~Blitter_32bppAnimAA();

	/* virtual */ void Draw(Blitter::BlitterParams *bp, BlitterMode mode, ZoomLevel zoom);
	/* virtual */ void DrawColourMappingRect(void *dst, int width, int height, PaletteID pal);
	/* virtual */ void SetPixel(void *video, int x, int y, uint8 colour);
	/* virtual */ void SetLine(void *video, int x, int y, uint8 *colours, uint width);
	/* virtual */ void DrawRect(void *video, int width, int height, uint8 colour);
	/* virtual */ void CopyFromBuffer(void *video, const void *src, int width, int height);
	/* virtual */ void CopyToBuffer(const void *video, void *dst, int width, int height);
	/* virtual */ void ScrollBuffer(void *video, int &left, int &top, int &width, int &height, int scroll_x, int scroll_y);
	/* virtual */ int BufferSize(int width, int height);
	/* virtual */ void PaletteAnimate(const Palette &palette);
	/* virtual */ Blitter::PaletteAnimation UsePaletteAnimation();

	/* virtual */ const char *GetName() { return "32bpp-anim-aa"; }
	/* virtual */ int GetBytesPerPixel() { return sizeof(uint32) + this->anim_buf_entry_size; }
	/* virtual */ void PostResize();

	/* virtual */ Sprite *Encode(SpriteLoader::Sprite *sprite, AllocatorProc *allocator);

	/**
	 * Look up the colour in the current palette.
	 * @note This one is to be used in PalleteAnimate and in Draw: depending on video driver palette to use when drawing and palette
	 *       from _cur_palette could differ due to video backend running in a separate thread and having its own local palette copy.
	 */
	inline uint32 LookupColourInPalette(uint index)
	{
		return this->palette.palette[index].data;
	}

	/**
	 * Look up the colour in the current palette.
	 * @note This one is to be used when it is required to access real _cur_palette contents. It might be not thread-safe if used
	 *       in funtions that are known to be called from threads other than main.
	 */
	inline uint32 LookupColourInGfxPalette(uint index)
	{
		return _cur_palette.palette[index].data;
	}

	/**
	 * Compose a colour based on RGB values.
	 * @note Copied here just to make patch compatible both to vanilla 1.2.1 sources and to current (as of r24450) trunk.
	 *       To prevent some questions: copying is done as a part of patch pre-release process, internal development version
	 *       has changes like this done to the base class where they really should be.
	 */
	static inline uint32 ComposeColour(uint a, uint r, uint g, uint b)
	{
		return (((a) << 24) & 0xFF000000) | (((r) << 16) & 0x00FF0000) | (((g) << 8) & 0x0000FF00) | ((b) & 0x000000FF);
	}

	/**
	 * Compose a colour based on RGBA values and the current pixel value.
	 * @note The only difference between this one and MakeGray from base class is ">> 8" instead of "/ 256". They seem to be
	 *       identical but having shift instead of division is what allows gcc's auto-vectorizer to be able to vectorize loops
	 *       calling this function.
	 */
	static inline uint32 ComposeColourRGBANoCheck(uint r, uint g, uint b, uint a, uint32 current)
	{
		uint cr = GB(current, 16, 8);
		uint cg = GB(current, 8,  8);
		uint cb = GB(current, 0,  8);

		/* The 256 is wrong, it should be 255, but 256 is much faster... */
		return ComposeColour(0xFF,
							(((int)(r - cr) * a) >> 8) + cr,
							(((int)(g - cg) * a) >> 8) + cg,
							(((int)(b - cb) * a) >> 8) + cb);
	}

	/**
	 * Compose a colour based on RGBA values and the current pixel value.
	 * Handles fully transparent and solid pixels in a special (faster) way.
	 * @node Copied here so it would use auto-vectorizer-friendly version of ComposeColourRGBANoCheck
	 */
	static inline uint32 ComposeColourRGBA(uint r, uint g, uint b, uint a, uint32 current)
	{
		if (a == 0) return current;
		if (a >= 255) return ComposeColour(0xFF, r, g, b);

		return ComposeColourRGBANoCheck(r, g, b, a, current);
	}

	/**
	 * Compose a colour based on Pixel value, alpha value, and the current pixel value.
	 * @node Copied here so it would use auto-vectorizer-friendly version of ComposeColourRGBANoCheck
	 */
	static inline uint32 ComposeColourPANoCheck(uint32 colour, uint a, uint32 current)
	{
		uint r  = GB(colour,  16, 8);
		uint g  = GB(colour,  8,  8);
		uint b  = GB(colour,  0,  8);

		return ComposeColourRGBANoCheck(r, g, b, a, current);
	}

	/**
	 * Compose a colour based on Pixel value, alpha value, and the current pixel value.
	 * Handles fully transparent and solid pixels in a special (faster) way.
	 * @node Copied here so it would use auto-vectorizer-friendly version of ComposeColourRGBANoCheck
	 */
	static inline uint32 ComposeColourPA(uint32 colour, uint a, uint32 current)
	{
		if (a == 0) return current;
		if (a >= 255) return (colour | 0xFF000000);

		return ComposeColourPANoCheck(colour, a, current);
	}


	/**
	 * Make a pixel looks like it is transparent.
	 * @param colour the colour already on the screen.
	 * @param nom the amount of transparency, nominator, makes colour lighter.
	 * @param drsh denominator expressed in right shift levels (i.e. drsh == 8 equials to denom = 256), makes colour darker.
	 * @return the new colour for the screen.
	 * @note Difference between this one and MakeTransparent from base class is that right shift is gcc auto-vectorizer friendly while
	 *       simple division is not.
	 */
	static inline uint32 MakeTransparent(uint32 colour, uint nom, uint drsh = 8)
	{
		uint r = GB(colour, 16, 8);
		uint g = GB(colour, 8,  8);
		uint b = GB(colour, 0,  8);

		return ComposeColour(0xFF, (r * nom) >> drsh, (g * nom) >> drsh, (b * nom) >> drsh);
	}

	/**
	 * Make a colour grey - based.
	 * @param colour the colour to make grey.
	 * @return the new colour, now grey.
	 * @note The only difference between this one and MakeGray from base class is ">> 16" instead of "/ 65536". They seem to be
	 *       identical but having shift instead of division is what allows gcc's auto-vectorizer to be able to vectorize loops
	 *       calling this function.
	 */
	static inline uint32 MakeGrey(uint32 colour)
	{
		uint r = GB(colour, 16, 8);
		uint g = GB(colour, 8,  8);
		uint b = GB(colour, 0,  8);

		/* To avoid doubles and stuff, multiple it with a total of 65536 (16bits), then
		 *  divide by it to normalize the value to a byte again. See heightmap.cpp for
		 *  information about the formula. */
		colour = ((r * 19595) + (g * 38470) + (b * 7471)) >> 16;

		return ComposeColour(0xFF, colour, colour, colour);
	}
	template <BlitterMode mode> void Draw(const Blitter::BlitterParams *bp, ZoomLevel zoom);

	/**
	 * Compose a colour based on RGB values.
	 * @note Copied here just to make patch compatible both to vanilla 1.2.1 sources and to current (as of r24450) trunk.
	 *       To prevent some questions: copying is done as a part of patch pre-release process, internal development version
	 *       has changes like this done to the base class where they really should be.
	 */
	static inline uint32 AdjustBrightness(uint32 colour, uint8 brightness)
	{
		/* Shortcut for normal brightness */
		if (brightness == DEFAULT_BRIGHTNESS) return colour;

		uint16 ob = 0;
		uint16 r = GB(colour, 16, 8) * brightness / DEFAULT_BRIGHTNESS;
		uint16 g = GB(colour, 8,  8) * brightness / DEFAULT_BRIGHTNESS;
		uint16 b = GB(colour, 0,  8) * brightness / DEFAULT_BRIGHTNESS;

		/* Sum overbright */
		if (r > 255) ob += r - 255;
		if (g > 255) ob += g - 255;
		if (b > 255) ob += b - 255;

		if (ob == 0) return ComposeColour(GB(colour, 24, 8), r, g, b);

		/* Reduce overbright strength */
		ob /= 2;
		return ComposeColour(GB(colour, 24, 8),
		                     r >= 255 ? 255 : min(r + ob * (255 - r) / 256, 255),
		                     g >= 255 ? 255 : min(g + ob * (255 - g) / 256, 255),
		                     b >= 255 ? 255 : min(b + ob * (255 - b) / 256, 255));
	}

	/**
	 * Check if the given colour index belongs to well-known colour remap range excluding palette-animate range.
	 * @param index colour index to lookup remap status for
	 * @return bool value, true if given index is from known remap range, false otherwise
	 * @warning This one is mostly a hack as the only 100% "true" assumption to be made is that the entire pallete except for index 0 belongs to a remapped range. In reality
	 * 			it's safe to only handle remaps ranges defined in original baseset and openttd.grf and it is even almost safe to only handle some subset of the remap ranges
	 * 			defined there. As primary use of this function is to give 32bpp blitters hint if that's safe blend some pixels together in advance prior to the actual Draw()
	 * 			call it is essential here to treat as "remapped" the bare minimum amount of indexes that would allow to have mostly correct final rendering.
	 * @note In case 100% compatibility is required it is sufficient to rewrite this to always return "true" for any colour index.
	 * @bug Tile borders remap to red/blue only partly supported.
	 * @bug Remapping to transparent and gray not supported at all, but they're simulated by different means in 32bpp blitters so that's not a problem.
	 * @bug Remapping defined in sprite 1194 for toyland is not supported (don't know what it does, maybe that's yet another "to gray" or "to transparent" sprite).
	 * @bug If a NewGRF define its own palette remap sprite colour indexes from there won't be detected as remapped.
	 * @bug Remap range used for church colouring is not detected due to performance reasons (visual impact is minor while performance gain could be huge for SBAA blitter).
	 * @todo Rewrite to return a value from a pre-defined array instead of doing insane amount of comparisons.
	 * */
	static inline bool isRemappedColour(uint8 index)
	{
		/* Known ranges from baseset + extra GRFs (DOS palette indexes, as it's used internally by engine):
		 * 0xE3+			Palette animate and friends. WARNING: we do not report is as being remap range, blitter which handles palette anim should take care of it.
		 * 0xC6..0xCD		Company colours remap main
		 * 0x50..0x57		Company colours remap extra (Introduced in OTTD & TTDPatch)
		 * 0x01..0x10   	Part of tile borders remap to red/blue (important to remap here are 0x09 and 0x0F for original DOS/Windows baseset and 0x09 and 0x0D for OpenGFX)
		 * 0x1C..0x1E		Part of bare land remap
		 * 0x51..0x56		DITTO (ovelaps with CC2)
		 * 0x5A..0x5E		DITTO
		 * 0x46..0x4F		PALETTE_TO_STRUCT coloured variations
		 * 0x58, 0x69, 0x6A,\
		 * 0x6D, 0x6E, 0x72, | Recolour sprites 1438 & 1439 from base set. Used to recolour "Church" sprite to "Cream" and "Red".
		 * 0x74, 0x7B,		 | May be safe not to support, would have to test.
		 * 0x76..0x78		/
		 */
		return (index >= 0xC6 && index <= 0xCD) || (index >= 0x46 && index <= 0x57) || (index == 0x09) || (index == 0x0D) || (index == 0x0F);
				/*|| (index >= 0x1C && index <= 0x1E) || (index == 0x58) || (index >= 0x5A && index <= 0x5E) || (index == 0x69) || (index == 0x6A) || (index == 0x6D)
				 *|| (index == 0x6E) || (index == 0x72) || (index == 0x74) || (index == 0x7B) || (index >= 0x76 && index <= 0x78); */
	};
};

/** Factory for the 32bpp blitter with animation. */
class FBlitter_32bppAnimAA: public BlitterFactory<FBlitter_32bppAnimAA> {
public:
	/* virtual */ const char *GetName() { return "32bpp-anim-aa"; }
	/* virtual */ const char *GetDescription() { return "32bpp Antialiased Animation Blitter (palette animation)"; }
	/* virtual */ Blitter *CreateInstance() { return new Blitter_32bppAnimAA(); }
};

/** Global vars for misc.ini */
extern uint8 _ini_blitter_32bpp_aa_level;
extern uint8 _ini_blitter_32bpp_aa_slots;
extern int8 _ini_blitter_32bpp_aa_anim_threads;


#endif /* BLITTER_32BPP_ANIM_HPP */
