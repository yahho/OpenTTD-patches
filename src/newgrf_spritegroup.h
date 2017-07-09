/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_spritegroup.h Action 2 handling. */

#ifndef NEWGRF_SPRITEGROUP_H
#define NEWGRF_SPRITEGROUP_H

#include <deque>

#include "core/align.h"
#include "core/pointer.h"
#include "core/flexarray.h"

#include "town_type.h"
#include "engine_type.h"
#include "house_type.h"

#include "newgrf_callbacks.h"
#include "newgrf_generic.h"
#include "newgrf_storage.h"
#include "newgrf_commons.h"

/**
 * Gets the value of a so-called newgrf "register".
 * @param i index of the register
 * @pre i < 0x110
 * @return the value of the register
 */
static inline uint32 GetRegister(uint i)
{
	extern TemporaryStorageArray<int32, 0x110> _temp_store;
	return _temp_store.GetValue(i);
}

/* List of different sprite group types */
enum SpriteGroupType {
	SGT_REAL,
	SGT_DETERMINISTIC,
	SGT_RANDOMIZED,
	SGT_CALLBACK,
	SGT_RESULT,
	SGT_TILELAYOUT,
	SGT_INDUSTRY_PRODUCTION,
};

struct SpriteGroup;
typedef uint32 SpriteGroupID;
struct ResolverObject;

/* SPRITE_WIDTH is 24. ECS has roughly 30 sprite groups per real sprite.
 * Adding an 'extra' margin would be assuming 64 sprite groups per real
 * sprite. 64 = 2^6, so 2^30 should be enough (for now) */

/* Common wrapper for all the different sprite group types */
struct SpriteGroup {
protected:
	static const size_t MAX_SIZE = 1 << 30;

	static std::deque <ttd_unique_ptr <SpriteGroup> > pool;

	/**
	 * Append a new sprite group to the pool.
	 * @param group The sprite group to append.
	 * @return The sprite group itself, for the convenience of our callers.
	 */
	template <typename T>
	static T *append (T *group)
	{
		assert (pool.size() < MAX_SIZE);
		pool.push_back (ttd_unique_ptr <SpriteGroup> (group));
		return group;
	}

	const SpriteGroupType type; ///< Type of the sprite group

	CONSTEXPR SpriteGroup (SpriteGroupType type) : type (type)
	{
	}

	/** Base sprite group resolver */
	virtual const SpriteGroup *Resolve (ResolverObject &object) const
	{
		return this;
	}

public:
	virtual ~SpriteGroup()
	{
	}

	bool IsType (SpriteGroupType type) const
	{
		return this->type == type;
	}

	virtual SpriteID GetResult (void) const
	{
		return 0;
	}

	virtual byte GetNumResults (void) const
	{
		return 0;
	}

	virtual uint16 GetCallbackResult (void) const
	{
		return CALLBACK_FAILED;
	}

	static const SpriteGroup *Resolve(const SpriteGroup *group, ResolverObject &object, bool top_level = true);

	/**
	 * Get a callback result from a SpriteGroup.
	 * @return Callback result.
	 */
	static uint16 CallbackResult (const SpriteGroup *result)
	{
		return result != NULL ? result->GetCallbackResult() : CALLBACK_FAILED;
	}

	/** Clear the sprite group pool. */
	static void clear (void)
	{
		pool.clear();
	}
};


/* 'Real' sprite groups contain a list of other result or callback sprite
 * groups. */
struct RealSpriteGroup : SpriteGroup, FlexArray <SpriteGroup *> {
private:
	/* Loaded = in motion, loading = not moving
	 * Each group contains several spritesets, for various loading stages */

	/* XXX: For stations the meaning is different - loaded is for stations
	 * with small amount of cargo whilst loading is for stations with a lot
	 * of da stuff. */

	const uint16 n;              ///< Total number of groups
	const byte n1;               ///< Number of loaded groups
	const byte n2;               ///< Number of loading groups
	const SpriteGroup *groups[]; ///< List of groups (can be SpriteIDs or Callback results)

	RealSpriteGroup (uint16 n, byte n1, byte n2)
		: SpriteGroup (SGT_REAL), n (n), n1 (n1), n2 (n2)
	{
		memset (this->groups, 0, n * sizeof(SpriteGroup *));
	}

protected:
	const SpriteGroup *Resolve (ResolverObject &object) const OVERRIDE;

public:
	static RealSpriteGroup *create (byte n1, byte n2)
	{
		uint16 total = (uint16)n1 + (uint16)n2;
		return SpriteGroup::append (new (total) RealSpriteGroup (total, n1, n2));
	}

	void set (uint i, const SpriteGroup *group)
	{
		this->groups[i] = group;
	}

	/**
	 * Get the count of sprite groups.
	 * @param alt Count the groups in the second (loading) set.
	 * @return The count of sprite groups in the set.
	 */
	uint get_count (bool alt) const
	{
		return alt ? this->n2 : this->n1;
	}

	/**
	 * Get a particular sprite group.
	 * @param alt Look for the group in the second (loading) set.
	 * @param i Index of the sprite group to get.
	 * @return The requested sprite group.
	 */
	const SpriteGroup *get_group (bool alt, uint i) const
	{
		if (alt) i += this->n1;
		return this->groups[i];
	}

	/**
	 * Get the first available sprite group from the first set.
	 * @return The first sprite group, or NULL if there isn't any.
	 */
	const SpriteGroup *get_first (void) const
	{
		return (this->n1 != 0) ? this->groups[0] : NULL;
	}

	/**
	 * Get the first available sprite group from either set.
	 * @param alt Try the second (loading) set of groups first.
	 * @return The first sprite group, or NULL if there isn't any.
	 */
	const SpriteGroup *get_first (bool alt) const
	{
		return (alt && (this->n2 != 0)) ? this->groups[this->n1] :
				(this->n != 0)  ? this->groups[0] : NULL;
	}
};

/* Shared by deterministic and random groups. */
enum VarSpriteGroupScope {
	VSG_BEGIN,

	VSG_SCOPE_SELF = VSG_BEGIN, ///< Resolved object itself
	VSG_SCOPE_PARENT,           ///< Related object of the resolved one
	VSG_SCOPE_RELATIVE,         ///< Relative position (vehicles only)

	VSG_END
};
DECLARE_POSTFIX_INCREMENT(VarSpriteGroupScope)

enum DeterministicSpriteGroupAdjustType {
	DSGA_TYPE_NONE,
	DSGA_TYPE_DIV,
	DSGA_TYPE_MOD,
};

enum DeterministicSpriteGroupAdjustOperation {
	DSGA_OP_ADD,  ///< a + b
	DSGA_OP_SUB,  ///< a - b
	DSGA_OP_SMIN, ///< (signed) min(a, b)
	DSGA_OP_SMAX, ///< (signed) max(a, b)
	DSGA_OP_UMIN, ///< (unsigned) min(a, b)
	DSGA_OP_UMAX, ///< (unsigned) max(a, b)
	DSGA_OP_SDIV, ///< (signed) a / b
	DSGA_OP_SMOD, ///< (signed) a % b
	DSGA_OP_UDIV, ///< (unsigned) a / b
	DSGA_OP_UMOD, ///< (unsigned) a & b
	DSGA_OP_MUL,  ///< a * b
	DSGA_OP_AND,  ///< a & b
	DSGA_OP_OR,   ///< a | b
	DSGA_OP_XOR,  ///< a ^ b
	DSGA_OP_STO,  ///< store a into temporary storage, indexed by b. return a
	DSGA_OP_RST,  ///< return b
	DSGA_OP_STOP, ///< store a into persistent storage, indexed by b, return a
	DSGA_OP_ROR,  ///< rotate a b positions to the right
	DSGA_OP_SCMP, ///< (signed) comparison (a < b -> 0, a == b = 1, a > b = 2)
	DSGA_OP_UCMP, ///< (unsigned) comparison (a < b -> 0, a == b = 1, a > b = 2)
	DSGA_OP_SHL,  ///< a << b
	DSGA_OP_SHR,  ///< (unsigned) a >> b
	DSGA_OP_SAR,  ///< (signed) a >> b
};


struct DeterministicSpriteGroupAdjust {
	DeterministicSpriteGroupAdjustOperation operation;
	DeterministicSpriteGroupAdjustType type;
	byte variable;
	byte parameter; ///< Used for variables between 0x60 and 0x7F inclusive.
	byte shift_num;
	uint32 and_mask;
	uint32 add_val;
	uint32 divmod_val;
	const SpriteGroup *subroutine;
};


struct DeterministicSpriteGroupRange {
	const SpriteGroup *group;
	uint32 low;
	uint32 high;
};


struct DeterministicSpriteGroup : SpriteGroup, FlexArrayBase {
private:
	template <typename T>
	static CONSTEXPR T *offset_pointer (void *ptr, size_t offset)
	{
		return (T*) (((char*)ptr) + offset);
	}

	const VarSpriteGroupScope var_scope;    ///< Scope.
	const byte size;                        ///< Logarithmic size of accumulator (0 for int8, 1 for int16, 2 for int32).
	const SpriteGroup *default_group;       ///< Default result group.
	DeterministicSpriteGroupAdjust *const adjusts;  ///< Vector of adjusts.
	DeterministicSpriteGroupRange *const ranges;    ///< Vector of result ranges.
	const uint num_adjusts;                 ///< Adjust count.
	const byte num_ranges;                  ///< Result range count.

	static CONSTEXPR size_t adjusts_offset (void)
	{
		return ttd_align_up<DeterministicSpriteGroupAdjust> (sizeof(DeterministicSpriteGroup));
	}

	DeterministicSpriteGroup (bool parent_scope,
			byte size, uint num_adjusts,
			byte num_ranges, size_t ranges_offset,
			const DeterministicSpriteGroupAdjust *adjusts)
		: SpriteGroup (SGT_DETERMINISTIC),
		  var_scope (parent_scope ? VSG_SCOPE_PARENT : VSG_SCOPE_SELF),
		  size (size), default_group (NULL),
		  adjusts (offset_pointer<DeterministicSpriteGroupAdjust> (this, adjusts_offset())),
		  ranges  (offset_pointer<DeterministicSpriteGroupRange>  (this, ranges_offset)),
		  num_adjusts (num_adjusts), num_ranges (num_ranges)
	{
		memcpy (this->adjusts, adjusts, num_adjusts * sizeof(DeterministicSpriteGroupAdjust));
		memset (this->ranges, 0, num_ranges * sizeof(DeterministicSpriteGroupRange));
	}

	/** Custom operator new to account for the extra storage. */
	void *operator new (size_t size, size_t total, size_t = 1)
	{
		assert (total >= size);
		return ::operator new (total);
	}

protected:
	const SpriteGroup *Resolve(ResolverObject &object) const;

public:
	static DeterministicSpriteGroup *create (bool parent_scope, byte size,
		uint num_adjusts, byte num_ranges,
		const DeterministicSpriteGroupAdjust *adjusts)
	{
		size_t adjusts_end = adjusts_offset() + num_adjusts * sizeof(DeterministicSpriteGroupAdjust);
		size_t ranges_offset = ttd_align_up<DeterministicSpriteGroupRange> (adjusts_end);
		size_t ranges_end = ranges_offset + num_ranges * sizeof(DeterministicSpriteGroupRange);
		size_t total_size = ttd_align_up<DeterministicSpriteGroup> (ranges_end);
		DeterministicSpriteGroup *group = new (total_size)
				DeterministicSpriteGroup (parent_scope, size,
						num_adjusts, num_ranges,
						ranges_offset, adjusts);
		return SpriteGroup::append (group);
	}

	void set_range (byte i, const SpriteGroup *group, uint32 low, uint32 high)
	{
		DeterministicSpriteGroupRange *range = &this->ranges[i];
		range->group = group;
		range->low   = low;
		range->high  = high;
	}

	void set_default (const SpriteGroup *group)
	{
		this->default_group = group;
	}
};

enum RandomizedSpriteGroupCompareMode {
	RSG_CMP_ANY,
	RSG_CMP_ALL,
};

struct RandomizedSpriteGroup : ZeroedMemoryAllocator, SpriteGroup {
	RandomizedSpriteGroup() : SpriteGroup(SGT_RANDOMIZED) {}
	~RandomizedSpriteGroup();

	VarSpriteGroupScope var_scope;  ///< Take this object:

	RandomizedSpriteGroupCompareMode cmp_mode; ///< Check for these triggers:
	byte triggers;
	byte count;

	byte lowest_randbit; ///< Look for this in the per-object randomized bitmask:
	byte num_groups; ///< must be power of 2

	const SpriteGroup **groups; ///< Take the group with appropriate index:

	static RandomizedSpriteGroup *create (void)
	{
		return SpriteGroup::append (new RandomizedSpriteGroup);
	}

protected:
	const SpriteGroup *Resolve(ResolverObject &object) const;
};


/* This contains a callback result. A failed callback has a value of
 * CALLBACK_FAILED */
struct CallbackResultSpriteGroup : ZeroedMemoryAllocator, SpriteGroup {
	/**
	 * Creates a spritegroup representing a callback result
	 * @param value The value that was used to represent this callback result
	 * @param grf_version8 True, if we are dealing with a new NewGRF which uses GRF version >= 8.
	 */
	CallbackResultSpriteGroup(uint16 value, bool grf_version8) :
		SpriteGroup(SGT_CALLBACK),
		result(value)
	{
		/* Old style callback results (only valid for version < 8) have the highest byte 0xFF so signify it is a callback result.
		 * New style ones only have the highest bit set (allows 15-bit results, instead of just 8) */
		if (!grf_version8 && (this->result >> 8) == 0xFF) {
			this->result &= ~0xFF00;
		} else {
			this->result &= ~0x8000;
		}
	}

	uint16 result;
	uint16 GetCallbackResult() const { return this->result; }

	static CallbackResultSpriteGroup *create (uint16 value, bool v8)
	{
		return SpriteGroup::append (new CallbackResultSpriteGroup (value, v8));
	}
};


/* A result sprite group returns the first SpriteID and the number of
 * sprites in the set */
struct ResultSpriteGroup : SpriteGroup {
	/**
	 * Creates a spritegroup representing a sprite number result.
	 * @param sprite The sprite number.
	 * @param num_sprites The number of sprites per set.
	 * @return A spritegroup representing the sprite number result.
	 */
	ResultSpriteGroup(SpriteID sprite, byte num_sprites) :
		SpriteGroup(SGT_RESULT),
		sprite(sprite),
		num_sprites(num_sprites)
	{
	}

	SpriteID sprite;
	byte num_sprites;
	SpriteID GetResult() const { return this->sprite; }
	byte GetNumResults() const { return this->num_sprites; }

	static ResultSpriteGroup *create (SpriteID sprite, byte num_sprites)
	{
		return SpriteGroup::append (new ResultSpriteGroup (sprite, num_sprites));
	}
};

/**
 * Action 2 sprite layout for houses, industry tiles, objects and airport tiles.
 */
struct TileLayoutSpriteGroup : ZeroedMemoryAllocator, SpriteGroup {
	TileLayoutSpriteGroup() : SpriteGroup(SGT_TILELAYOUT) {}
	~TileLayoutSpriteGroup() {}

	NewGRFSpriteLayout dts;

	const DrawTileSprites *ProcessRegisters(uint8 *stage) const;

	static TileLayoutSpriteGroup *create (void)
	{
		return SpriteGroup::append (new TileLayoutSpriteGroup);
	}
};

struct IndustryProductionSpriteGroup : ZeroedMemoryAllocator, SpriteGroup {
	IndustryProductionSpriteGroup() : SpriteGroup(SGT_INDUSTRY_PRODUCTION) {}

	uint8 version;
	int16 subtract_input[3];  // signed
	uint16 add_output[2];     // unsigned
	uint8 again;

	static IndustryProductionSpriteGroup *create (void)
	{
		return SpriteGroup::append (new IndustryProductionSpriteGroup);
	}
};

/**
 * Interface to query and set values specific to a single #VarSpriteGroupScope (action 2 scope).
 *
 * Multiple of these interfaces are combined into a #ResolverObject to allow access
 * to different game entities from a #SpriteGroup-chain (action 1-2-3 chain).
 */
struct ScopeResolver {
	virtual ~ScopeResolver()
	{
	}

	virtual uint32 GetRandomBits() const;
	virtual uint32 GetTriggers() const;
	virtual void SetTriggers(int triggers) const;

	virtual uint32 GetVariable(byte variable, uint32 parameter, bool *available) const;
	virtual void StorePSA(uint reg, int32 value);
};

/**
 * Interface for #SpriteGroup-s to access the gamestate.
 *
 * Using this interface #SpriteGroup-chains (action 1-2-3 chains) can be resolved,
 * to get the results of callbacks, rerandomisations or normal sprite lookups.
 */
struct ResolverObject {
	const GRFFile *const grffile;   ///< GRFFile the resolved SpriteGroup belongs to

	ResolverObject(const GRFFile *grffile, CallbackID callback = CBID_NO_CALLBACK, uint32 callback_param1 = 0, uint32 callback_param2 = 0);
	virtual ~ResolverObject();

	ScopeResolver default_scope; ///< Default implementation of the grf scope.

	CallbackID callback;        ///< Callback being resolved.
	uint32 callback_param1;     ///< First parameter (var 10) of the callback.
	uint32 callback_param2;     ///< Second parameter (var 18) of the callback.

	byte trigger;

	uint32 last_value;          ///< Result of most recent DeterministicSpriteGroup (including procedure calls)
	uint32 reseed[VSG_END];     ///< Collects bits to rerandomise while triggering triggers.

	virtual const SpriteGroup *ResolveReal(const RealSpriteGroup *group) const;

	virtual ScopeResolver *GetScope(VarSpriteGroupScope scope = VSG_SCOPE_SELF, byte relative = 0);

	/**
	 * Returns the OR-sum of all bits that need reseeding
	 * independent of the scope they were accessed with.
	 * @return OR-sum of the bits.
	 */
	uint32 GetReseedSum() const
	{
		uint32 sum = 0;
		for (VarSpriteGroupScope vsg = VSG_BEGIN; vsg < VSG_END; vsg++) {
			sum |= this->reseed[vsg];
		}
		return sum;
	}

	/**
	 * Resets the dynamic state of the resolver object.
	 * To be called before resolving an Action-1-2-3 chain.
	 */
	void ResetState()
	{
		this->last_value = 0;
		this->trigger    = 0;
		memset(this->reseed, 0, sizeof(this->reseed));
	}
};

#endif /* NEWGRF_SPRITEGROUP_H */
