/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_anim_aa.cpp Implementation of the optimized 32 bpp blitter with animation support and supersampling antialiasing. */

#include "../stdafx.h"
#include "../settings_type.h"
#include "../video/video_driver.hpp"
#include "32bpp_anim_aa.hpp"

#include "../table/sprites.h"
#include "../core/mem_func.hpp"
#include "../core/math_func.hpp"

/** Instantiation of the 32bpp with animation blitter factory. */
static FBlitter_32bppAnimAA iFBlitter_32bppAnimAA;

/** Global vars from misc.ini */
uint8 _ini_blitter_32bpp_aa_level = 4;
uint8 _ini_blitter_32bpp_aa_slots = 4;
int8 _ini_blitter_32bpp_aa_anim_threads = 4;

Blitter_32bppAnimAA::Blitter_32bppAnimAA() :
	anim_buf(NULL),
	anim_buf_width(0),
	anim_buf_height(0),
	anim_aa_continue_animate(true),
	anim_threaded(true),
	anim_threads_qty(0),
	aa_level(4),
	aa_anim_slots(4),
	anim_threads_jobs_active(0),
	remap_pixels(NULL)
{
	if ((_ini_blitter_32bpp_aa_level > 0) && (_ini_blitter_32bpp_aa_level <= 32))
	{
		/* It should be 1 << (int)floor(log(_ini_blitter_32bpp_aa_level)/log(2)), but we don't want to bother with FP */
		for(this->aa_level = 1; this->aa_level <= _ini_blitter_32bpp_aa_level; this->aa_level <<=1 ) ;
		this->aa_level >>= 1;
	}
	DEBUG(driver, 0, "32bpp-anim-aa blitter: Requested %dxSSAA, would use %dxSSAA.", _ini_blitter_32bpp_aa_level, this->aa_level);

	/* Would allocate ~4KB at the worst case of 32x AA. */
	this->temp_pixels = MallocT<Colour>(this->aa_level * this->aa_level + 1);

	if (_ini_blitter_32bpp_aa_slots > 0) {
		this->aa_anim_slots = (_ini_blitter_32bpp_aa_slots <= 255) ? _ini_blitter_32bpp_aa_slots : 255;
	}

	if(this->aa_anim_slots > (this->aa_level * this->aa_level))
		this->aa_anim_slots = this->aa_level * this->aa_level;

	DEBUG(driver, 0, "32bpp-anim-aa blitter: Requested %d AA anim slots, would use %d.", _ini_blitter_32bpp_aa_slots, this->aa_anim_slots);

	this->anim_buf_entry_size = Align(sizeof(AnimBufferEntry) + this->aa_anim_slots * sizeof(this->anim_buf->mask[0]), LX2_ABE_ALIGN);

	this->anim_threaded = _ini_blitter_32bpp_aa_anim_threads > 1;
	if (_ini_blitter_32bpp_aa_anim_threads > 1) {
		/* We create one less thread than requested as main draw thread also counts */
		this->anim_threads_qty = (_ini_blitter_32bpp_aa_anim_threads <= 127) ? _ini_blitter_32bpp_aa_anim_threads - 1 : 127;
	} else if (_ini_blitter_32bpp_aa_anim_threads == 0) {
		DEBUG(driver, 0, "32bpp-anim-aa blitter: Auto-detecting anim threads count to use...");
		/* We use OS-driven signaling to syncronize threads and due to scheduling granularity related issues it's better to use way more threads
		 * for multicore CPU than to use a number of threads that matches CPU count. That way we split the work to be done in a smaller chunks
		 * that are more likely to be significantly less than per-thread time slots that are being used by OS scheduler and that results in less
		 * time wasted on waiting. It's complicated and hard to believe but that's how it is. With 64 threads on 8 core CPU benchmarks shows
		 * about 6.25x increase in speed of PaletteAnimate() call while doing the same job with 8 threads could give about 7x boost if you're
		 * lucky but most of the times what you really get is a jittery performance having majority of calls performing with a 3.7x boost while
		 * some other getting as much as 7x boost or as low as 2x. Thus it's better to spend more cycles synchonizing larger amount of threads
		 * but get less jittery performance which would be still pretty good. Reducing jitter is especially important due to in real gaming session
		 * it could translate into jittery resulting FPS alternating between 20 and 40 FPS instead of being ~35 all the time. */
		this->anim_threads_qty = min(128, GetCPUCoreCount() * 8);
		if (this->anim_threads_qty == 0) {
			DEBUG(driver, 0, "32bpp-anim-aa blitter: GetCPUCoreCount() returned zero core count, failing back to using 2 threads.");
			this->anim_threads_qty = 1;
		} else {
			if (this->anim_threads_qty == (2*8)) {
				/* Dual-core CPU, benchmarks show that for such case it's better to have 2 threads instead of 8x2=16.*/
				this->anim_threads_qty = 1;
			} else {
				this->anim_threads_qty--;
			}
		}
		this->anim_threaded = this->anim_threads_qty > 0;
	}

	if (this->anim_threaded) {
		if (this->anim_threads_qty != (_ini_blitter_32bpp_aa_anim_threads - 1)) {
			DEBUG(driver, 1, "32bpp-anim-aa blitter: Requested %d anim threads, but would use %d instead.", _ini_blitter_32bpp_aa_anim_threads, this->anim_threads_qty + 1);
		} else {
			DEBUG(driver, 0, "32bpp-anim-aa blitter: Starting up in threaded anim mode with %d threads.", this->anim_threads_qty + 1);
		}

		this->anim_ti = CallocT<Blitter_32bppAnimAA::AnimThreadInfo>(this->anim_threads_qty);
		int threads_created = 0;

		this->th_mutex_out = ThreadMutex::New();

		/* Time to allocate stuff related to threaded palette animation */
		for (; threads_created < this->anim_threads_qty && this->anim_threaded; threads_created++)
		{
			this->anim_ti[threads_created].th_mutex_in  = ThreadMutex::New();
			/* With current implementation next condition should never evaluate to true: we'd either hit an exception upon failed "new" or get what we requested.
			 * OTOH there are some compilers out in the wild that might return NULL on a failed "new" instead of raising an exception which for us would be equal
			 * to null pointer derefference later on. Let's try to handle this situation gracefuly just because we can and performance losses are minimal. */
			if (this->anim_ti[threads_created].th_mutex_in == NULL) {
				this->anim_threaded = false;
				break;
			}

			/* Here we have to use a hack to artificially force memory barrier on a variable as a side-effect of the lock. We need make storing this to self not
			 * to be reordered by both a CPU and a compiler. We don't want to make it less hackish by introducing proper locking in static thread proc because it
			 * would require us to statically initialize a lock object also (which is painful to do in C++) meaning that even if the given game instance doesn't
			 * use this blitter at all we still would waste resources on it. Forcing memory barrier by using lock isn't required on pthreads because they warrant
			 * memory barrier on pthread_create(), but that's not a case for Win32/64 where the behavior is undefined (though most probably it's dos use barriers
			 * to protect its own internal thread-related structures). In any case it's better to be safe that sorry here as in general we only do initializing
			 * once for entire run so wasting several extra microseconds here is not a big deal. */

			this->th_mutex_out->BeginCritical();
			this->anim_ti[threads_created].self = this;
			this->th_mutex_out->EndCritical();

			this->anim_threaded = ThreadObject::New(&Blitter_32bppAnimAA::_stDrawAnimThreadProc, &this->anim_ti[threads_created]);
		}

		if (!this->anim_threaded)
		{
			/* Something went wrong while creating threads, falling back to non-threaded behavior.
			 * "Expected" disposition: threads_created >= 1, creation of the last thread failed for some reason, other threads had been creaded and are expected to
			 * be (right now or shortly afterwards) waiting for signal on mutex. We should destroy mutexes (if they are not NULL) in the anim_ti[threads_created-1]
			 * and then iterate through the rest of the enties communicating with threads telling them to shutdown gracefully and then proceeding with destroying
			 * both thread and related mutexes. */
			threads_created--;

			ShowInfoF("32bpp-anim-aa blitter: Failed to create palette anim threads, failing back to non-threaded behavior.");
			DEBUG(driver, 1, "32bpp-anim-aa blitter: Failed to create %d anim threads (but was successful to create %d thread[s] before failure), falling back to non-threaded behavior.",
					this->anim_threads_qty, threads_created);

			if(this->anim_ti[threads_created].th_mutex_in != NULL) delete this->anim_ti[threads_created].th_mutex_in;

			this->anim_aa_continue_animate = false;

			this->th_mutex_out->BeginCritical();
			while(threads_created > 0)
			{
				threads_created--;
				this->anim_ti[threads_created].th_mutex_in->BeginCritical();
				this->anim_ti[threads_created].th_mutex_in->SendSignal();
				this->anim_ti[threads_created].th_mutex_in->EndCritical();
				this->th_mutex_out->WaitForSignal();
				delete this->anim_ti[threads_created].th_mutex_in;
			}
			this->th_mutex_out->EndCritical();
			delete this->th_mutex_out;
			this->th_mutex_out = NULL;
			free(this->anim_ti);
			this->anim_ti = NULL;
		}
	} else {
		DEBUG(driver, 0, "32bpp-anim-aa blitter: Starting up in single-threaded palette animation mode.");
	}
}

Blitter_32bppAnimAA::~Blitter_32bppAnimAA()
{
	/* Useless for now as blitters are used as singletons mostly, but who knows how the things would develop in the future */

	if (this->anim_threaded)
	{
		/* Let's inform threads that we're shutting down so they would also shut down themselves */
		this->anim_aa_continue_animate = false;
		this->th_mutex_out->BeginCritical();
		for (uint i = 0; i < this->anim_threads_qty; i++)
		{
			this->anim_ti[i].th_mutex_in->BeginCritical();
			this->anim_ti[i].th_mutex_in->SendSignal();
			this->anim_ti[i].th_mutex_in->EndCritical();
			this->th_mutex_out->WaitForSignal();
			delete this->anim_ti[i].th_mutex_in;
		}
		this->th_mutex_out->EndCritical();
		delete this->th_mutex_out;
		free(this->anim_ti);
	}

	free(this->anim_buf);
	free(this->remap_pixels);
	free(this->temp_pixels);
}



template <BlitterMode mode>
inline void Blitter_32bppAnimAA::Draw(const Blitter::BlitterParams *bp, ZoomLevel zoom) {
	const SpriteData *src = (const SpriteData *)bp->sprite;
	const uint32 *src_px = src->data + src->offset[zoom];

	for (uint i = bp->skip_top; i != 0; i--) src_px = src_px + *src_px;

	uint32 *dst = (uint32 *)bp->dst + bp->top * bp->pitch + bp->left;

	/* Read comment in SetPixel implementation for info on assumption we do here */
	AnimBufferEntry *anim = (AnimBufferEntry *)((uint8 *)this->anim_buf + this->anim_buf_entry_size * (this->anim_buf_width * bp->top + bp->left +
			((uint32 *)bp->dst - (uint32 *)_screen.dst_ptr)));

	const byte *remap = bp->remap; // store so we don't have to access it via bp everytime

	/* We do draw looping destination line-by-line */
	for (int y = 0; y < bp->height; y++) {
		uint32 *dst_ln = dst + bp->pitch;
		AnimBufferEntry *anim_ln = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size * this->anim_buf_width);
		const uint32 *src_px_ln = src_px + *src_px;
		src_px++;

		uint32 *dst_end = dst + bp->skip_left;
		uint32 n;
		uint8 t;

		/* First stage is to skip pixels on the left. As encoded sprite format is RLE-like and not per-pixel seekable
		 * - we can't navigate our src pointer directly to desired start location. Thus it is required to decode the
		 * sprite line pixel-by-pixel in loop and skip pixels that what we're told to skip.
		 * Storage format is optimized so most of blocks could be skipped by using simple pointer math. */

		while (dst < dst_end) {
			n = *src_px++;
			t = n >> 24;
			n &= 0xFFFFFF;
			if (t == 0) {
				dst += n;
				if (dst > dst_end) anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size * (dst - dst_end));
			} else {
				if (dst + n > dst_end) {
					uint d = dst_end - dst;
					n = min<uint>(n - d, (uint)bp->width);
					if (t != 3) {
						src_px += d;
					} else {
						/* Worst case: have to seek d pixels forward in Class 3 RLE group */
						while (d > 0) {
							bool has_extra_pix = (*src_px & (1 << 31)) != 0;
							while ((*src_px & (1 << 29)) != 0) src_px +=  GB(*src_px, 24, 5) + 1;
							src_px +=  GB(*src_px, 24, 5) + (has_extra_pix ? 3 : 1);
							d--;
						}
					}

					dst = dst_end - bp->skip_left;
					dst_end = dst + bp->width;
					goto draw;
				}
				dst += n;

				if (t != 3) {
					src_px += n;
				} else {
					/* Worst case: have to seek n pixels forward in Class 3 RLE group */
					while (n > 0) {
						bool has_extra_pix = (*src_px & (1 << 31)) != 0;
						while ((*src_px & (1 << 29)) != 0) src_px +=  GB(*src_px, 24, 5) + 1;
						src_px +=  GB(*src_px, 24, 5) + (has_extra_pix ? 3 : 1);
						n--;
					}
				}
			}
		}

		dst -= bp->skip_left;
		dst_end += bp->width - bp->skip_left;

		while (dst < dst_end) {
			n = *src_px++;
			t = n >> 24;
			n &= 0xFFFFFF;
			n = min<uint32>(n, (uint32)(dst_end - dst));

			if (t == 0) {
				anim = (AnimBufferEntry *)((uint8 *)anim + n * this->anim_buf_entry_size);
				dst += n;
				continue;
			}

			draw:;

			/* Sad story about sprites and seams: to compensate for rounding errors when downsampling we have to use a hackish trick:
			 * force target pixel to be fully opaque in case it would be fully opaque when downsamled using original nearest neighbour
			 * downsampling technique. Same thing for fully transparent pixels. Encoding process had done most of the job for us, the
			 * only case we have to care is if the subpixel we should base our decision on is of a "Class 3 Type 5" family.
			 * "Luckly" for us encoder had set special flag so we don't have to guess anything. */

			switch (mode) {
				case BM_COLOUR_REMAP:

					/* We do not merge together do/while loops for different if(t==?) branches to allow compiler optimize cases t==1 and t==2 better. */
					if (t == 1) {
						/* Opaque 32bpp pixels RLE group */
						do {
							*dst++ = *src_px++;
							anim->mask_samples = 0;
							anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
						} while (--n != 0);
					} else if (t == 2) {
						/* Alpha-blended 32bpp pixels RLE group */
						do {
							*dst = ComposeColourPANoCheck(*src_px, *src_px >> 24, *dst);
							src_px++;
							dst++;
							anim->mask_samples = 0;
							anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
						} while (--n != 0);
					}
					else
					{
						/* Palette-remapped group */
						do {
							bool has_extra_pix = (*src_px & (1 << 31)) != 0, has_more_chunks;
							int blend_qty = 0, anim_qty = 0, anim_mask_samples = 0, pixel_samples;

							/* For remapped case there's chance that after remapping alpha channel would change. We would have to do check later on if
							 * needed and wither set force_opaque or skip this entire pixel as a "forced fully transparent". */
							uint8 force_opaque = (*src_px & (1 << 30)) != 0 ? 2 : 0;

							do {
								has_more_chunks = (*src_px & (1 << 29)) != 0;
								if (force_opaque != 3) {
									for(int d = GB(*src_px, 24, 5); d >= 0; d--) {
										uint r = remap[GB(*src_px, 0, 8)];
										if (r != 0) {
											this->temp_pixels[blend_qty].data = this->AdjustBrightness(this->LookupColourInPalette(r), GB(*src_px, 8, 8));
											/* Above PALETTE_ANIM_START is palette animation */
											if (r >= PALETTE_ANIM_START) {
												/* For remapped case there's a chance that non-animated colour would be mapped into animated range. As the number of anim slots
												 * could (and most probably will) be less than this->aa_level * this->aa_level we have to handle this case somehow. Introducing complicated
												 * throw out logic in "hot" part of the code is not a welcome thing so for now we simply place remapped anim pixels into
												 * slots until we run out of slots and ignore the rest. The only real-life situation known for us when this could happen is a
												 * pulsating red colour for "tile border" sprite that's being displayed when it's not possible to level ground due to some
												 * building being on the way. */

												if (anim_qty < this->aa_anim_slots) {
													if (anim_qty == 0) {
														anim_mask_samples = GB(*src_px, 16, 8);
													}
#if LX2_CONDENSED_AP == 1
													/* Zeroed mask serves as end marker so we add 1 to colour index to warrant that we always
													 * got non-zero IndexedPixel.data value. PALETTE_ANIM_SIZE == 28 allows us to do it safely */
													anim->mask[anim_qty % this->aa_anim_slots].c = r + 1 - PALETTE_ANIM_START;
													anim->mask[anim_qty % this->aa_anim_slots].v = GB(*src_px, 8 + 5, 3);
#else
													anim->mask[anim_qty % this->aa_anim_slots].c = r;
													anim->mask[anim_qty % this->aa_anim_slots].v = GB(*src_px, 8, 8);
#endif
													this->temp_pixels[blend_qty].a = 255;

													/* This sub-pixel currently might be located not in the designated place for this anim slot, place it accordingly. */
													if (blend_qty != (anim_qty % this->aa_anim_slots)) {
														Swap(this->temp_pixels[blend_qty], this->temp_pixels[anim_qty % this->aa_anim_slots]);
													}
													anim_qty++;
												} else {
													/* We throw out subsample that couldn't fit into available anim slots but we still want to account for the fact
													 * of its existence in blending calculations. */
													anim_mask_samples++;
												}
											} else {
												this->temp_pixels[blend_qty].a = GB(*src_px, 16, 8);
											}
										} else {
											this->temp_pixels[blend_qty].data = 0;
										}

										if (force_opaque == 2) {
											/* Execution gets here only once per sequence after processing of the first sub-sample */
											force_opaque = (r >= PALETTE_ANIM_START) ? 255 : this->temp_pixels[blend_qty].a;
											if (force_opaque == 0) {
												/* Insert #if 0 here to disable "force_transparent" hack */
												force_opaque = 3;
												src_px += d + 1;
												break;
												/* Insert #endif here to disable "force_transparent" hack */
											} else
												force_opaque = (force_opaque == 255) ? 1 : 0;
										}

										blend_qty++;
										src_px++;
									}
								} else /* if force_opaque == 3 */	{
									src_px += GB(*src_px, 24, 5) + 1;
								}
							} while(has_more_chunks);

							if (force_opaque == 3) {
								dst++;
								anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
								if (has_extra_pix) src_px++;
								continue;
							}

							/* This comparison frees us from doing BlendPixels for ~32% of pixels (basing on gcov stats), which is significantly
							 * different from the BM_NORMAL stats. Suspects are font sprites which are always remapped and have only one zoom level. */
							if (blend_qty > 1) {
								this->temp_pixels->data = this->BlendPixels(this->temp_pixels, blend_qty);
							}

							/* So now we have a pre-blended value for all remapped pixels, time to blend it with non-remapped part (if any)
							 * taking into account coverage percentage. About 3.5% of pixels require it (gcov stats). */
							if (has_extra_pix) {
								pixel_samples = *src_px++;
								anim->pixel.data = *src_px++;

								if (force_opaque == 0) force_opaque = (anim->pixel.a == 255) ? 1 : 0;

								/* Total N+M subsamples.
								 * We known pre-blended values for M and N sub-samples.
								 * We want to compose final pixel value.
								 * Alpha channels act as weights modifiers.
								 * X = (M*Am*Xm + N*An*Xn)/(M*Am + N*An) */

								uint weight_1, weight_2, weight_s;
								weight_1 = pixel_samples * anim->pixel.a;
								if (anim_qty > 0) {
									/* Real subsamples count for remapped indexed pixels might be more than blend_qty due to some sub-samples
									 * that couldn't fit in this->aa_anim_slots had been thrown away as a part of encoding process. Account for it
									 * in our equations. */
									weight_2 = (blend_qty - anim_qty + anim_mask_samples) * this->temp_pixels->a;
									weight_s = weight_1 + weight_2;
									this->temp_pixels->a = weight_s / (pixel_samples + blend_qty + anim_mask_samples - anim_qty);
								} else {
									weight_2 = blend_qty * this->temp_pixels->a;
									weight_s = weight_1 + weight_2;
									this->temp_pixels->a = weight_s / (pixel_samples + blend_qty);
								}
								this->temp_pixels->r = (weight_2 * this->temp_pixels->r + weight_1 * anim->pixel.r) / weight_s;
								this->temp_pixels->g = (weight_2 * this->temp_pixels->g + weight_1 * anim->pixel.g) / weight_s;
								this->temp_pixels->b = (weight_2 * this->temp_pixels->b + weight_1 * anim->pixel.b) / weight_s;
							}

							if (force_opaque != 0) {
								*dst = (0xFF << 24) | this->temp_pixels->data;
							} else {
								if (this->temp_pixels->a == 255) {
									*dst = this->temp_pixels->data;
								} else {
									/* Have to store original pixel value because it'd be needed later. Why don't we store it for a == 255 branch?
									 * Because what we draw now has the same alpha as would had been calculated in PaletteAnimate() if we were calculating
									 * it there. I.e. if we get opaque pixel when combine pre-blended + remapped + anim subpixels and if we assume that
									 * anim pixels are always opaque - there's no chance to get semi-transparent pixel combining all above except anim. */
									if (anim_qty > 0) this->temp_pixels[blend_qty].data = *dst;
									*dst = ComposeColourPANoCheck(this->temp_pixels->data, this->temp_pixels->a, *dst);
								}
							}

//							/* FIXME: Temp branch for visual output testing */
//							*dst = ComposeColourPANoCheck(0xFFFF0000, 255 - 255 * ((this->aa_level * this->aa_level) - blend_qty - (has_extra_pix ? 1 : 0)) / (this->aa_level * this->aa_level), *dst);

							/* And now back to anim buffer stuff */
							if (anim_qty > 0) {
								/* Set end marker for palette animation proc */
								if (anim_qty < this->aa_anim_slots) anim->mask[anim_qty].data = 0;

								/* We have to calculate yet another pre-blended value excluding remapped pixels that had been placed into anim buffer slots. */
								this->temp_pixels->data = this->BlendPixels(&this->temp_pixels[anim_qty], blend_qty - anim_qty);

								if (has_extra_pix) {
									int weight_1 = anim->pixel.a * pixel_samples;
									int weight_2 = this->temp_pixels->a * (blend_qty - anim_qty);
									int weight_s = weight_1 + weight_2;
									pixel_samples += blend_qty - anim_qty;
									if (weight_s != 0) {
										anim->pixel.r = (weight_1 * anim->pixel.r + weight_2 * this->temp_pixels->r) / weight_s;
										anim->pixel.g = (weight_1 * anim->pixel.g + weight_2 * this->temp_pixels->g) / weight_s;
										anim->pixel.b = (weight_1 * anim->pixel.b + weight_2 * this->temp_pixels->b) / weight_s;
										anim->pixel.a = weight_s / pixel_samples;
									} else {
										anim->pixel.data = 0;
									}
								} else {
									anim->pixel.data = this->temp_pixels->data;
									pixel_samples = blend_qty - anim_qty;
								}

								/* TODO: We made an assumption that remapped anim sub-pixels are always opaque. There's a chance for this assumption
								 * to be wrong for 32bpp non-opaque sprite + mask remaping into anim palette range, but these are unlikely and we don't
								 * want to store extra uint8 (alpha) per anim sub-pixel in anim buffer plus one uint32 per target pixel (original
								 * dest) just to be correct for the case that is unlikely to ever happen. It should be fixed though if reality
								 * proves that this case is not as unlikely as we thought.
								 *
								 * Nice consequence of this assumption is that we could do alpha-blending now (there's some math you have to do to prove it),
								 * if needed, so we wouldn't have to alpha-blend each time PaletteAnimate is called. And another nice consequence is that we
								 * don't have to store force opaque flag in anim buffer as PaletteAnimate() is expected to always produce opaque pixels,
								 * so we could take force_opaque flag into account here and don't care about it later during palette animation. Lovely! */
								if (force_opaque == 0)
									anim->pixel.data = ComposeColourRGBA(anim->pixel.r, anim->pixel.g, anim->pixel.b, anim->pixel.a, this->temp_pixels[blend_qty].data);

								/* Yes, we could loose precision here. Yes, PaletteAnimate() could produce slightly different output than Draw() for animated pixels because of
								 * that. No, we don't worry about it as it could only happen for AA_LEVEL > 16 and extremely minor blending errors would be of a less worry than
								 * the turtle-like speed you'd get in case you have aa_anim_slots set to anything higher than 16. */
								if (pixel_samples > LX2_MAX_PS) {
									if (anim_mask_samples > pixel_samples) {
										/* Should never happen with current implementation: anim_mask_samples is limited by the byte storage size, while
										 * pixel_samples isn't up to moment we store it into anim buffer. */
										assert(anim_mask_samples > pixel_samples);
										pixel_samples = min(LX2_MAX_PS, pixel_samples * 0xFF / anim_mask_samples);
										anim_mask_samples = 0xFF;
									} else {
										anim_mask_samples = min(0xFF, anim_mask_samples * LX2_MAX_PS / pixel_samples);
										pixel_samples = LX2_MAX_PS;
									}
								}

								anim->pixel_samples = pixel_samples;
								anim->mask_samples = anim_mask_samples;
							} else {
								/* Write "no-anim" marker into anim buffer */
								anim->mask_samples = 0;
							}

							dst++;
							anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
						} while (--n != 0);

					}
					break;

				case BM_TRANSPARENT:
					/* TODO -- We make an assumption here that the remap in fact is transparency, not some colour.
					 *  This is never a problem with the code we produce, but newgrfs can make it fail... or at least:
					 *  we produce a result the newgrf maker didn't expect ;) */

					/* Here we only have to blend in alpha channels for Class 3. We do not merge together do/while loops for
					 * different if(t==?) branches to allow compiler optimize cases t==1 and t==2 better. */
					if (t == 1)	{
						/* Opaque 32bpp pixels RLE group */
						src_px += n;
						do {
							*dst = MakeTransparent(*dst, 3, 2);
							if (anim->mask_samples != 0) {
								/* Read comment in next (t == 2) if branch for description of what being done here. */
								uint ms_new = anim->mask_samples, ps_new = ((ms_new + anim->pixel_samples) << 2) - 3 * ms_new;
								if (anim->pixel_samples == 0) {
									anim->pixel.data = 0;
								} else {
									anim->pixel.r = 4 * anim->pixel.r * anim->pixel_samples / ps_new;
									anim->pixel.g = 4 * anim->pixel.g * anim->pixel_samples / ps_new;
									anim->pixel.b = 4 * anim->pixel.b * anim->pixel_samples / ps_new;
								}
								ms_new *= 3;
								/* We only do downwards rebalancing if strictly required */
								if ((ms_new > 0xFF) || (ps_new > LX2_MAX_PS)) {
									if (ms_new > ps_new) {
										ps_new = min(LX2_MAX_PS, ps_new * 0xFF / ms_new);
										ms_new = 0xFF;
									} else {
										ms_new = min(0xFF, ms_new * LX2_MAX_PS / ps_new);
										ps_new = LX2_MAX_PS;
									}
								}
								anim->pixel_samples = ps_new;
								anim->mask_samples = ms_new;
							}
							anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
							dst++;
						} while (--n != 0);

					} else if (t == 2) {
						/* Alpha-blended 32bpp pixels RLE group */
						do {
							/* As this loop is already filled up with logic constructs - let's have one more safeguarding us from div-by-zero and potentionally giving small speedup */
							if (GB(*src_px, 24, 8) != 0) {
								*dst = MakeTransparent(*dst, ((1 << 10) - GB(*src_px, 24, 8)), 10);
								if (anim->mask_samples != 0) {
									/* What we do here is simulate MakeTransparent effect by rebalancing mask_samples vs. pixel_samples and making pre-blended pixel part darker.
									 * Math used here is simple and could be checked on a piece of paper within few minutes.
									 *
									 * There are some pre-conditions to be met though:
									 * a) (pixel_samples + mask_samples) != 0. This one is always true due to "if" above.
									 * b) nom < denom. Despite being OK from math PoV it would lead to negative pixel_samples count which is both
									 *    makes no sence (how could we take -1 number of samples?) and won't fit into unsigned data type.
									 * c) As mask_samples and pixel_samples are limited by storage size we have to scale them
									 *    down after calculations so we won't hit arithmetic overflown eventually.
									 *
									 * Useful property that is strictly true after calculations:
									 * if (mask_samples == 1) then (pixel_samples >= 1)
									 */
									uint ms_new = anim->mask_samples, ps_new = (1 << 10) * (ms_new + anim->pixel_samples) - ((1 << 10) - GB(*src_px, 24, 8)) * ms_new;
									if (anim->pixel_samples == 0) {
										anim->pixel.data = 0;
									} else {
										anim->pixel.r = (1 << 10) * anim->pixel.r * anim->pixel_samples / ps_new;
										anim->pixel.g = (1 << 10) * anim->pixel.g * anim->pixel_samples / ps_new;
										anim->pixel.b = (1 << 10) * anim->pixel.b * anim->pixel_samples / ps_new;
									}
									ms_new *= ((1 << 10) - GB(*src_px, 24, 8));
									/* Checking here if rebalancing is strictly needed is a waste of time due to 1024 a.k.a. 1<<10 being used as denom. We're 100%
									 * warranded to get ms_new larger than 0xFF. So let's just always rebalance downwards and live with it. */
									if (ms_new > ps_new) {
										ps_new = min(LX2_MAX_PS, ps_new * 0xFF / ms_new);
										ms_new = 0xFF;
									} else {
										ms_new = min(0xFF, ms_new * LX2_MAX_PS / ps_new);
										ps_new = LX2_MAX_PS;
									}
									anim->pixel_samples = ps_new;
									anim->mask_samples = ms_new;
								}
							}
							dst++;
							anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
							src_px++;
						} while (--n != 0);

					} else if (t == 3) {

						do {
							bool has_extra_pix = (*src_px & (1 << 31)) != 0, has_more_chunks;
							uint32 alpha_sum = 0, alpha_qty = 0;

							/* This case is the same as unremapped - there's no way alpha channel of decoded remapped pixels to be different than
							 * it was stored during encoding process. So let's just check it and set flag if needed. */
							bool force_opaque = (*src_px & (1 << 30)) != 0 && ((GB(*src_px, 16, 8) == 255) || (GB(*src_px, 0, 8) >= PALETTE_ANIM_START));

							if (!force_opaque) {
								if (GB(*src_px, 0, 8) >= PALETTE_ANIM_START) {
									alpha_qty = GB(*src_px, 16, 8);
									alpha_sum = 255 * alpha_qty;
								}
								do {
									has_more_chunks = (*src_px & (1 << 29)) != 0;
									/* Cache value as compiler don't do it on its own */
									int d = GB(*src_px, 24, 5);
									alpha_qty += d + 1;
									for (; d >= 0; d--) {
										if (GB(*src_px, 0, 8) >= PALETTE_ANIM_START) {
											alpha_qty--;
										} else {
											alpha_sum += GB(*src_px, 16, 8);
										}
										src_px++;
									}
								} while(has_more_chunks);
							} else {
								/* Simply skip current sequence */
								while ((*src_px & (1 << 29)) != 0) src_px +=  GB(*src_px, 24, 5) + 1;
								src_px +=  GB(*src_px, 24, 5) +  1;
							}

							if (has_extra_pix) {
								uint subpixels_count = *src_px++;
								if (!force_opaque) force_opaque = GB(*src_px, 24, 8) == 255;
								if (!force_opaque) {
									alpha_sum += GB(*src_px, 24, 8) * subpixels_count;
									alpha_qty += subpixels_count;
								}
								src_px++;
							}

							if (force_opaque) {
								alpha_sum = 255;
								alpha_qty = 1;
							}

							alpha_sum /= alpha_qty;

							if (alpha_sum != 0) {
								*dst = MakeTransparent(*dst, ((1 << 10) - alpha_sum), 10);
								if (anim->mask_samples != 0) {
									uint ms_new = anim->mask_samples, ps_new = (1 << 10) * (ms_new + anim->pixel_samples) - ((1 << 10) - alpha_sum) * ms_new;
									if (anim->pixel_samples == 0) {
										anim->pixel.data = 0;
									} else {
										anim->pixel.r = (1 << 10) * anim->pixel.r * anim->pixel_samples / ps_new;
										anim->pixel.g = (1 << 10) * anim->pixel.g * anim->pixel_samples / ps_new;
										anim->pixel.b = (1 << 10) * anim->pixel.b * anim->pixel_samples / ps_new;
									}
									ms_new *= ((1 << 10) - alpha_sum);
									/* Checking here if rebalancing is strictly needed is a waste of time due to 1024 a.k.a. 1<<10 being used as denom. We're 100%
									 * warranded to get ms_new larger than 0xFF. So let's just always rebalance downwards and live with it. */
									if (ms_new > ps_new) {
										ps_new = min(LX2_MAX_PS, ps_new * 0xFF / ms_new);
										ms_new = 0xFF;
									} else {
										ms_new = min(0xFF, ms_new * LX2_MAX_PS / ps_new);
										ps_new = LX2_MAX_PS;
									}
									anim->pixel_samples = ps_new;
									anim->mask_samples = ms_new;
								}
							}
							dst++;
							anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
						} while (--n != 0);
					}
					break;

				default: /* BM_NORMAL */

					/* We do not merge together do/while loops for different if(t==?) branches to allow compiler optimize cases t==1 and t==2 better. */
					if (t == 1)	{
						/* Opaque 32bpp pixels RLE group */
						do {
							*dst++ = *src_px++;
							anim->mask_samples = 0;
							anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
						} while (--n != 0);
					} else if(t == 2) {
						/* Alpha-blended 32bpp pixels RLE group */
						do {
							*dst = ComposeColourPANoCheck(*src_px, *src_px >> 24, *dst);
							src_px++;
							dst++;
							anim->mask_samples = 0;
							anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
						} while (--n != 0);
					} else {
						/* Palette-remapped group */
						do {
							bool has_extra_pix = (*src_px & (1 << 31)) != 0, has_more_chunks;
							int blend_qty = 0, anim_qty = 0, anim_mask_samples = 0, pixel_samples;

							/* For this unremapped case there's no way alpha channel of decoded remapped pixels to be different from what was stored by
							 * encoder. So let's just check and set flag if needed. */
							bool force_opaque = (*src_px & (1 << 30)) != 0 && ((GB(*src_px, 0, 8) >= PALETTE_ANIM_START) || (GB(*src_px, 16, 8) == 255));

							do {
								has_more_chunks = (*src_px & (1 << 29)) != 0;
								for (int d = GB(*src_px, 24, 5); d >= 0; d--) {
									/* Compiler assumes pointer aliasing, can't optimise this on its own */
									uint m = GB(*src_px, 0, 8);
#if LX2_CONDENSED_AP == 1
									this->temp_pixels[blend_qty].data = this->AdjustBrightness(this->LookupColourInPalette(m), GB(*src_px, 8, 8));
#else
									this->temp_pixels[blend_qty].data = this->cached_palette[GB(*src_px, 0, 16)].data;
#endif

									/* Above PALETTE_ANIM_START is palette animation */
									if (m >= PALETTE_ANIM_START) {
										if (anim_qty == 0) {
											anim_mask_samples = GB(*src_px, 16, 8);
										}
#if LX2_CONDENSED_AP == 1
										anim->mask[anim_qty % this->aa_anim_slots].c = m + 1 - PALETTE_ANIM_START;
										anim->mask[anim_qty % this->aa_anim_slots].v = GB(*src_px, 8 + 5, 3);
#else
										anim->mask[anim_qty % this->aa_anim_slots].data = *src_px & 0xFFFF;
#endif
										this->temp_pixels[blend_qty].a = 255;

										/* Make sure that the subpixel lands into designated temp_pixels index related to the current anim slot. */
										if (blend_qty != (anim_qty % this->aa_anim_slots)) {
											Swap(this->temp_pixels[blend_qty], this->temp_pixels[anim_qty % this->aa_anim_slots]);
										}
										anim_qty++;
										/* We rely on the fact that encoder throws out anim pixels and warrants that we never should get more than this->aa_anim_slots here */
										assert(anim_qty <= this->aa_anim_slots);
									} else {
										this->temp_pixels[blend_qty].a = GB(*src_px, 16, 8);
									}
									blend_qty++;
									src_px++;
								}
							} while(has_more_chunks);

							/* It used to be if (blend_qty > 1) branch here, like there's for BM_REMAP, but for BM_NORMAL draw gcov shows that for 4x AA +
							 * 16 ANIM_AA_SLOTs we only avoid ~0.4% BlendPixels() executions thus making branching here to be rather bad than good. */
							this->temp_pixels->data = this->BlendPixels(this->temp_pixels, blend_qty);

							/* So now we have a pre-blended value for all remapped pixels, time to blend it with non-remapped part (if any)
							 * taking into account coverage percentage. */
							if (has_extra_pix) {
								pixel_samples = *src_px++;
								anim->pixel.data = *src_px++;

								if (!force_opaque) force_opaque = (anim->pixel.a == 255);

								/* Total N+M subsamples.
								 * We known pre-blended values for M and N sub-samples.
								 * We want to compose final pixel value.
								 * Alpha channels act as weights modifiers.
								 * X = (M*Am*Xm + N*An*Xn)/(M*Am + N*An) */
								uint weight_1, weight_2, weight_s;
								weight_1 = pixel_samples * anim->pixel.a;

								if (anim_qty > 0) {
									/* Real subsamples count for remapped indexed pixels might be more than blend_qty due to some sub-samples
									 * that couldn't fit in this->aa_anim_slots had been thrown away as a part of encoding process. Account for it
									 * in out equations. */
									weight_2 = (blend_qty - anim_qty + anim_mask_samples) * this->temp_pixels->a;
									weight_s = weight_1 + weight_2;
									this->temp_pixels->a = weight_s / (anim->pixel_samples + blend_qty - anim_qty + anim_mask_samples);
								} else {
									weight_2 = blend_qty * this->temp_pixels->a;
									weight_s = weight_1 + weight_2;
									this->temp_pixels->a = weight_s / (anim->pixel_samples + blend_qty);
								}
								this->temp_pixels->r = (weight_2 * this->temp_pixels->r + weight_1 * anim->pixel.r) / weight_s;
								this->temp_pixels->g = (weight_2 * this->temp_pixels->g + weight_1 * anim->pixel.g) / weight_s;
								this->temp_pixels->b = (weight_2 * this->temp_pixels->b + weight_1 * anim->pixel.b) / weight_s;
							}

							if (force_opaque) {
								*dst = (0xFF << 24) | this->temp_pixels->data;
							} else {
								if (this->temp_pixels->a == 255) {
									*dst = this->temp_pixels->data;
								} else {
									/* Have to store original pixel value because it'd be needed later. Why don't we store it for a == 255 branch?
									 * Because what we draw now has the same alpha as would had been calculated in PaletteAnimate() if we were calculating
									 * it there. I.e. if we get opaque pixel when combine pre-blended + remapped + anim subpixels and if we assume that
									 * anim pixels are always opaque - there's no chance to get semi-transparent pixel combining all above except anim. */
									if (anim_qty > 0) this->temp_pixels[blend_qty].data = *dst;
									*dst = ComposeColourPANoCheck(this->temp_pixels->data, this->temp_pixels->a, *dst);
								}
							}

//							/* FIXME: Temp branch for visual output testing */
//							*dst = ComposeColourPANoCheck(0xFFFF0000, 255 - 255 * ((this->aa_level * this->aa_level) - blend_qty - (has_extra_pix ? 1 : 0)) / (this->aa_level * this->aa_level), *dst);

							/* And now back to anim buffer stuff */
							if (anim_qty > 0) {
								/* Set end marker for palette animation proc */
								if (anim_qty < this->aa_anim_slots) anim->mask[anim_qty].data = 0;

								/* We have to calculate yet another pre-blended value excluding remapped pixels that had been placed into anim buffer slots. */
								this->temp_pixels->data = blend_qty - anim_qty;
								if (this->temp_pixels->data != 0) {
									/* gcov show that only ~0.05% pixels take this branch. */
									this->temp_pixels->data = this->BlendPixels(&this->temp_pixels[anim_qty], this->temp_pixels->data);
								}

								if (has_extra_pix) {
									/* Fix the value back when doing calculations on the next line */
									int weight_1 = pixel_samples * anim->pixel.a;
									int weight_2 = (blend_qty - anim_qty) * this->temp_pixels->a;
									int weight_s = weight_1 + weight_2;
									pixel_samples += blend_qty - anim_qty;
									if (weight_s != 0) {
										anim->pixel.r = (weight_1 * anim->pixel.r + weight_2 * this->temp_pixels->r) / weight_s;
										anim->pixel.g = (weight_1 * anim->pixel.g + weight_2 * this->temp_pixels->g) / weight_s;
										anim->pixel.b = (weight_1 * anim->pixel.b + weight_2 * this->temp_pixels->b) / weight_s;
										anim->pixel.a = weight_s / pixel_samples;
									} else {
										anim->pixel.data = 0;
									}
								} else {
									anim->pixel.data = this->temp_pixels->data;
									pixel_samples = blend_qty - anim_qty;
								}

								/* TODO: We made an assumption that remapped anim sub-pixels are always opaque. There's a chance for this assumption
								 * to be wrong for 32bpp non-opaque sprite + mask remaping into anim palette range, but these are unlikely and we don't
								 * want to store extra uint8 (alpha) per anim sub-pixel in anim buffer plus one uint32 per target pixel (original
								 * dest) just to be correct for the case that is unlikely to ever happen. It should be fixed though if reality
								 * proves that this case is not as unlikely as we thought.
								 *
								 * Nice consequence of this assumption is that we could do alpha-blending now (there's some math you have to do to prove it),
								 * if needed, so we wouldn't have to alpha-blend each time PaletteAnimate is called. And another nice consequence is that we
								 * don't have to store force opaque flag in anim buffer as PaletteAnimate() is expected to always produce opaque pixels,
								 * so we could take force_opaque flag into account here and don't care about it later during palette animation. Lovely! */
								if (!force_opaque) {
									anim->pixel.data = ComposeColourRGBA(anim->pixel.r, anim->pixel.g, anim->pixel.b, anim->pixel.a, this->temp_pixels[blend_qty].data);
								}

								/* Yes, we could loose precision here. Yes, PaletteAnimate() could produce slightly different output than Draw() for animated pixels because of
								 * that. No, we don't worry about it as it could only happen for AA_LEVEL > 16 and extremely minor blending errors would be of a less worry than
								 * the turtle-like speed you'd get in case you have aa_anim_slots value set to anything higher than 16. */
								if (pixel_samples > LX2_MAX_PS) {
									if (anim_mask_samples > pixel_samples) {
										/* Should never happen with current implementation: anim_mask_samples is limited by the byte storage size, while
										 * pixel_samples isn't up to moment we store it into anim buffer. */
										assert(anim_mask_samples > pixel_samples);
										pixel_samples = min(LX2_MAX_PS, pixel_samples * 0xFF / anim_mask_samples);
										anim_mask_samples = 0xFF;
									} else {
										anim_mask_samples = min(0xFF, anim_mask_samples * LX2_MAX_PS / pixel_samples);
										pixel_samples = LX2_MAX_PS;
									}
								}

								anim->pixel_samples = pixel_samples;
								anim->mask_samples = anim_mask_samples;
							} else {
								/* Write "no-anim" marker into anim buffer */
								anim->mask_samples = 0;
							}

							dst++;
							anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
						} while (--n != 0);
					}
					break; /* switch (mode) */
			}
		}
		anim   = anim_ln;
		dst    = dst_ln;
		src_px = src_px_ln;
	}

}

void Blitter_32bppAnimAA::Draw(Blitter::BlitterParams *bp, BlitterMode mode, ZoomLevel zoom)
{
	if (_screen_disable_anim) {
		/* This means our output is not to the screen, so we can't be doing any animation stuff, so use our parent Draw() */
		Blitter_32bppOptimized::Draw(bp, mode, zoom);
		return;
	}

	switch (mode) {
		default: NOT_REACHED();
		/* no break */
		case BM_NORMAL:       Draw<BM_NORMAL>      (bp, zoom); return;
		case BM_COLOUR_REMAP: Draw<BM_COLOUR_REMAP>(bp, zoom); return;
		case BM_TRANSPARENT:  Draw<BM_TRANSPARENT> (bp, zoom); return;
	}
}

void Blitter_32bppAnimAA::DrawColourMappingRect(void *dst, int width, int height, PaletteID pal)
{
	if (_screen_disable_anim) {
		/* This means our output is not to the screen, so we can't be doing any animation stuff, so use our parent DrawColourMappingRect() */
		Blitter_32bppOptimized::DrawColourMappingRect(dst, width, height, pal);
		return;
	}

	uint32 *udst = (uint32 *)dst;

	/* Read comment in SetPixel implementation for info on assumption we do here */
	AnimBufferEntry *anim = (AnimBufferEntry *)((uint8 *)this->anim_buf + this->anim_buf_entry_size * ((uint32 *)dst - (uint32 *)_screen.dst_ptr));

	if (pal == PALETTE_TO_TRANSPARENT) {
		do {
			for (int i = 0; i != width; i++) {
				*udst = MakeTransparent(*udst, 154);
				udst++;
				if (anim->mask_samples != 0)
				{
					uint ms_new = anim->mask_samples, ps_new = 16 * (ms_new + anim->pixel_samples) - 9 * ms_new;
					if (anim->pixel_samples == 0) {
						anim->pixel.data = 0;
					} else {
						anim->pixel.r = 16 * anim->pixel.r * anim->pixel_samples / ps_new;
						anim->pixel.g = 16 * anim->pixel.g * anim->pixel_samples / ps_new;
						anim->pixel.b = 16 * anim->pixel.b * anim->pixel_samples / ps_new;
					}
					ms_new *= 9;
					if ((ms_new > 0xFF) || (ps_new > LX2_MAX_PS)) {
						if (ms_new > ps_new) {
							ps_new = min(LX2_MAX_PS, ps_new * 0xFF / ms_new);
							ms_new = 0xFF;
						} else {
							ms_new = min(0xFF, ms_new * LX2_MAX_PS / ps_new);
							ps_new = LX2_MAX_PS;
						}
					}
					anim->pixel_samples = ps_new;
					anim->mask_samples = ms_new;
				}
				anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
			}
			udst = udst - width + _screen.pitch;
			anim = (AnimBufferEntry *)((uint8 *)anim + (this->anim_buf_width - width) * this->anim_buf_entry_size);
		} while (--height);
		return;
	}
	if (pal == PALETTE_NEWSPAPER) {
		do {
			for (int i = 0; i != width; i++) {
				*udst = MakeGrey(*udst);
				udst++;
				anim->mask_samples = 0;
				anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
			}
			udst = udst - width + _screen.pitch;
			anim = (AnimBufferEntry *)((uint8 *)anim + (this->anim_buf_width - width) * this->anim_buf_entry_size);
		} while (--height);
		return;
	}

	DEBUG(misc, 0, "32bpp blitter doesn't know how to draw this colour table ('%d')", pal);
}

void Blitter_32bppAnimAA::SetPixel(void *video, int x, int y, uint8 colour)
{
	uint32 colour32 = LookupColourInPalette(colour);
	*((uint32 *)video + x + y * _screen.pitch) = colour32;

	/* Set the colour in the anim-buffer too, if we are rendering to the screen */
	if (_screen_disable_anim) return;

	/* We assume here (and everywhere in our implementation) that screen's individual pixel
	 * is uint32, and that _screen.pitch == _screen.width. It's always safe with current null and cocoa
	 * video backends. It's also OK with win32 video backend if it's not in 8bpp mode (and 32bpp blitter
	 * wouldn't be used for that -> no problems). For Allegro and SDL backends things are undefined.
	 * For example on 64bit system with 64bit alignment for surface/bitmap lines it might happen
	 * that pitch != width. In case there would be any real-life reports proving that it really happens
	 * code should be fixed to use safer but slower approach.
	 */
	AnimBufferEntry *anim = (AnimBufferEntry *)((uint8 *)this->anim_buf + this->anim_buf_entry_size * (x + y * this->anim_buf_width));
	anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size * ((uint32 *)video - (uint32 *)_screen.dst_ptr));

	if (colour >= PALETTE_ANIM_START) {
#if LX2_CONDENSED_AP == 1
		anim->mask[0].c = colour + 1 - PALETTE_ANIM_START;
		anim->mask[0].v = DEFAULT_BRIGHTNESS >> 5;
#else
		anim->mask[0].v = DEFAULT_BRIGHTNESS;
		anim->mask[0].c = colour;
#endif
		if (this->aa_anim_slots > 1) anim->mask[1].data = 0;
		anim->pixel.data = 0;
		anim->pixel_samples = 0;
		anim->mask_samples = 1;
	} else {
		anim->mask_samples = 0;
	}
}

void Blitter_32bppAnimAA::SetLine(void *video, int x, int y, uint8 *colours, uint width)
{
	Colour *dst = (Colour *)video + x + y * _screen.pitch;
	if (_screen_disable_anim) {
		for (uint i = 0; i < width; i++) {
			*dst = LookupColourInPalette(colours[i]);
			dst++;
		}
	} else {
		AnimBufferEntry *anim = (AnimBufferEntry *)((uint8 *)this->anim_buf + this->anim_buf_entry_size * (x + y * this->anim_buf_width));
		anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size * ((uint32 *)video - (uint32 *)_screen.dst_ptr));
		for (uint i = 0; i < width; i++) {
			*dst = LookupColourInPalette(colours[i]);
			dst++;

			anim->mask_samples = 0;
			anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
		}
	}
}

void Blitter_32bppAnimAA::DrawRect(void *video, int width, int height, uint8 colour)
{
	if (_screen_disable_anim) {
		/* This means our output is not to the screen, so we can't be doing any animation stuff, so use our parent DrawRect() */
		Blitter_32bppOptimized::DrawRect(video, width, height, colour);
		return;
	}

	uint32 colour32 = LookupColourInPalette(colour);
	AnimBufferEntry *anim = (AnimBufferEntry *)((uint8 *)this->anim_buf + this->anim_buf_entry_size * ((uint32 *)video - (uint32 *)_screen.dst_ptr));

	do {
		uint32 *dst = (uint32 *)video;

		/* We do conditional branching outside of the loop to allow auto-vectorizer to vectorize at least one loop and also
		 * because more complicated branch have next to none chances to be taken (this func is mostly used to paint widgets
		 * and windows background - and to a no surprise they are seldomly painted with palette-animated colors). */
		if (colour >= PALETTE_ANIM_START) {
			for (int i = width; i > 0; i--) {
				*dst = colour32;
#if LX2_CONDENSED_AP == 1
				anim->mask[0].c = colour + 1 - PALETTE_ANIM_START;
				anim->mask[0].v = DEFAULT_BRIGHTNESS >> 5;
#else
				anim->mask[0].c = colour;
				anim->mask[0].v = DEFAULT_BRIGHTNESS;
#endif
				if (this->aa_anim_slots > 1) anim->mask[1].data = 0;
				anim->pixel.data = 0;
				anim->pixel_samples = 0;
				anim->mask_samples = 1;
				dst++;
				anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
			}
		} else {
			for (int i = width; i > 0; i--, dst++) {
				*dst = colour32;
				anim->mask_samples = 0;
				anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
			}
		}

		video = (uint32 *)video + _screen.pitch;
		anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size * (this->anim_buf_width - width));
	} while (--height);
}

void Blitter_32bppAnimAA::CopyFromBuffer(void *video, const void *src, int width, int height)
{
	assert(!_screen_disable_anim);
	assert(video >= _screen.dst_ptr && video <= (uint32 *)_screen.dst_ptr + _screen.width + _screen.height * _screen.pitch);
	uint32 *dst = (uint32 *)video;
	const uint32 *usrc = (const uint32 *)src;
	AnimBufferEntry * anim_line = (AnimBufferEntry *)((uint8 *)this->anim_buf + this->anim_buf_entry_size * ((uint32 *)video - (uint32 *)_screen.dst_ptr));

	for (uint y = height; y > 0; y--) {
		memcpy(dst, usrc, width * sizeof(uint32));
		usrc += width;
		dst += _screen.pitch;
		/* Copy back a chunk of the anim-buffer */
		memcpy(anim_line, usrc, width * this->anim_buf_entry_size);
		usrc = (const uint32 *)((uint8 *)usrc + width * this->anim_buf_entry_size);
		anim_line = (AnimBufferEntry *)((uint8 *)anim_line + this->anim_buf_width * this->anim_buf_entry_size);
	}

	uint x = ((uint32 *)video - (uint32 *)_screen.dst_ptr) % _screen.pitch;
	uint y = ((uint32 *)video - (uint32 *)_screen.dst_ptr) / _screen.pitch;
	this->doPaletteAnimate(x, y, width, height);
}

void Blitter_32bppAnimAA::CopyToBuffer(const void *video, void *dst, int width, int height)
{
	assert(!_screen_disable_anim);
	assert(video >= _screen.dst_ptr && video <= (uint32 *)_screen.dst_ptr + _screen.width + _screen.height * _screen.pitch);

	if (this->anim_buf == NULL) return; ///<TODO: Check if it's correct

	uint32 *udst = (uint32 *)dst;
	const uint32 *src = (const uint32 *)video;
	const AnimBufferEntry *anim_line = (AnimBufferEntry *)((uint8 *)this->anim_buf + this->anim_buf_entry_size * ((const uint32 *)video - (uint32 *)_screen.dst_ptr));

	for (; height > 0; height--) {
		memcpy(udst, src, width * sizeof(uint32));
		src += _screen.pitch;
		udst += width;
		/* Copy a chunk from the anim-buffer. */
		memcpy(udst, anim_line, width * this->anim_buf_entry_size);
		udst = (uint32 *)((uint8 *)udst + width * this->anim_buf_entry_size);
		anim_line = (const AnimBufferEntry *)((const uint8 *)anim_line + this->anim_buf_width * this->anim_buf_entry_size);
	}
}

void Blitter_32bppAnimAA::ScrollBuffer(void *video, int &left, int &top, int &width, int &height, int scroll_x, int scroll_y)
{
	assert(!_screen_disable_anim);
	assert(video >= _screen.dst_ptr && video <= (uint32 *)_screen.dst_ptr + _screen.width + _screen.height * _screen.pitch);
	AnimBufferEntry *dst, *src;

	/* We need to scroll the anim-buffer too */
	if (scroll_y > 0) {
		dst = (AnimBufferEntry *)((uint8 *)this->anim_buf + this->anim_buf_entry_size * (left + this->anim_buf_width * (top + height - 1)));
		src = (AnimBufferEntry *)((uint8 *)this->anim_buf + this->anim_buf_entry_size * (left + this->anim_buf_width * (top + height - 1 - scroll_y)));

		/* Adjust left & width */
		if (scroll_x >= 0) {
			dst = (AnimBufferEntry *)((uint8 *)dst + scroll_x * this->anim_buf_entry_size);
		} else {
			src = (AnimBufferEntry *)((uint8 *)src - scroll_x * this->anim_buf_entry_size);
		}

		uint tw = width + (scroll_x >= 0 ? -scroll_x : scroll_x);
		uint th = height - scroll_y;
		for (; th > 0; th--) {
			memcpy(dst, src, tw * this->anim_buf_entry_size);
			src= (AnimBufferEntry *)((uint8 *)src - this->anim_buf_entry_size * this->anim_buf_width);
			dst= (AnimBufferEntry *)((uint8 *)dst - this->anim_buf_entry_size * this->anim_buf_width);
		}
	}
	else
	{
		/* Calculate pointers */
		dst = (AnimBufferEntry *)((uint8 *)this->anim_buf + this->anim_buf_entry_size * (left + top * this->anim_buf_width));
		src = (AnimBufferEntry *)((uint8 *)this->anim_buf + this->anim_buf_entry_size * (left + (top - scroll_y) * this->anim_buf_width));

		/* Adjust left & width */
		if (scroll_x >= 0) {
			dst = (AnimBufferEntry *)((uint8 *)dst + scroll_x * this->anim_buf_entry_size);
		} else {
			src = (AnimBufferEntry *)((uint8 *)src - scroll_x * this->anim_buf_entry_size);
		}

		/* the y-displacement may be 0 therefore we have to use memmove,
		 * because source and destination may overlap */
		uint tw = width + (scroll_x >= 0 ? -scroll_x : scroll_x);
		uint th = height + scroll_y;
		for (; th > 0; th--) {
			memmove(dst, src, tw * this->anim_buf_entry_size);
			src= (AnimBufferEntry *)((uint8 *)src + this->anim_buf_entry_size * this->anim_buf_width);
			dst= (AnimBufferEntry *)((uint8 *)dst + this->anim_buf_entry_size * this->anim_buf_width);
		}
	}

	Blitter_32bppBase::ScrollBuffer(video, left, top, width, height, scroll_x, scroll_y);
}

int Blitter_32bppAnimAA::BufferSize(int width, int height)
{
	/* Per one real screen pixel we store one AnimBufferEntry */
	return width * height * (sizeof(uint32) + this->anim_buf_entry_size);
}

void Blitter_32bppAnimAA::_stDrawAnimThreadProc(void * data)
{
	((const Blitter_32bppAnimAA::AnimThreadInfo *)data)->self->DrawAnimThread((const Blitter_32bppAnimAA::AnimThreadInfo *)data);
}


void Blitter_32bppAnimAA::DrawAnimThread(const Blitter_32bppAnimAA::AnimThreadInfo *ath)
{
	assert(ath->self == this);

	ath->th_mutex_in->BeginCritical();
	ath->th_mutex_in->WaitForSignal();

	/* A note about thread safety: we do not misuse volatile for "quit loop condition" below. This is due to we do use locks inside the loop and
	 * threading library should take care of having full memory barrier for both compiler and CPU here. If the compiler or the CPU are not forced
	 * to make things right by the threading lib - using volatile for this var might look like a "solution" but actually it'd be just a matter of
	 * luck if things would work in the expected way. Compiler vs. threading lib combo is what should be fixed for such case and not our code. */
	while(this->anim_aa_continue_animate)
	{
		this->doPaletteAnimate(ath->left, ath->top, ath->width, ath->height);
		/* Threads syncronisation could be done using InterlockedDecrement on Win32/64 and using __sync_sub_and_fetch GCC builtins on archs where
		 * they're available. Implementing it that way would be better from performance PoV (would save us about worker_threads_count locks/unlocks
		 * per one complete job and would keep us from issues related to scheduling granularity). It is not portable and thus in case we're about
		 * to implement it in that way we should provide a fallback for archs where required atomic functions are not supported/available
		 * and use some configure-driven conditional compilation chosing most profitable execution path for given target arch. It is clearly
		 * possible to do (and we have tested it interlally for Win32/64 and Linux on IA-32 and AMD64) but for now we stick with a conservative
		 * approach which would work and serve well enough for this case. It might be a "proper" way to head for implementing per-sprite
		 * multithreaded Draw() and in case it'd be done there - it could also be copied here. Downside is that we would essentially spinlock instead
		 * of using OS-provided services to sleep thus CPU hogging would be severe. Then again it could be dealt with using techniques like
			 * "limited iterations spinlock" like one that is implemented internally on Win32/64 for "...SpinCount" part of critical section objects. */
		this->th_mutex_out->BeginCritical();
		this->anim_threads_jobs_active--;
		if (this->anim_threads_jobs_active == 0) this->th_mutex_out->SendSignal();
		this->th_mutex_out->EndCritical();
		ath->th_mutex_in->WaitForSignal();
	}
	ath->th_mutex_in->EndCritical();
	this->th_mutex_out->BeginCritical();
	this->th_mutex_out->SendSignal();
	this->th_mutex_out->EndCritical();
}

void Blitter_32bppAnimAA::doPaletteAnimate(uint left, uint top, uint width, uint height)
{
	AnimBufferEntry *anim = (AnimBufferEntry *)((uint8 *)this->anim_buf + this->anim_buf_entry_size * (top * this->anim_buf_width + left));
	uint32 *dst = (uint32 *)_screen.dst_ptr + top * _screen.pitch + left;
	uint blend_qty = 0;
	Colour tmp;

	/* Let's walk the anim buffer and try to find the pixels */
	for (int y = height; y != 0 ; y--) {
		for (int x = width; x != 0 ; x--) {
			/* Do we need to update this pixel? */
			if (anim->mask_samples != 0) {
				/* Skipping math here could give essential speedup (up to 5-10x for some corner cases) but trouble is that we can't directly
				 * rely on mask_samples as a measure of the real used anim slots - its value could had been rebalanced.
				 * But we can detect that rebalance might had been done by checking if pixel_samples is non-zero as that's a property
				 * warranted to held for BP_TRANSPARENT Draw() rebalancing, i.e. we know that if mask_samples is 1 and pixel_samples is non-zero
				 * than it's possible that values had been rebalanced and then we can't trust mask_samples to reflect a number of anim slots used. */
				if ((this->aa_anim_slots > 1) && ((anim->mask_samples > 1) || (anim->pixel_samples != 0))) {
					uint32 r = 0, g = 0, b = 0;
					for (blend_qty = 0; blend_qty < this->aa_anim_slots && anim->mask[blend_qty].data != 0; blend_qty++) {
						tmp.data = this->cached_palette[anim->mask[blend_qty].data].data;
						r += tmp.r;
						g += tmp.g;
						b += tmp.b;
					}

					/* Looks ugly but provides a really nice boost for typical aa_anim_slots values */
					switch (blend_qty) {
					case 2:
						tmp.data = (0xFF << 24) | ((r << 15) & 0xFF0000) | ((g << 7) & 0xFF00) | ((b >> 1) & 0xFF);
						break;
					case 4:
						tmp.data = (0xFF << 24) | ((r << 14) & 0xFF0000) | ((g << 6) & 0xFF00) | ((b >> 2) & 0xFF);
						break;
					case 8:
						tmp.data = (0xFF << 24) | ((r << 13) & 0xFF0000) | ((g << 5) & 0xFF00) | ((b >> 3) & 0xFF);
						break;
					case 16:
						tmp.data = (0xFF << 24) | ((r << 12) & 0xFF0000) | ((g << 4) & 0xFF00) | ((b >> 4) & 0xFF);
						break;
					case 32:
						tmp.data = (0xFF << 24) | ((r << 11) & 0xFF0000) | ((g << 3) & 0xFF000) | ((b >> 5) & 0xFF);
						break;
					case 64:
						tmp.data = (0xFF << 24) | ((r << 10) & 0xFF0000) | ((g << 2) & 0xFF00) | ((b >> 6) & 0xFF);
						break;
					case 128:
						tmp.data = (0xFF << 24) | ((r << 9) & 0xFF0000) | ((g << 1) & 0xFF00) | ((b >> 7) & 0xFF);
						break;
					case 255:
					case 256:
						tmp.data = (0xFF << 24) | ((r << 8) & 0xFF0000) | (g & 0xFF00) | ((b >> 8) & 0xFF);
						break;
					default:
						tmp.data = (0xFF << 24) | ((r / blend_qty) << 16) | ((g / blend_qty) << 8) | (b / blend_qty);
					}
				} else {
					tmp.data = this->cached_palette[anim->mask[0].data].data;
				}

				if (anim->pixel_samples != 0)
				{
					uint weight_s = anim->mask_samples + anim->pixel_samples;
					tmp.r = (((uint32)(anim->mask_samples)) * tmp.r + anim->pixel_samples * anim->pixel.r) / weight_s;
					tmp.g = (((uint32)(anim->mask_samples)) * tmp.g + anim->pixel_samples * anim->pixel.g) / weight_s;
					tmp.b = (((uint32)(anim->mask_samples)) * tmp.b + anim->pixel_samples * anim->pixel.b) / weight_s;
				}

				*dst = tmp.data;
			}
			dst++;
			anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size);
		}
		dst += _screen.pitch - width;
		anim = (AnimBufferEntry *)((uint8 *)anim + this->anim_buf_entry_size * (this->anim_buf_width - width));
	}
}


void Blitter_32bppAnimAA::PaletteAnimateThreaded()
{
	assert(!_screen_disable_anim);

	this->th_mutex_out->BeginCritical();
	assert(this->anim_threads_jobs_active == 0);
	this->anim_threads_jobs_active = this->anim_threads_qty;

	for (int i = 0; i < this->anim_threads_qty; i++)
	{
		this->anim_ti[i].th_mutex_in->BeginCritical();
		this->anim_ti[i].th_mutex_in->SendSignal();
		this->anim_ti[i].th_mutex_in->EndCritical();
	}

	/* We had signaled worker threads that they've got some work to do, let's do out part */
	uint top = this->anim_threads_qty * (this->anim_buf_height / (this->anim_threads_qty + 1));
	this->doPaletteAnimate(0, top, this->anim_buf_width, this->anim_buf_height - top);

	while (this->anim_threads_jobs_active > 0) this->th_mutex_out->WaitForSignal();
	assert(this->anim_threads_jobs_active == 0);
	this->th_mutex_out->EndCritical();
}

void Blitter_32bppAnimAA::PaletteAnimate(const Palette &palette)
{
	assert(!_screen_disable_anim);

	this->palette = palette;
	/* If first_dirty is 0, it is for 8bpp indication to send the new
	 *  palette. However, only the animation colours might possibly change.
	 *  Especially when going between toyland and non-toyland. */
	assert(this->palette.first_dirty == PALETTE_ANIM_START || this->palette.first_dirty == 0);

	this->UpdateCachedPalette(this->palette.first_dirty, this->palette.count_dirty);

	if (this->anim_threaded)
		this->PaletteAnimateThreaded();
	else
		this->doPaletteAnimate(0, 0, this->anim_buf_width, this->anim_buf_height);

	/* FIXME: It is wrong to have it here. We are only called by the videodriver and it should know without our advice that it is required
	 * to redisply the entire screen. As of now the only driver that acts semi-correctly with this is win32_v - it invalidates the entire
	 * window each time after calling the GameLoop() in case any colour in palette became dirty. Including the palette animated range.
	 * It makes Windows produce WM_PAINT message in not so distant future which triggers the redraw. With next call in place things got worse:
	 * for each palette animate call we get two screen updates. It happens because next call invalidates the entire screen once again right
	 * before the video driver performs the actual blit. In response Windows sends yet another WM_PAINT message that forces video driver to
	 * redraw the screen again. What a waste of CPU time! To fix this either win32_v driver should be made broken in a manner similar to
	 * other drivers or other drivers should be fixed to gracefully update the entire screen after they call PaletteAnimate(). */
	_video_driver->MakeDirty(0, 0, _screen.width, _screen.height);
}

void Blitter_32bppAnimAA::UpdateCachedPalette(uint8 first_dirty, int16 count_dirty) {
	assert(count_dirty < (257 - first_dirty));
#if LX2_CONDENSED_AP == 1
	if (first_dirty < PALETTE_ANIM_START) {
		count_dirty -= (PALETTE_ANIM_START - first_dirty);
		if (count_dirty < 0) return;
		first_dirty = 0;
	} else {
		first_dirty -= PALETTE_ANIM_START;
	}
	for (int v = 0; v < (1 << 3); v++) {
		for (int c = 0; c < count_dirty; c++) {
			this->cached_palette[(v << 5) | (c + first_dirty + 1)].data = this->AdjustBrightness(this->LookupColourInPalette(c + PALETTE_ANIM_START), v << 5);
		}
	}
#else
	for (int v = 0; v < 256; v++) {
		for (int c = first_dirty; c < (first_dirty + count_dirty); c++) {
			this->cached_palette[(v << 8) | c].data = this->AdjustBrightness(this->LookupColourInPalette(c), v);
		}
	}
#endif
}

Blitter::PaletteAnimation Blitter_32bppAnimAA::UsePaletteAnimation()
{
	return Blitter::PALETTE_ANIMATION_BLITTER;
}

void Blitter_32bppAnimAA::PostResize()
{
	/* We rely on PostResize() being called by the same thread that calls PaletteAnimate().
	 * If things would be decoupled in future - we would have to dance with mutexes to be safe. */
	if ((_screen.width << 1) != this->anim_buf_width || (_screen.height << 1) != this->anim_buf_height) {
		/* The size of the screen changed; we can assume we can wipe all data from our buffer */
		free(this->anim_buf);
		this->anim_buf = (AnimBufferEntry *)calloc(_screen.width * _screen.height, this->anim_buf_entry_size);
		this->anim_buf_width = _screen.width;
		this->anim_buf_height = _screen.height;

		if (this->anim_threaded) {
			/* Critical section stuff here isn't really needed, at least in that amounts, because we should be called from the same thread that calls PaletteAnimate()
			 * and our multithreaded worker is designed in a way that PaletteAnimate() won't return until all workers are done and wait on mutexes for signal.
			 * In case OTTD engine structure would be changed in a way so there would be a chance for being called from a different thread - we would have to redo
			 * this one with proper locking to ensure that workers would not touch the buffer the entire time we are busy reallocating it and updating vars. */
			this->th_mutex_out->BeginCritical();
			for (int i = 0; i < this->anim_threads_qty; i++)
			{
				this->anim_ti[i].th_mutex_in->BeginCritical();
				this->anim_ti[i].left = 0;
				this->anim_ti[i].width = this->anim_buf_width;
				this->anim_ti[i].top = i * (this->anim_buf_height / (this->anim_threads_qty + 1));
				this->anim_ti[i].height = this->anim_buf_height / (this->anim_threads_qty + 1);
				this->anim_ti[i].th_mutex_in->EndCritical();
			}
			this->th_mutex_out->EndCritical();
		}
	}
}

inline uint32 Blitter_32bppAnimAA::BlendPixels(const Colour* pixels, uint count)
{
	/* Accordingly to common sense (and gcov output) this func is one of the hottest points of the blitter.
	 * I really want this one to be as optimized as possible. For now it's mostly a generic implementation in C
	 * and hope is that compiler would do its best optimizing things. It might be reasonable in future to create
	 * a separate version coded in assembler and using SSE for performing calculations (auto-vectorized might fail). */

	/* It used to be two loops here for optimizition purpose, one counting sum(a) and another doind rest of work
	 * only in case sum(a) != 0. gcov proved that benefits are next to none due to only ~2% of blended together pixels
	 * being fully transparent. Thus we merged loops into one that could be auto-vectorized. */
	uint32 r = 0, g = 0, b = 0, a = 0;
	for(uint i = count; i > 0; i--, pixels++)
	{
		a += pixels->a;
		r += pixels->r * pixels->a;
		g += pixels->g * pixels->a;
		b += pixels->b * pixels->a;
	}
	if (a == 0)
		return 0;
	else
		return ((a / count) << 24) | ((r / a) << 16) | ((g / a) << 8) | (b / a);
}


Sprite *Blitter_32bppAnimAA::Encode(SpriteLoader::Sprite *sprite, AllocatorProc *allocator)
{
	assert(this->temp_pixels != NULL);

	uint32 *dst_px_orig[ZOOM_LVL_COUNT];
	uint32 lengths[ZOOM_LVL_COUNT];

	ZoomLevel zoom_min;
	ZoomLevel zoom_max;

	if (sprite->type == ST_FONT) {
		zoom_min = ZOOM_LVL_NORMAL;
		zoom_max = ZOOM_LVL_NORMAL;
	} else {
		zoom_min = _settings_client.gui.zoom_min;
		zoom_max = _settings_client.gui.zoom_max;
		if (zoom_max == zoom_min) zoom_max = ZOOM_LVL_MAX;
	}

	if (this->remap_pixels == NULL)
		this->remap_pixels = MallocT<const SpriteLoader::CommonPixel *>(this->aa_level * this->aa_level);

	for (ZoomLevel z = zoom_min; z <= zoom_max; z++) {

		ZoomLevel aa_z = z;
		for (uint a = this->aa_level; (a > 1) && (aa_z != ZOOM_LVL_MIN); a >>= 1)
			aa_z--;
		uint real_aa_level = 1 << (z - aa_z);

		const SpriteLoader::Sprite *src_orig = &sprite[z], *src_orig_aa = &sprite[aa_z];

		uint size = src_orig->height * src_orig->width;

		/* Have to allocate memory here so it would be sufficient for storing encoded stream for the worst case scenario,
		 * which is: each pixel from Class 3, so we have to store at most this->aa_level * this->aa_level + 1 uint32 records per pixel,
		 * plus one uint32 per line for size record, plus 1 + width/0x1000000 per file for RLE-group records. */
		dst_px_orig[z] = MallocT<uint32>(src_orig->height * (2 + src_orig->width / 0x1000000) + (real_aa_level * real_aa_level + 1) * size);

#ifdef _DEBUG
		/* In case building debug version we use this one in a number of assert checks */
		uint32 *dst_end = dst_px_orig[z] + src_orig->height * (2 + src_orig->width / 0x1000000) + (real_aa_level * real_aa_level + 1) * size;
#endif

		uint32 *dst_px_ln = dst_px_orig[z];

		const SpriteLoader::CommonPixel *src_base = (const SpriteLoader::CommonPixel *)src_orig_aa->data,
				*src_end = src_base + src_orig_aa->width * src_orig_aa->height;

		while(src_base < src_end) {

			uint32 *dst_px = dst_px_ln + 1, *dst_class = dst_px++;

			uint last = 4;
			uint32 len = 0;

			for (uint x = 0; x < src_orig->width; x++) {

				/* Time to gather pointers to sub-samples this pixel consists of */
				uint blend_qty = 0, transp_qty = 0, alpha_qty = 0, remap_qty = 0, anim_qty = 0;
				int force_something_remap_idx = -1;
				bool force_opaque = false;

				for (uint aay = 0; aay < real_aa_level; aay++)
				{
					const SpriteLoader::CommonPixel *src = src_base + aay * src_orig_aa->width;
					if (src >= src_end) break;
					for(uint aax = 0; aax < real_aa_level; aax++)
					{
						if ((x * real_aa_level + aax) >= src_orig_aa->width) break;
						/* We've got the pointer to the pixel to work with. Time to classify it. */
						if (src->a < 2)
							/* Type 1 (fully transparent) pixel */
							transp_qty++;
						else
						{
							alpha_qty += (src->a < 254) ? 1 : 0;
							if (src->m == 0)
							{
								/* Type 2 or 3 pixel (opaque or alpha-blended 32bpp) */
								this->temp_pixels[blend_qty++].data = this->ComposeColour((src->a == 254) ? 255 : src->a, src->r, src->g, src->b);
							}
							else
							{
								if ((sprite->type == ST_FONT) || (src->m >= PALETTE_ANIM_START) || this->isRemappedColour(src->m))
								{
									/* Type 5 (palette remap or anim) pixel */

									/* We store all non-anim sub-samples in encoded stream but only this->aa_anim_slots anim sub-samples at max.
									 * This selective storage approach has a non obvious consequence that we have to deal with force_opaque
									 * flag in some graceful way. As we assume that every anim-pixel would always end up being opaque and
									 * as we reserve first slots in remap_pixels array for resp. anim slots we could use it when specifying
									 * force_opaque_index - it doesn't matter if the pixel force_opaque_index is pointing to is really one
									 * that had been anim-pixel staying last at the first line of sub-samples as long as this index points
									 * to any anim sub-pixel. End result after rendering would be exactly the same, i.e. we would get 100%
									 * opaque pixel in any case. */

									if (src->m >= PALETTE_ANIM_START)
									{
										if ( (this->aa_anim_slots >= this->aa_level * this->aa_level) ||
												((this->aa_anim_slots - (anim_qty - this->aa_anim_slots) * this->aa_anim_slots / (this->aa_level * this->aa_level - this->aa_anim_slots)) > (anim_qty % this->aa_anim_slots)))
										{
											this->remap_pixels[remap_qty] = src;
											/* This pixel currently is in the anim buffer slot, place it accordingly in pixels_to_blend array. */
											if (remap_qty != (anim_qty % this->aa_anim_slots))
												Swap(this->remap_pixels[remap_qty], this->remap_pixels[anim_qty % this->aa_anim_slots]);
											if(anim_qty < this->aa_anim_slots) remap_qty++;
										}
										anim_qty++;
									}
									else
										this->remap_pixels[remap_qty++] = src;
								}
								else
								{
									/* Type 4 (static 8bpp or masked 32bpp) pixel */

									uint8 rgb_max = max(src->r, max(src->g, src->b));
									uint32 colour;

									/* Black pixel (8bpp or old 32bpp image), so use default value */
									if (rgb_max == 0)
										/* Save ourselves from extra store and cmp, get larger code in return */
										colour = this->LookupColourInGfxPalette(src->m);
									else
										colour = this->AdjustBrightness(this->LookupColourInGfxPalette(src->m), rgb_max);

									this->temp_pixels[blend_qty].a = (src->a == 254) ? 255 : src->a;
									this->temp_pixels[blend_qty].r = GB(colour, 16, 8);
									this->temp_pixels[blend_qty].g = GB(colour, 8,  8);
									this->temp_pixels[blend_qty].b = GB(colour, 0,  8);
									blend_qty++;
								}
							}
						}

						src++;
					}
					if (aay == 0)
					{
						/* We're at the edge of the first line of sub-samples. Time to track back the pixel that would had determine target
						 * pixel colour and transparency with original resize algo. */
						src--;
						for (int i = min(real_aa_level, src_orig_aa->width - (x * real_aa_level)) - 1; (i > 0) && (src->a == 0); i--) src--;

						/* Insert #if 0 here to disable "force_opaque/force_transparent" hack */
						if (src->a == 0)
						{
							/* Original algo would result in fully transparent pixel, we have to simulate it. */
							remap_qty = blend_qty = alpha_qty = 0;
							break;
						}
						force_opaque = (src->a == 255);
						/* Insert #endif here to disable "force_opaque/force_transparent" hack */

						if (force_opaque && (remap_qty > 0))
						{
							if (src->m >= PALETTE_ANIM_START)
								force_something_remap_idx = 0;
							else if (src == remap_pixels[remap_qty - 1])
								force_something_remap_idx = remap_qty - 1;
						}
					}
				}

				/* Wow, that was tough :-). But now we have _almost_ all the bits we need to classify and encode.
				 * What we have to additionally care about is the class 1/2 case with blended together pixels
				 * resulting in fully transparent one after taking transp_qty into account. So let's blend first
				 * if needed and only then judge. */

				if (blend_qty > 0)
				{
					this->temp_pixels[blend_qty].data = this->BlendPixels(this->temp_pixels, blend_qty);
					/* Have to scale down alpha in case not forcing opaque and there were any fully transparent pixels */
					if (force_opaque) this->temp_pixels[blend_qty].a = 255;
					else if (transp_qty > 0)
						this->temp_pixels[blend_qty].a = blend_qty * this->temp_pixels[blend_qty].a / (transp_qty + blend_qty);
				}

				uint t = 0;
				if (remap_qty > 0) t = 3;
				else
				{
					if ((blend_qty > 0) && (this->temp_pixels[blend_qty].a > 0)) {
						/* We have our pixels blended already respecting the value of the force_opaque flag so it's easier use the result to judge */
						t = (this->temp_pixels[blend_qty].a == 255) ? 1 : 2;
					}
				}

				if (last != t || len == 0xFFFFFF) {
					if (last != 4) {
#ifdef _DEBUG
						assert(dst_end > dst_class);
#endif
						*dst_class = (last << 24) | len;
						dst_class = dst_px++;
					}
					len = 0;
				}

				last = t;
				len++;

				src_base += real_aa_level;

				if (t == 0) continue;

				if (t == 3)
				{
#ifdef _DEBUG
					assert(dst_end > dst_px);
#endif
					*dst_px = 0;
					if((blend_qty > 0) || (!force_opaque && (transp_qty > 0))) *dst_px |= 1 << 31;
					if(force_something_remap_idx != -1)
					{
						*dst_px |= (1 << 30);
						Swap(this->remap_pixels[0], this->remap_pixels[force_something_remap_idx]);
					}
					for(uint i = 0; i < remap_qty; i++)
					{
#ifdef _DEBUG
						assert(dst_end > dst_px);
#endif
						if(i % 32 == 0)
						{
							uint qty_left = remap_qty - i - 1;
							*dst_px |= (min(0x1F, qty_left) & 0x1F) << 24;
							if(qty_left > 0x1F) *dst_px |= 0x20 << 24;
						}

						uint8 rgb_max = max(this->remap_pixels[i]->r, max(this->remap_pixels[i]->g, this->remap_pixels[i]->b));
						if (rgb_max == 0) rgb_max = DEFAULT_BRIGHTNESS;

						if (this->remap_pixels[i]->m >= PALETTE_ANIM_START)
							/* Special case treatment of anim pixels: we assume that alpha for them is always 255 so we don't have to store it,
							 * but we do need to store real amount of anim sub-samples original pixel had so we could properly blend them when drawing.
							 * We reuse alpha channel byte for storing this value (capped on 0xFF). */
							*dst_px |= (min(anim_qty, 0xFF) << 16) | (rgb_max << 8) | this->remap_pixels[i]->m;
						else
							*dst_px |= (this->remap_pixels[i]->a << 16) | (rgb_max << 8) | this->remap_pixels[i]->m;

						dst_px++;
#ifdef _DEBUG
						assert(dst_end > dst_px);
#endif
						*dst_px = 0;
					}

					if ((blend_qty > 0) || (!force_opaque && (transp_qty > 0)))
					{
						/* Why to store sub-pixels quantity for pre-blended part? That's because of "coverage". What's "coverage"?
						 * Imagine that we do 4x AA and original sprite we use as an AA data source has 26x26 dimensions.
						 * Resize is done with rounding up, so we'd get 13x13, 7x7 and 4x4 sprites when resized down.
						 * It means that for pixel located at bottom right corner of the smallest zoom level ((3,3) coordinates)
						 * we only have 1 subpixel instead of usual 16. Per sub-sample coverage for this case is 100%.
						 * For pixel at the bottom right corner of 7x7 sprite we have 4 subpixels to work with instead of 16.
						 * Here coverage is 1/4 = 25%. Maximum possible value for coverage is 100%, minimum is 1/(this->aa_level*this->aa_level).
						 * Total sub-pixels quantity for given pixel = 1/COVERAGE.
						 * What is means that pre-blended pixel should influence on the final result in proportion to the number of
						 * subpixels we used to compose this pre-blended pixes. So we have to store it. */

#ifdef _DEBUG
						assert(dst_end > dst_px);
#endif
						/* Store sub-pixels quantity used to compose pre-blended pixel part */
						*dst_px++ = blend_qty + transp_qty;
						if(blend_qty == 0)
							this->temp_pixels[blend_qty].data = 0;
#ifdef _DEBUG
						assert(dst_end > dst_px);
#endif
						*dst_px = this->temp_pixels[blend_qty].data;
						dst_px++;
					}
				}
				else
				{
#ifdef _DEBUG
					assert(dst_end > dst_px);
#endif
					assert(this->temp_pixels[blend_qty].a != 0);
					/* Class 1 or 2 - store one pre-blended value. */
					*dst_px = this->temp_pixels[blend_qty].data;
					dst_px++;
				}

			}

			if (last != 4) {
#ifdef _DEBUG
						assert(dst_end > dst_class);
#endif
				*dst_class = (last << 24) | len;
			}

#ifdef _DEBUG
			assert(dst_end > dst_px_ln);
#endif
			*dst_px_ln = dst_px - dst_px_ln;
			dst_px_ln = dst_px;
			/* It is really go back src_orig->width * real_aa_level pixels (i.e. back to the beginning of the line) and then move forward real_aa_level lines */
			src_base += (src_orig_aa->width - src_orig->width) * real_aa_level;
		}

		lengths[z] = dst_px_ln - dst_px_orig[z];
	}

	uint len = 0; // total length of data
	for (ZoomLevel z = zoom_min; z <= zoom_max; z++)
		len += lengths[z];

	Sprite *dest_sprite = (Sprite *)allocator(sizeof(*dest_sprite) + sizeof(SpriteData) + len * sizeof(uint32));

	dest_sprite->height = sprite->height;
	dest_sprite->width  = sprite->width;
	dest_sprite->x_offs = sprite->x_offs;
	dest_sprite->y_offs = sprite->y_offs;

	SpriteData *dst = (SpriteData *)dest_sprite->data;
	memset(dst, 0, sizeof(*dst));

	for (ZoomLevel z = zoom_min; z <= zoom_max; z++) {
		dst->offset[z] = z == zoom_min ? 0 : lengths[z - 1] + dst->offset[z - 1];
		memcpy(dst->data + dst->offset[z], dst_px_orig[z], sizeof(*dst->data) * lengths[z]);
		free(dst_px_orig[z]);
	}

	return dest_sprite;
}
