/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file station_gui.cpp The GUI for stations. */

#include "stdafx.h"
#include "debug.h"
#include "core/pointer.h"
#include "gui.h"
#include "textbuf_gui.h"
#include "company_func.h"
#include "command_func.h"
#include "vehicle_gui.h"
#include "cargotype.h"
#include "station_gui.h"
#include "strings_func.h"
#include "string.h"
#include "window_func.h"
#include "viewport_func.h"
#include "widgets/dropdown_func.h"
#include "station_base.h"
#include "waypoint_base.h"
#include "tilehighlight_func.h"
#include "company_base.h"
#include "sortlist_type.h"
#include "core/geometry_func.hpp"
#include "vehiclelist.h"
#include "town.h"
#include "linkgraph/linkgraph.h"
#include "station_func.h"
#include "zoom_func.h"

#include "widgets/station_widget.h"

#include "table/strings.h"

#include <utility>
#include <bitset>
#include <set>
#include <map>
#include <vector>
#include <algorithm>

/**
 * Calculates and draws the accepted and supplied cargo around the selected tile(s)
 * @param dpi area to draw on
 * @param left x position where the string is to be drawn
 * @param right the right most position to draw on
 * @param top y position where the string is to be drawn
 * @param rad radius around selected tile(s) to be searched
 * @param sct which type of cargo is to be displayed (passengers/non-passengers)
 * @return Returns the y value below the strings that were drawn
 */
int DrawStationCoverageAreaText (BlitArea *dpi, int left, int right, int top,
	int rad, StationCoverageType sct)
{
	uint32 accept_mask = 0;
	uint32 supply_mask = 0;

	if (_thd.drawstyle == HT_RECT) {
		TileIndex tile = TileVirtXY (_thd.pos.x, _thd.pos.y);
		if (tile < MapSize()) {
			TileArea ta (tile, _thd.size.x / TILE_SIZE, _thd.size.y / TILE_SIZE);
			CargoArray accept_cargoes = GetAreaAcceptance (ta, rad);
			CargoArray supply_cargoes = GetAreaProduction (ta, rad);

			for (CargoID i = 0; i < NUM_CARGO; i++) {
				switch (sct) {
					case SCT_PASSENGERS_ONLY:
						if (!IsCargoInClass (i, CC_PASSENGERS)) continue;
						break;
					case SCT_NON_PASSENGERS_ONLY:
						if (IsCargoInClass (i, CC_PASSENGERS)) continue;
						break;
					case SCT_ALL: break;
					default: NOT_REACHED();
				}
				if (accept_cargoes[i] >= 8) SetBit(accept_mask, i);
				if (supply_cargoes[i] >= 1) SetBit(supply_mask, i);
			}
		}
	}

	SetDParam (0, accept_mask);
	top = DrawStringMultiLine (dpi, left, right, top, INT32_MAX, STR_STATION_BUILD_ACCEPTS_CARGO)  + WD_PAR_VSEP_NORMAL;
	SetDParam (0, supply_mask);
	top = DrawStringMultiLine (dpi, left, right, top, INT32_MAX, STR_STATION_BUILD_SUPPLIES_CARGO) + WD_PAR_VSEP_NORMAL;
	return top;
}

/**
 * Check whether we need to redraw the station coverage text.
 * If it is needed actually make the window for redrawing.
 * @param w the window to check.
 */
void CheckRedrawStationCoverage(const Window *w)
{
	if (_thd.dirty & 1) {
		_thd.dirty &= ~1;
		w->SetDirty();
	}
}

/**
 * Draw small boxes of cargo amount and ratings data at the given
 * coordinates. If amount exceeds 576 units, it is shown 'full', same
 * goes for the rating: at above 90% orso (224) it is also 'full'
 *
 * @param dpi    area to draw on
 * @param left   left most coordinate to draw the box at
 * @param right  right most coordinate to draw the box at
 * @param y      coordinate to draw the box at
 * @param type   Cargo type
 * @param amount Cargo amount
 * @param rating ratings data for that particular cargo
 *
 * @note Each cargo-bar is 16 pixels wide and 6 pixels high
 * @note Each rating 14 pixels wide and 1 pixel high and is 1 pixel below the cargo-bar
 */
static void StationsWndShowStationRating (BlitArea *dpi,
	int left, int right, int y, CargoID type, uint amount, byte rating)
{
	static const uint units_full  = 576; ///< number of units to show station as 'full'
	static const uint rating_full = 224; ///< rating needed so it is shown as 'full'

	const CargoSpec *cs = CargoSpec::Get(type);
	if (!cs->IsValid()) return;

	int colour = cs->rating_colour;
	TextColour tc = GetContrastColour(colour);
	uint w = (minu(amount, units_full) + 5) / 36;

	int height = GetCharacterHeight(FS_SMALL);

	/* Draw total cargo (limited) on station (fits into 16 pixels) */
	if (w != 0) GfxFillRect (dpi, left, y, left + w - 1, y + height, colour);

	/* Draw a one pixel-wide bar of additional cargo meter, useful
	 * for stations with only a small amount (<=30) */
	if (w == 0) {
		uint rest = amount / 5;
		if (rest != 0) {
			w += left;
			GfxFillRect (dpi, w, y + height - rest, w, y + height, colour);
		}
	}

	DrawString (dpi, left + 1, right, y, cs->abbrev, tc);

	/* Draw green/red ratings bar (fits into 14 pixels) */
	y += height + 2;
	GfxFillRect (dpi, left + 1, y, left + 14, y, PC_RED);
	rating = minu(rating, rating_full) / 16;
	if (rating != 0) GfxFillRect (dpi, left + 1, y, left + rating, y, PC_GREEN);
}

typedef GUIList<const Station*> GUIStationList;

/**
 * The list of stations per company.
 */
class CompanyStationsWindow : public Window
{
protected:
	/* Runtime saved values */
	static Listing last_sorting;
	static byte facilities;               // types of stations of interest
	static bool include_empty;            // whether we should include stations without waiting cargo
	static const uint32 cargo_filter_max;
	static uint32 cargo_filter;           // bitmap of cargo types to include
	static const Station *last_station;

	/* Constants for sorting stations */
	static const StringID sorter_names[];
	static GUIStationList::SortFunction * const sorter_funcs[];

	GUIStationList stations;
	Scrollbar *vscroll;

	/**
	 * (Re)Build station list
	 *
	 * @param owner company whose stations are to be in list
	 */
	void BuildStationsList(const Owner owner)
	{
		if (!this->stations.NeedRebuild()) return;

		DEBUG(misc, 3, "Building station list for company %d", owner);

		this->stations.Clear();

		const Station *st;
		FOR_ALL_STATIONS(st) {
			if (st->owner == owner || (st->owner == OWNER_NONE && HasStationInUse(st->index, true, owner))) {
				if (this->facilities & st->facilities) { // only stations with selected facilities
					int num_waiting_cargo = 0;
					for (CargoID j = 0; j < NUM_CARGO; j++) {
						if (st->goods[j].HasRating()) {
							num_waiting_cargo++; // count number of waiting cargo
							if (HasBit(this->cargo_filter, j)) {
								*this->stations.Append() = st;
								break;
							}
						}
					}
					/* stations without waiting cargo */
					if (num_waiting_cargo == 0 && this->include_empty) {
						*this->stations.Append() = st;
					}
				}
			}
		}

		this->stations.Compact();
		this->stations.RebuildDone();

		this->vscroll->SetCount(this->stations.Length()); // Update the scrollbar
	}

	/** Sort stations by their name */
	static int CDECL StationNameSorter(const Station * const *a, const Station * const *b)
	{
		static char buf_cache[64];
		char buf[64];

		SetDParam(0, (*a)->index);
		GetString (buf, STR_STATION_NAME);

		if (*b != last_station) {
			last_station = *b;
			SetDParam(0, (*b)->index);
			GetString (buf_cache, STR_STATION_NAME);
		}

		int r = strnatcmp(buf, buf_cache); // Sort by name (natural sorting).
		if (r == 0) return (*a)->index - (*b)->index;
		return r;
	}

	/** Sort stations by their type */
	static int CDECL StationTypeSorter(const Station * const *a, const Station * const *b)
	{
		return (*a)->facilities - (*b)->facilities;
	}

	/** Sort stations by their waiting cargo */
	static int CDECL StationWaitingTotalSorter(const Station * const *a, const Station * const *b)
	{
		int diff = 0;

		CargoID j;
		FOR_EACH_SET_CARGO_ID(j, cargo_filter) {
			diff += (*a)->goods[j].cargo.TotalCount() - (*b)->goods[j].cargo.TotalCount();
		}

		return diff;
	}

	/** Sort stations by their available waiting cargo */
	static int CDECL StationWaitingAvailableSorter(const Station * const *a, const Station * const *b)
	{
		int diff = 0;

		CargoID j;
		FOR_EACH_SET_CARGO_ID(j, cargo_filter) {
			diff += (*a)->goods[j].cargo.AvailableCount() - (*b)->goods[j].cargo.AvailableCount();
		}

		return diff;
	}

	/** Sort stations by their rating */
	static int CDECL StationRatingMaxSorter(const Station * const *a, const Station * const *b)
	{
		byte maxr1 = 0;
		byte maxr2 = 0;

		CargoID j;
		FOR_EACH_SET_CARGO_ID(j, cargo_filter) {
			if ((*a)->goods[j].HasRating()) maxr1 = max(maxr1, (*a)->goods[j].rating);
			if ((*b)->goods[j].HasRating()) maxr2 = max(maxr2, (*b)->goods[j].rating);
		}

		return maxr1 - maxr2;
	}

	/** Sort stations by their rating */
	static int CDECL StationRatingMinSorter(const Station * const *a, const Station * const *b)
	{
		byte minr1 = 255;
		byte minr2 = 255;

		for (CargoID j = 0; j < NUM_CARGO; j++) {
			if (!HasBit(cargo_filter, j)) continue;
			if ((*a)->goods[j].HasRating()) minr1 = min(minr1, (*a)->goods[j].rating);
			if ((*b)->goods[j].HasRating()) minr2 = min(minr2, (*b)->goods[j].rating);
		}

		return -(minr1 - minr2);
	}

	/** Sort the stations list */
	void SortStationsList()
	{
		if (!this->stations.Sort()) return;

		/* Reset name sorter sort cache */
		this->last_station = NULL;

		/* Set the modified widget dirty */
		this->SetWidgetDirty(WID_STL_LIST);
	}

public:
	CompanyStationsWindow (const WindowDesc *desc, WindowNumber window_number) :
		Window (desc), stations(), vscroll (NULL)
	{
		this->stations.SetListing(this->last_sorting);
		this->stations.SetSortFuncs(this->sorter_funcs);
		this->stations.ForceRebuild();
		this->stations.NeedResort();
		this->SortStationsList();

		this->CreateNestedTree();
		this->vscroll = this->GetScrollbar(WID_STL_SCROLLBAR);
		this->InitNested(window_number);
		this->owner = (Owner)this->window_number;

		const CargoSpec *cs;
		FOR_ALL_SORTED_STANDARD_CARGOSPECS(cs) {
			if (!HasBit(this->cargo_filter, cs->Index())) continue;
			this->LowerWidget(WID_STL_CARGOSTART + index);
		}

		if (this->cargo_filter == this->cargo_filter_max) this->cargo_filter = _cargo_mask;

		for (uint i = 0; i < 5; i++) {
			if (HasBit(this->facilities, i)) this->LowerWidget(i + WID_STL_TRAIN);
		}
		this->SetWidgetLoweredState(WID_STL_NOCARGOWAITING, this->include_empty);

		this->GetWidget<NWidgetCore>(WID_STL_SORTDROPBTN)->widget_data = this->sorter_names[this->stations.SortType()];
	}

	void OnDelete (void) FINAL_OVERRIDE
	{
		this->last_sorting = this->stations.GetListing();
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case WID_STL_SORTBY: {
				Dimension d = GetStringBoundingBox(this->GetWidget<NWidgetCore>(widget)->widget_data);
				d.width += padding.width + Window::SortButtonWidth() * 2; // Doubled since the string is centred and it also looks better.
				d.height += padding.height;
				*size = maxdim(*size, d);
				break;
			}

			case WID_STL_SORTDROPBTN: {
				Dimension d = {0, 0};
				for (int i = 0; this->sorter_names[i] != INVALID_STRING_ID; i++) {
					d = maxdim(d, GetStringBoundingBox(this->sorter_names[i]));
				}
				d.width += padding.width;
				d.height += padding.height;
				*size = maxdim(*size, d);
				break;
			}

			case WID_STL_LIST:
				resize->height = FONT_HEIGHT_NORMAL;
				size->height = WD_FRAMERECT_TOP + 5 * resize->height + WD_FRAMERECT_BOTTOM;
				break;

			case WID_STL_TRAIN:
			case WID_STL_TRUCK:
			case WID_STL_BUS:
			case WID_STL_AIRPLANE:
			case WID_STL_SHIP:
				size->height = max<uint>(FONT_HEIGHT_SMALL, 10) + padding.height;
				break;

			case WID_STL_CARGOALL:
			case WID_STL_FACILALL:
			case WID_STL_NOCARGOWAITING: {
				Dimension d = GetStringBoundingBox(widget == WID_STL_NOCARGOWAITING ? STR_ABBREV_NONE : STR_ABBREV_ALL);
				d.width  += padding.width + 2;
				d.height += padding.height;
				*size = maxdim(*size, d);
				break;
			}

			default:
				if (widget >= WID_STL_CARGOSTART) {
					Dimension d = GetStringBoundingBox(_sorted_cargo_specs[widget - WID_STL_CARGOSTART]->abbrev);
					d.width  += padding.width + 2;
					d.height += padding.height;
					*size = maxdim(*size, d);
				}
				break;
		}
	}

	void OnPaint (BlitArea *dpi) OVERRIDE
	{
		this->BuildStationsList((Owner)this->window_number);
		this->SortStationsList();

		this->DrawWidgets (dpi);
	}

	void DrawWidget (BlitArea *dpi, const Rect &r, int widget) const OVERRIDE
	{
		switch (widget) {
			case WID_STL_SORTBY:
				/* draw arrow pointing up/down for ascending/descending sorting */
				this->DrawSortButtonState (dpi, WID_STL_SORTBY, this->stations.IsDescSortOrder() ? SBS_DOWN : SBS_UP);
				break;

			case WID_STL_LIST: {
				bool rtl = _current_text_dir == TD_RTL;
				int max = min(this->vscroll->GetPosition() + this->vscroll->GetCapacity(), this->stations.Length());
				int y = r.top + WD_FRAMERECT_TOP;
				for (int i = this->vscroll->GetPosition(); i < max; ++i) { // do until max number of stations of owner
					const Station *st = this->stations[i];
					assert(st->xy != INVALID_TILE);

					/* Do not do the complex check HasStationInUse here, it may be even false
					 * when the order had been removed and the station list hasn't been removed yet */
					assert(st->owner == owner || st->owner == OWNER_NONE);

					SetDParam(0, st->index);
					SetDParam(1, st->facilities);
					int x = DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, STR_STATION_LIST_STATION);
					x += rtl ? -5 : 5;

					/* show cargo waiting and station ratings */
					for (uint j = 0; j < _sorted_standard_cargo_specs_size; j++) {
						CargoID cid = _sorted_cargo_specs[j]->Index();
						if (st->goods[cid].cargo.TotalCount() > 0) {
							/* For RTL we work in exactly the opposite direction. So
							 * decrement the space needed first, then draw to the left
							 * instead of drawing to the left and then incrementing
							 * the space. */
							if (rtl) {
								x -= 20;
								if (x < r.left + WD_FRAMERECT_LEFT) break;
							}
							StationsWndShowStationRating (dpi, x, x + 16, y, cid, st->goods[cid].cargo.TotalCount(), st->goods[cid].rating);
							if (!rtl) {
								x += 20;
								if (x > r.right - WD_FRAMERECT_RIGHT) break;
							}
						}
					}
					y += FONT_HEIGHT_NORMAL;
				}

				if (this->vscroll->GetCount() == 0) { // company has no stations
					DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, STR_STATION_LIST_NONE);
					return;
				}
				break;
			}

			case WID_STL_NOCARGOWAITING:
				DrawString (dpi, r.left + 1, r.right + 1, r.top + 1, STR_ABBREV_NONE, TC_BLACK, SA_HOR_CENTER);
				break;

			case WID_STL_CARGOALL:
				DrawString (dpi, r.left + 1, r.right + 1, r.top + 1, STR_ABBREV_ALL, TC_BLACK, SA_HOR_CENTER);
				break;

			case WID_STL_FACILALL:
				DrawString (dpi, r.left + 1, r.right + 1, r.top + 1, STR_ABBREV_ALL, TC_BLACK, SA_HOR_CENTER);
				break;

			default:
				if (widget >= WID_STL_CARGOSTART) {
					const CargoSpec *cs = _sorted_cargo_specs[widget - WID_STL_CARGOSTART];
					GfxFillRect (dpi, r.left + 1, r.top + 1, r.right - 1, r.bottom - 1, cs->rating_colour);
					TextColour tc = GetContrastColour(cs->rating_colour);
					DrawString (dpi, r.left + 1, r.right + 1, r.top + 1, cs->abbrev, tc, SA_HOR_CENTER);
				}
				break;
		}
	}

	virtual void SetStringParameters(int widget) const
	{
		if (widget == WID_STL_CAPTION) {
			SetDParam(0, this->window_number);
			SetDParam(1, this->vscroll->GetCount());
		}
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_STL_LIST: {
				uint id_v = this->vscroll->GetScrolledRowFromWidget(pt.y, this, WID_STL_LIST, 0, FONT_HEIGHT_NORMAL);
				if (id_v >= this->stations.Length()) return; // click out of list bound

				const Station *st = this->stations[id_v];
				/* do not check HasStationInUse - it is slow and may be invalid */
				assert(st->owner == (Owner)this->window_number || st->owner == OWNER_NONE);

				if (_ctrl_pressed) {
					ShowExtraViewPortWindow(st->xy);
				} else {
					ScrollMainWindowToTile(st->xy);
				}
				break;
			}

			case WID_STL_TRAIN:
			case WID_STL_TRUCK:
			case WID_STL_BUS:
			case WID_STL_AIRPLANE:
			case WID_STL_SHIP:
				if (_ctrl_pressed) {
					ToggleBit(this->facilities, widget - WID_STL_TRAIN);
					this->ToggleWidgetLoweredState(widget);
				} else {
					uint i;
					FOR_EACH_SET_BIT(i, this->facilities) {
						this->RaiseWidget(i + WID_STL_TRAIN);
					}
					this->facilities = 1 << (widget - WID_STL_TRAIN);
					this->LowerWidget(widget);
				}
				this->stations.ForceRebuild();
				this->SetDirty();
				break;

			case WID_STL_FACILALL:
				for (uint i = WID_STL_TRAIN; i <= WID_STL_SHIP; i++) {
					this->LowerWidget(i);
				}

				this->facilities = FACIL_TRAIN | FACIL_TRUCK_STOP | FACIL_BUS_STOP | FACIL_AIRPORT | FACIL_DOCK;
				this->stations.ForceRebuild();
				this->SetDirty();
				break;

			case WID_STL_CARGOALL: {
				for (uint i = 0; i < _sorted_standard_cargo_specs_size; i++) {
					this->LowerWidget(WID_STL_CARGOSTART + i);
				}
				this->LowerWidget(WID_STL_NOCARGOWAITING);

				this->cargo_filter = _cargo_mask;
				this->include_empty = true;
				this->stations.ForceRebuild();
				this->SetDirty();
				break;
			}

			case WID_STL_SORTBY: // flip sorting method asc/desc
				this->stations.ToggleSortOrder();
				this->SetDirty();
				break;

			case WID_STL_SORTDROPBTN: // select sorting criteria dropdown menu
				ShowDropDownMenu(this, this->sorter_names, this->stations.SortType(), WID_STL_SORTDROPBTN, 0, 0);
				break;

			case WID_STL_NOCARGOWAITING:
				if (_ctrl_pressed) {
					this->include_empty = !this->include_empty;
					this->ToggleWidgetLoweredState(WID_STL_NOCARGOWAITING);
				} else {
					for (uint i = 0; i < _sorted_standard_cargo_specs_size; i++) {
						this->RaiseWidget(WID_STL_CARGOSTART + i);
					}

					this->cargo_filter = 0;
					this->include_empty = true;

					this->LowerWidget(WID_STL_NOCARGOWAITING);
				}
				this->stations.ForceRebuild();
				this->SetDirty();
				break;

			default:
				if (widget >= WID_STL_CARGOSTART) { // change cargo_filter
					/* Determine the selected cargo type */
					const CargoSpec *cs = _sorted_cargo_specs[widget - WID_STL_CARGOSTART];

					if (_ctrl_pressed) {
						ToggleBit(this->cargo_filter, cs->Index());
						this->ToggleWidgetLoweredState(widget);
					} else {
						for (uint i = 0; i < _sorted_standard_cargo_specs_size; i++) {
							this->RaiseWidget(WID_STL_CARGOSTART + i);
						}
						this->RaiseWidget(WID_STL_NOCARGOWAITING);

						this->cargo_filter = 0;
						this->include_empty = false;

						SetBit(this->cargo_filter, cs->Index());
						this->LowerWidget(widget);
					}
					this->stations.ForceRebuild();
					this->SetDirty();
				}
				break;
		}
	}

	virtual void OnDropdownSelect(int widget, int index)
	{
		if (this->stations.SortType() != index) {
			this->stations.SetSortType(index);

			/* Display the current sort variant */
			this->GetWidget<NWidgetCore>(WID_STL_SORTDROPBTN)->widget_data = this->sorter_names[this->stations.SortType()];

			this->SetDirty();
		}
	}

	virtual void OnTick()
	{
		if (_pause_mode != PM_UNPAUSED) return;
		if (this->stations.NeedResort()) {
			DEBUG(misc, 3, "Periodic rebuild station list company %d", this->window_number);
			this->SetDirty();
		}
	}

	virtual void OnResize()
	{
		this->vscroll->SetCapacityFromWidget(this, WID_STL_LIST, WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM);
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (data == 0) {
			/* This needs to be done in command-scope to enforce rebuilding before resorting invalid data */
			this->stations.ForceRebuild();
		} else {
			this->stations.ForceResort();
		}
	}
};

Listing CompanyStationsWindow::last_sorting = {false, 0};
byte CompanyStationsWindow::facilities = FACIL_TRAIN | FACIL_TRUCK_STOP | FACIL_BUS_STOP | FACIL_AIRPORT | FACIL_DOCK;
bool CompanyStationsWindow::include_empty = true;
const uint32 CompanyStationsWindow::cargo_filter_max = UINT32_MAX;
uint32 CompanyStationsWindow::cargo_filter = UINT32_MAX;
const Station *CompanyStationsWindow::last_station = NULL;

/* Availible station sorting functions */
GUIStationList::SortFunction * const CompanyStationsWindow::sorter_funcs[] = {
	&StationNameSorter,
	&StationTypeSorter,
	&StationWaitingTotalSorter,
	&StationWaitingAvailableSorter,
	&StationRatingMaxSorter,
	&StationRatingMinSorter
};

/* Names of the sorting functions */
const StringID CompanyStationsWindow::sorter_names[] = {
	STR_SORT_BY_NAME,
	STR_SORT_BY_FACILITY,
	STR_SORT_BY_WAITING_TOTAL,
	STR_SORT_BY_WAITING_AVAILABLE,
	STR_SORT_BY_RATING_MAX,
	STR_SORT_BY_RATING_MIN,
	INVALID_STRING_ID
};

/**
 * Make a horizontal row of cargo buttons, starting at widget #WID_STL_CARGOSTART.
 * @param biggest_index Pointer to store biggest used widget number of the buttons.
 * @return Horizontal row.
 */
static NWidgetBase *CargoWidgets(int *biggest_index)
{
	NWidgetHorizontal *container = new NWidgetHorizontal();

	for (uint i = 0; i < _sorted_standard_cargo_specs_size; i++) {
		NWidgetBackground *panel = new NWidgetBackground(WWT_PANEL, COLOUR_GREY, WID_STL_CARGOSTART + i);
		panel->SetMinimalSize(14, 11);
		panel->SetResize(0, 0);
		panel->SetFill(0, 1);
		panel->SetDataTip(0, STR_STATION_LIST_USE_CTRL_TO_SELECT_MORE);
		container->Add(panel);
	}
	*biggest_index = WID_STL_CARGOSTART + _sorted_standard_cargo_specs_size;
	return container;
}

static const NWidgetPart _nested_company_stations_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, WID_STL_CAPTION), SetDataTip(STR_STATION_LIST_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_STL_TRAIN), SetMinimalSize(14, 11), SetDataTip(STR_TRAIN, STR_STATION_LIST_USE_CTRL_TO_SELECT_MORE), SetFill(0, 1),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_STL_TRUCK), SetMinimalSize(14, 11), SetDataTip(STR_LORRY, STR_STATION_LIST_USE_CTRL_TO_SELECT_MORE), SetFill(0, 1),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_STL_BUS), SetMinimalSize(14, 11), SetDataTip(STR_BUS, STR_STATION_LIST_USE_CTRL_TO_SELECT_MORE), SetFill(0, 1),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_STL_SHIP), SetMinimalSize(14, 11), SetDataTip(STR_SHIP, STR_STATION_LIST_USE_CTRL_TO_SELECT_MORE), SetFill(0, 1),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_STL_AIRPLANE), SetMinimalSize(14, 11), SetDataTip(STR_PLANE, STR_STATION_LIST_USE_CTRL_TO_SELECT_MORE), SetFill(0, 1),
		NWidget(WWT_PUSHBTN, COLOUR_GREY, WID_STL_FACILALL), SetMinimalSize(14, 11), SetDataTip(0x0, STR_STATION_LIST_SELECT_ALL_FACILITIES), SetFill(0, 1),
		NWidget(WWT_PANEL, COLOUR_GREY), SetMinimalSize(5, 11), SetFill(0, 1), EndContainer(),
		NWidgetFunction(CargoWidgets),
		NWidget(WWT_PANEL, COLOUR_GREY, WID_STL_NOCARGOWAITING), SetMinimalSize(14, 11), SetDataTip(0x0, STR_STATION_LIST_NO_WAITING_CARGO), SetFill(0, 1), EndContainer(),
		NWidget(WWT_PUSHBTN, COLOUR_GREY, WID_STL_CARGOALL), SetMinimalSize(14, 11), SetDataTip(0x0, STR_STATION_LIST_SELECT_ALL_TYPES), SetFill(0, 1),
		NWidget(WWT_PANEL, COLOUR_GREY), SetDataTip(0x0, STR_NULL), SetResize(1, 0), SetFill(1, 1), EndContainer(),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_STL_SORTBY), SetMinimalSize(81, 12), SetDataTip(STR_BUTTON_SORT_BY, STR_TOOLTIP_SORT_ORDER),
		NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_STL_SORTDROPBTN), SetMinimalSize(163, 12), SetDataTip(STR_SORT_BY_NAME, STR_TOOLTIP_SORT_CRITERIA), // widget_data gets overwritten.
		NWidget(WWT_PANEL, COLOUR_GREY), SetDataTip(0x0, STR_NULL), SetResize(1, 0), SetFill(1, 1), EndContainer(),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, COLOUR_GREY, WID_STL_LIST), SetMinimalSize(346, 125), SetResize(1, 10), SetDataTip(0x0, STR_STATION_LIST_TOOLTIP), SetScrollbar(WID_STL_SCROLLBAR), EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_STL_SCROLLBAR),
			NWidget(WWT_RESIZEBOX, COLOUR_GREY),
		EndContainer(),
	EndContainer(),
};

static WindowDesc::Prefs _company_stations_prefs ("list_stations");

static const WindowDesc _company_stations_desc(
	WDP_AUTO, 358, 162,
	WC_STATION_LIST, WC_NONE,
	0,
	_nested_company_stations_widgets, lengthof(_nested_company_stations_widgets),
	&_company_stations_prefs
);

/**
 * Opens window with list of company's stations
 *
 * @param company whose stations' list show
 */
void ShowCompanyStations(CompanyID company)
{
	if (!Company::IsValidID(company)) return;

	AllocateWindowDescFront<CompanyStationsWindow>(&_company_stations_desc, company);
}

static const NWidgetPart _nested_station_view_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, WID_SV_CAPTION), SetDataTip(STR_STATION_VIEW_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_SV_SORT_ORDER), SetMinimalSize(81, 12), SetFill(1, 1), SetDataTip(STR_BUTTON_SORT_BY, STR_TOOLTIP_SORT_ORDER),
		NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_SV_SORT_BY), SetMinimalSize(168, 12), SetResize(1, 0), SetFill(0, 1), SetDataTip(0x0, STR_TOOLTIP_SORT_CRITERIA),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_SV_GROUP), SetMinimalSize(81, 12), SetFill(1, 1), SetDataTip(STR_STATION_VIEW_GROUP, 0x0),
		NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_SV_GROUP_BY), SetMinimalSize(168, 12), SetResize(1, 0), SetFill(0, 1), SetDataTip(0x0, STR_TOOLTIP_GROUP_ORDER),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, COLOUR_GREY, WID_SV_WAITING), SetMinimalSize(237, 44), SetResize(1, 10), SetScrollbar(WID_SV_SCROLLBAR), EndContainer(),
		NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_SV_SCROLLBAR),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_GREY, WID_SV_ACCEPT_RATING_LIST), SetMinimalSize(249, 23), SetResize(1, 0), EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_HORIZONTAL, NC_EQUALSIZE),
			NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_SV_LOCATION), SetMinimalSize(45, 12), SetResize(1, 0), SetFill(1, 1),
					SetDataTip(STR_BUTTON_LOCATION, STR_STATION_VIEW_CENTER_TOOLTIP),
			NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_SV_ACCEPTS_RATINGS), SetMinimalSize(46, 12), SetResize(1, 0), SetFill(1, 1),
					SetDataTip(STR_STATION_VIEW_RATINGS_BUTTON, STR_STATION_VIEW_RATINGS_TOOLTIP),
			NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_SV_RENAME), SetMinimalSize(45, 12), SetResize(1, 0), SetFill(1, 1),
					SetDataTip(STR_BUTTON_RENAME, STR_STATION_VIEW_RENAME_TOOLTIP),
		EndContainer(),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_SV_CLOSE_AIRPORT), SetMinimalSize(45, 12), SetResize(1, 0), SetFill(1, 1),
				SetDataTip(STR_STATION_VIEW_CLOSE_AIRPORT, STR_STATION_VIEW_CLOSE_AIRPORT_TOOLTIP),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_SV_TRAINS), SetMinimalSize(14, 12), SetFill(0, 1), SetDataTip(STR_TRAIN, STR_STATION_VIEW_SCHEDULED_TRAINS_TOOLTIP),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_SV_ROADVEHS), SetMinimalSize(14, 12), SetFill(0, 1), SetDataTip(STR_LORRY, STR_STATION_VIEW_SCHEDULED_ROAD_VEHICLES_TOOLTIP),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_SV_SHIPS), SetMinimalSize(14, 12), SetFill(0, 1), SetDataTip(STR_SHIP, STR_STATION_VIEW_SCHEDULED_SHIPS_TOOLTIP),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_SV_PLANES),  SetMinimalSize(14, 12), SetFill(0, 1), SetDataTip(STR_PLANE, STR_STATION_VIEW_SCHEDULED_AIRCRAFT_TOOLTIP),
		NWidget(WWT_RESIZEBOX, COLOUR_GREY),
	EndContainer(),
};

/**
 * Draws icons of waiting cargo in the StationView window
 *
 * @param i type of cargo
 * @param waiting number of waiting units
 * @param dpi   area to draw on
 * @param left  left most coordinate to draw on
 * @param right right most coordinate to draw on
 * @param y y coordinate
 * @param width the width of the view
 */
static void DrawCargoIcons (CargoID i, uint waiting, BlitArea *dpi,
	int left, int right, int y)
{
	int width = ScaleGUITrad(10);
	uint num = min((waiting + (width / 2)) / width, (right - left) / width); // maximum is width / 10 icons so it won't overflow
	if (num == 0) return;

	SpriteID sprite = CargoSpec::Get(i)->GetCargoIcon();

	int x = _current_text_dir == TD_RTL ? left : right - num * width;
	do {
		DrawSprite (dpi, sprite, PAL_NONE, x, y);
		x += width;
	} while (--num);
}

enum SortOrder {
	SO_DESCENDING,
	SO_ASCENDING
};

enum CargoSortType {
	ST_COUNT,          ///< by amount of cargo
	ST_STATION_STRING, ///< by station name
	ST_STATION_ID,     ///< by station id
};


/** A node in the tree of cached destinations for a cargo type in a station. */
struct CargoDestNode {
	typedef std::map <StationID, CargoDestNode> map;
	typedef map::const_iterator iterator;

	CargoDestNode *const parent; ///< the parent of this entry
	uint count;                  ///< amount of cargo for this node and children
	map children;                ///< children of this node

	CargoDestNode (CargoDestNode *p = NULL)
		: parent(p), count(0), children()
	{
	}

	void clear (void)
	{
		this->children.clear();
	}

	iterator begin (void) const
	{
		return this->children.begin();
	}

	iterator end (void) const
	{
		return this->children.end();
	}

	const CargoDestNode *find (StationID id) const
	{
		iterator iter (this->children.find (id));
		return (iter != end()) ? &iter->second : NULL;
	}

	CargoDestNode *insert (StationID id);

	void update (uint count);

	void estimate (CargoID cargo, StationID source, StationID next,
		uint count);
};

/** Find or insert a child node of the current node. */
CargoDestNode *CargoDestNode::insert (StationID id)
{
	const std::pair <map::iterator, map::iterator> range
		(this->children.equal_range (id));

	if (range.first != range.second) return &range.first->second;

	map::iterator iter (this->children.insert (range.first,
		std::make_pair (id, CargoDestNode (this))));

	return &iter->second;
}

/**
 * Update the count for this node and propagate the change uptree.
 * @param count The amount to be added to this node.
 */
void CargoDestNode::update (uint count)
{
	CargoDestNode *n = this;
	do {
		n->count += count;
		n = n->parent;
	} while (n != NULL);
}

/**
 * Estimate the amounts of cargo per final destination for a given cargo,
 * source station and next hop.
 * @param cargo ID of the cargo to estimate destinations for.
 * @param source Source station of the given batch of cargo.
 * @param next Intermediate hop to start the calculation at ("next hop").
 * @param count Size of the batch of cargo.
 */
void CargoDestNode::estimate (CargoID cargo, StationID source,
	StationID next, uint count)
{
	if (!Station::IsValidID(next) || !Station::IsValidID(source)) {
		this->insert (INVALID_STATION)->update (count);
		return;
	}

	std::map <StationID, uint> tmp;
	uint tmp_count = 0;

	const FlowStatMap &flowmap = Station::Get(next)->goods[cargo].flows;
	FlowStatMap::const_iterator map_it = flowmap.find(source);
	if (map_it != flowmap.end()) {
		const FlowStat::SharesMap *shares = map_it->second.GetShares();
		uint32 prev_count = 0;
		for (FlowStat::SharesMap::const_iterator i = shares->begin(); i != shares->end(); ++i) {
			uint add = i->first - prev_count;
			tmp[i->second] += add;
			tmp_count += add;
			prev_count = i->first;
		}
	}

	if (tmp_count == 0) {
		this->insert (INVALID_STATION)->update (count);
		return;
	}

	uint sum_estimated = 0;
	while (sum_estimated < count) {
		for (std::map <StationID, uint>::iterator i = tmp.begin(); i != tmp.end() && sum_estimated < count; ++i) {
			uint estimate = DivideApprox (i->second * count, tmp_count);
			if (estimate == 0) estimate = 1;

			sum_estimated += estimate;
			if (sum_estimated > count) {
				estimate -= sum_estimated - count;
				sum_estimated = count;
				if (estimate == 0) break;
			}

			if (i->first == next) {
				this->insert(next)->update(estimate);
			} else {
				this->estimate (cargo, source, i->first, estimate);
			}
		}
	}
}

/**
 * Rebuild the cache for estimated destinations which is used to quickly show the "destination" entries
 * even if we actually don't know the destination of a certain packet from just looking at it.
 * @param dest CargoDestNode to save the results in.
 * @param st Station to recalculate the cache for.
 * @param i Cargo to recalculate the cache for.
 */
static void RecalcDestinations (CargoDestNode *dest, const Station *st, CargoID i)
{
	dest->clear();

	const FlowStatMap &flows = st->goods[i].flows;
	for (FlowStatMap::const_iterator it = flows.begin(); it != flows.end(); ++it) {
		StationID from = it->first;
		CargoDestNode *source_entry = dest->insert (from);
		const FlowStat::SharesMap *shares = it->second.GetShares();
		uint32 prev_count = 0;
		for (FlowStat::SharesMap::const_iterator flow_it = shares->begin(); flow_it != shares->end(); ++flow_it) {
			StationID via = flow_it->second;
			CargoDestNode *via_entry = source_entry->insert (via);
			if (via == st->index) {
				via_entry->insert(via)->update (flow_it->first - prev_count);
			} else {
				via_entry->estimate (i, from, via, flow_it->first - prev_count);
			}
			prev_count = flow_it->first;
		}
	}
}


struct expanded_map : std::map <StationID, expanded_map> { };

class CargoNodeEntry {
private:
	typedef std::map <StationID, ttd_unique_ptr <CargoNodeEntry> > map;

	CargoNodeEntry *const parent;    ///< The parent of this entry.
	const StationID station;         ///< Station this entry is for.
	expanded_map *const expanded;    ///< Map of expanded nodes, or NULL if this node is not expanded itself.
	uint count;             ///< Total amount of cargo under this node.
	map children;           ///< Children of this node, per station.

protected:
	CargoNodeEntry (StationID id, expanded_map *expanded, CargoNodeEntry *p = NULL)
		: parent(p), station(id), expanded(expanded), count(0)
	{
	}

public:
	typedef map::iterator iterator;
	typedef map::const_iterator const_iterator;

	/** Get the parent of this node. */
	const CargoNodeEntry *get_parent (void) const
	{
		return this->parent;
	}

	/** Get the station of this node. */
	StationID get_station (void) const
	{
		return this->station;
	}

	/** Get the expanded map of this node. */
	expanded_map *get_expanded (void) const
	{
		return this->expanded;
	}

	/** Get the total amount of cargo under this node. */
	uint get_count (void) const
	{
		return this->count;
	}

	/** Check if this node has no children. */
	bool empty (void) const
	{
		return this->children.empty();
	}

	/** Check if there is a single child with the given id. */
	bool has_single_child (StationID station) const
	{
		return (this->children.size() == 1) &&
				(this->children.begin()->first == station);
	}

	CargoNodeEntry *insert (StationID station, expanded_map *expanded);

	void update (uint count);

	typedef std::vector <const CargoNodeEntry *> vector;
	vector sort (CargoSortType type, SortOrder order) const;
};

/** Find or insert a child node of the current node. */
CargoNodeEntry *CargoNodeEntry::insert (StationID station, expanded_map *expanded)
{
	const std::pair <map::iterator, map::iterator> range
		(this->children.equal_range (station));

	if (range.first != range.second) {
		CargoNodeEntry *n = range.first->second.get();
		assert (n->expanded == expanded);
		return n;
	}

	CargoNodeEntry *n = new CargoNodeEntry (station, expanded, this);
	map::iterator iter (this->children.insert (range.first,
		std::make_pair (station, ttd_unique_ptr<CargoNodeEntry>(n))));

	return iter->second.get();
}

/**
 * Update the count for this node and propagate the change uptree.
 * @param count The amount to be added to this node.
 */
void CargoNodeEntry::update (uint count)
{
	CargoNodeEntry *n = this;
	do {
		n->count += count;
		n = n->parent;
	} while (n != NULL);
}

/** Compare two numbers in the given order. */
template<class Tid>
static inline bool sort_id (Tid st1, Tid st2, SortOrder order)
{
	return (order == SO_ASCENDING) ? st1 < st2 : st2 < st1;
}

/** Compare two stations, as given by their id, by their name. */
static bool sort_station (StationID st1, StationID st2, SortOrder order)
{
	static char buf1[MAX_LENGTH_STATION_NAME_CHARS];
	static char buf2[MAX_LENGTH_STATION_NAME_CHARS];

	if (!Station::IsValidID(st1)) {
		return Station::IsValidID(st2) ? (order == SO_ASCENDING) : sort_id (st1, st2, order);
	} else if (!Station::IsValidID(st2)) {
		return order == SO_DESCENDING;
	}

	SetDParam(0, st1);
	GetString (buf1, STR_STATION_NAME);
	SetDParam(0, st2);
	GetString (buf2, STR_STATION_NAME);

	int res = strnatcmp(buf1, buf2); // Sort by name (natural sorting).
	return (res == 0) ? sort_id (st1, st2, order) :
		(order == SO_ASCENDING) ? (res < 0) : (res > 0);
}

struct CargoNodeSorter {
	const CargoSortType type;
	const SortOrder order;

	CargoNodeSorter (CargoSortType t, SortOrder o) : type(t), order(o)
	{
	}

	bool operator() (const CargoNodeEntry *a, const CargoNodeEntry *b);
};

bool CargoNodeSorter::operator() (const CargoNodeEntry *a,
	const CargoNodeEntry *b)
{
	switch (this->type) {
		case ST_COUNT: {
			uint ca = a->get_count();
			uint cb = b->get_count();
			if (ca != cb) {
				return (this->order == SO_ASCENDING) ?
						(ca < cb) : (cb < ca);
			}
		}       /* fall through */
		case ST_STATION_STRING:
			return sort_station (a->get_station(),
					b->get_station(), this->order);
		case ST_STATION_ID:
			return sort_id (a->get_station(), b->get_station(),
					this->order);
		default:
			NOT_REACHED();
	}
}

/** Sort the children into a vector. */
inline CargoNodeEntry::vector CargoNodeEntry::sort (CargoSortType type,
	SortOrder order) const
{
	vector v;
	v.reserve (this->children.size());

	map::const_iterator iter = this->children.begin();
	while (iter != this->children.end()) {
		v.push_back ((iter++)->second.get());
	}

	std::sort (v.begin(), v.end(), CargoNodeSorter (type, order));

	return v;
}

class CargoRootEntry : public CargoNodeEntry {
private:
	bool transfers; ///< If there are transfers for this cargo.
	uint reserved;  ///< Reserved amount of cargo.

public:
	CargoRootEntry (StationID station, expanded_map *expanded)
		: CargoNodeEntry (station, expanded), reserved (0)
	{
	}

	/** Set the transfers state. */
	void set_transfers (bool value) { this->transfers = value; }

	/** Get the transfers state. */
	bool get_transfers (void) const { return this->transfers; }

	/** Update the reserved count. */
	void update_reserved (uint count)
	{
		this->reserved += count;
		this->update (count);
	}

	/** Get the reserved count. */
	uint get_reserved (void) const
	{
		return this->reserved;
	}
};


/**
 * The StationView window
 */
struct StationViewWindow : public Window {
	/**
	 * A row being displayed in the cargo view (as opposed to being "hidden" behind a plus sign).
	 */
	struct RowDisplay {
		RowDisplay(expanded_map *f, StationID n) : filter(f), next_station(n) {}
		RowDisplay(CargoID n) : filter(NULL), next_cargo(n) {}

		/**
		 * Parent of the cargo entry belonging to the row.
		 */
		expanded_map *filter;
		union {
			/**
			 * ID of the station belonging to the entry actually displayed if it's to/from/via.
			 */
			StationID next_station;

			/**
			 * ID of the cargo belonging to the entry actually displayed if it's cargo.
			 */
			CargoID next_cargo;
		};
	};

	typedef std::vector<RowDisplay> CargoDataVector;

	static const int NUM_COLUMNS = 3; ///< Number of extra "columns" in the cargo view: from, via, to

	/**
	 * Type of data invalidation.
	 */
	enum Invalidation {
		INV_FLOWS = 0x100, ///< The planned flows have been recalculated and everything has to be updated.
		INV_CARGO = 0x200  ///< Some cargo has been added or removed.
	};

	/**
	 * Type of grouping used in each of the "columns".
	 */
	enum Grouping {
		GR_SOURCE,      ///< Group by source of cargo ("from").
		GR_NEXT,        ///< Group by next station ("via").
		GR_DESTINATION, ///< Group by estimated final destination ("to").
	};

	/**
	 * Display mode of the cargo view.
	 */
	enum Mode {
		MODE_WAITING, ///< Show cargo waiting at the station.
		MODE_PLANNED  ///< Show cargo planned to pass through the station.
	};

	uint expand_shrink_width;     ///< The width allocated to the expand/shrink 'button'
	int rating_lines;             ///< Number of lines in the cargo ratings view.
	int accepts_lines;            ///< Number of lines in the accepted cargo view.
	Scrollbar *vscroll;

	/** Height of the #WID_SV_ACCEPT_RATING_LIST widget for different views. */
	enum AcceptListHeight {
		ALH_RATING  = 13, ///< Height of the cargo ratings view.
		ALH_ACCEPTS = 3,  ///< Height of the accepted cargo view.
	};

	static const StringID _sort_names[];  ///< Names of the sorting options in the dropdown.
	static const StringID _group_names[]; ///< Names of the grouping options in the dropdown.

	static const Grouping arrangements[6][NUM_COLUMNS]; ///< Possible grouping arrangements.

	/**
	 * Sort types of the different 'columns'.
	 * In fact only ST_COUNT and ST_STATION_STRING are active and you can only
	 * sort all the columns in the same way. The other options haven't been
	 * included in the GUI due to lack of space.
	 */
	CargoSortType sorting;

	/** Sort order (ascending/descending) for the 'columns'. */
	SortOrder sort_order;

	int scroll_to_row;                  ///< If set, scroll the main viewport to the station pointed to by this row.
	int grouping_index;                 ///< Currently selected entry in the grouping drop down.
	Mode current_mode;                  ///< Currently selected display mode of cargo view.
	const Grouping *groupings;          ///< Grouping modes for the different columns.

	CargoDestNode cached_destinations[NUM_CARGO];      ///< Cache for the flows passing through this station.
	std::bitset <NUM_CARGO> cached_destinations_valid; ///< Bitset of up-to-date cached_destinations entries

	std::bitset <NUM_CARGO> expanded_cargoes;          ///< Bitset of expanded cargo rows.
	expanded_map expanded_rows[NUM_CARGO];             ///< Parent entry of currently expanded rows.

	CargoDataVector displayed_rows;     ///< Parent entry of currently displayed rows (including collapsed ones).

	StationViewWindow (const WindowDesc *desc, WindowNumber window_number) :
		Window (desc), expand_shrink_width (0),
		rating_lines (ALH_RATING), accepts_lines (ALH_ACCEPTS),
		vscroll (NULL),
		sorting (ST_COUNT), sort_order (SO_DESCENDING),
		scroll_to_row (INT_MAX), grouping_index (0),
		current_mode (MODE_WAITING), groupings (NULL),
		cached_destinations(), cached_destinations_valid(),
		expanded_cargoes(), expanded_rows(), displayed_rows()
	{
		this->CreateNestedTree();
		this->vscroll = this->GetScrollbar(WID_SV_SCROLLBAR);
		/* Nested widget tree creation is done in two steps to ensure that this->GetWidget<NWidgetCore>(WID_SV_ACCEPTS_RATINGS) exists in UpdateWidgetSize(). */
		this->InitNested(window_number);

		this->SelectGroupBy(_settings_client.gui.station_gui_group_order);
		this->SelectSortBy(_settings_client.gui.station_gui_sort_by);
		this->SelectSortOrder((SortOrder)_settings_client.gui.station_gui_sort_order);
		this->owner = Station::Get(window_number)->owner;
	}

	void OnDelete (void) FINAL_OVERRIDE
	{
		DeleteWindowById(WC_TRAINS_LIST,   VehicleListIdentifier(VL_STATION_LIST, VEH_TRAIN,    this->owner, this->window_number).Pack(), false);
		DeleteWindowById(WC_ROADVEH_LIST,  VehicleListIdentifier(VL_STATION_LIST, VEH_ROAD,     this->owner, this->window_number).Pack(), false);
		DeleteWindowById(WC_SHIPS_LIST,    VehicleListIdentifier(VL_STATION_LIST, VEH_SHIP,     this->owner, this->window_number).Pack(), false);
		DeleteWindowById(WC_AIRCRAFT_LIST, VehicleListIdentifier(VL_STATION_LIST, VEH_AIRCRAFT, this->owner, this->window_number).Pack(), false);
	}

	/**
	 * Show a certain cargo entry characterized by source/next/dest station, cargo ID and amount of cargo at the
	 * right place in the cargo view. I.e. update as many rows as are expanded following that characterization.
	 * @param data Root entry of the tree.
	 * @param cargo Cargo ID of the entry to be shown.
	 * @param source Source station of the entry to be shown.
	 * @param next Next station the cargo to be shown will visit.
	 * @param dest Final destination of the cargo to be shown.
	 * @param count Amount of cargo to be shown.
	 */
	void ShowCargo (CargoRootEntry *root, CargoID cargo, StationID source, StationID next, StationID dest, uint count)
	{
		if (count == 0) return;

		root->set_transfers (source != this->window_number);
		CargoNodeEntry *data = root;

		if (this->expanded_cargoes.test (cargo)) {
			if (_settings_game.linkgraph.GetDistributionType(cargo) != DT_MANUAL) {
				for (int i = 0; i < NUM_COLUMNS; ++i) {
					StationID s;
					switch (groupings[i]) {
						default: NOT_REACHED();
						case GR_SOURCE:
							s = source;
							break;
						case GR_NEXT:
							s = next;
							break;
						case GR_DESTINATION:
							s = dest;
							break;
					}
					expanded_map *expand = data->get_expanded();
					expanded_map::iterator iter = expand->find (s);
					if (iter != expand->end()) {
						data = data->insert (s, &iter->second);
					} else {
						data = data->insert (s, NULL);
						break;
					}
				}
			} else {
				if (source != this->window_number) {
					data = data->insert (source, NULL);
				}
			}
		}

		data->update (count);
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case WID_SV_WAITING:
				resize->height = FONT_HEIGHT_NORMAL;
				size->height = WD_FRAMERECT_TOP + 4 * resize->height + WD_FRAMERECT_BOTTOM;
				this->expand_shrink_width = max(GetStringBoundingBox("-").width, GetStringBoundingBox("+").width) + WD_FRAMERECT_LEFT + WD_FRAMERECT_RIGHT;
				break;

			case WID_SV_ACCEPT_RATING_LIST:
				size->height = WD_FRAMERECT_TOP + ((this->GetWidget<NWidgetCore>(WID_SV_ACCEPTS_RATINGS)->widget_data == STR_STATION_VIEW_RATINGS_BUTTON) ? this->accepts_lines : this->rating_lines) * FONT_HEIGHT_NORMAL + WD_FRAMERECT_BOTTOM;
				break;

			case WID_SV_CLOSE_AIRPORT:
				if (!(Station::Get(this->window_number)->facilities & FACIL_AIRPORT)) {
					/* Hide 'Close Airport' button if no airport present. */
					size->width = 0;
					resize->width = 0;
					fill->width = 0;
				}
				break;
		}
	}

	void OnPaint (BlitArea *dpi) OVERRIDE
	{
		const Station *st = Station::Get(this->window_number);

		/* disable some buttons */
		this->SetWidgetDisabledState(WID_SV_RENAME,   st->owner != _local_company);
		this->SetWidgetDisabledState(WID_SV_TRAINS,   !(st->facilities & FACIL_TRAIN));
		this->SetWidgetDisabledState(WID_SV_ROADVEHS, !(st->facilities & FACIL_TRUCK_STOP) && !(st->facilities & FACIL_BUS_STOP));
		this->SetWidgetDisabledState(WID_SV_SHIPS,    !(st->facilities & FACIL_DOCK));
		this->SetWidgetDisabledState(WID_SV_PLANES,   !(st->facilities & FACIL_AIRPORT));
		this->SetWidgetDisabledState(WID_SV_CLOSE_AIRPORT, !(st->facilities & FACIL_AIRPORT) || st->owner != _local_company || st->owner == OWNER_NONE); // Also consider SE, where _local_company == OWNER_NONE
		this->SetWidgetLoweredState(WID_SV_CLOSE_AIRPORT, (st->facilities & FACIL_AIRPORT) && (st->airport.flags & AIRPORT_CLOSED_block) != 0);

		this->DrawWidgets (dpi);

		if (!this->IsShaded()) {
			/* Draw 'accepted cargo' or 'cargo ratings'. */
			const NWidgetBase *wid = this->GetWidget<NWidgetBase>(WID_SV_ACCEPT_RATING_LIST);
			const Rect r = {(int)wid->pos_x, (int)wid->pos_y, (int)(wid->pos_x + wid->current_x - 1), (int)(wid->pos_y + wid->current_y - 1)};
			if (this->GetWidget<NWidgetCore>(WID_SV_ACCEPTS_RATINGS)->widget_data == STR_STATION_VIEW_RATINGS_BUTTON) {
				int lines = this->DrawAcceptedCargo (dpi, r);
				if (lines > this->accepts_lines) { // Resize the widget, and perform re-initialization of the window.
					this->accepts_lines = lines;
					this->ReInit();
					return;
				}
			} else {
				int lines = this->DrawCargoRatings (dpi, r);
				if (lines > this->rating_lines) { // Resize the widget, and perform re-initialization of the window.
					this->rating_lines = lines;
					this->ReInit();
					return;
				}
			}

			/* Draw arrow pointing up/down for ascending/descending sorting */
			this->DrawSortButtonState (dpi, WID_SV_SORT_ORDER, sort_order == SO_ASCENDING ? SBS_UP : SBS_DOWN);

			int pos = this->vscroll->GetPosition();

			int maxrows = this->vscroll->GetCapacity();

			displayed_rows.clear();

			/* Draw waiting cargo. */
			NWidgetBase *nwi = this->GetWidget<NWidgetBase>(WID_SV_WAITING);
			Rect waiting_rect = { (int)nwi->pos_x, (int)nwi->pos_y, (int)(nwi->pos_x + nwi->current_x - 1), (int)(nwi->pos_y + nwi->current_y - 1)};

			for (CargoID i = 0; i < NUM_CARGO; i++) {
				if (!this->cached_destinations_valid.test (i)) {
					this->cached_destinations_valid.set (i);
					RecalcDestinations (&this->cached_destinations[i], st, i);
				}

				CargoRootEntry cargo (this->window_number, &this->expanded_rows[i]);
				if (this->current_mode == MODE_WAITING) {
					this->BuildCargoList (i, st->goods[i].cargo, &cargo);
				} else {
					this->BuildFlowList (i, st->goods[i].flows, &cargo);
				}

				if (cargo.get_count() > 0) {
					pos = this->DrawCargoEntry (&cargo, i, dpi, waiting_rect, pos, maxrows);
				}
			}
			this->vscroll->SetCount (this->vscroll->GetPosition() - pos); // update scrollbar

			scroll_to_row = INT_MAX;
		}
	}

	virtual void SetStringParameters(int widget) const
	{
		const Station *st = Station::Get(this->window_number);
		SetDParam(0, st->index);
		SetDParam(1, st->facilities);
	}

	/**
	 * Build up the cargo view for PLANNED mode and a specific cargo.
	 * @param i Cargo to show.
	 * @param flows The current station's flows for that cargo.
	 * @param cargo The CargoRootEntry to save the results in.
	 */
	void BuildFlowList (CargoID i, const FlowStatMap &flows, CargoRootEntry *cargo)
	{
		const CargoDestNode *source_dest = &this->cached_destinations[i];
		for (FlowStatMap::const_iterator it = flows.begin(); it != flows.end(); ++it) {
			StationID from = it->first;
			const CargoDestNode *source_entry = source_dest->find (from);
			const FlowStat::SharesMap *shares = it->second.GetShares();
			for (FlowStat::SharesMap::const_iterator flow_it = shares->begin(); flow_it != shares->end(); ++flow_it) {
				const CargoDestNode *via_entry = source_entry->find (flow_it->second);
				for (CargoDestNode::iterator dest_it = via_entry->begin(); dest_it != via_entry->end(); ++dest_it) {
					ShowCargo (cargo, i, from, flow_it->second, dest_it->first, dest_it->second.count);
				}
			}
		}
	}

	/**
	 * Build up the cargo view for WAITING mode and a specific cargo.
	 * @param i Cargo to show.
	 * @param packets The current station's cargo list for that cargo.
	 * @param cargo The CargoRootEntry to save the result in.
	 */
	void BuildCargoList (CargoID i, const StationCargoList &packets, CargoRootEntry *cargo)
	{
		const CargoDestNode *source_dest = &this->cached_destinations[i];
		for (StationCargoList::ConstIterator it = packets.Packets()->begin(); it != packets.Packets()->end(); it++) {
			const CargoPacket *cp = *it;
			StationID next = it.GetKey();

			const CargoDestNode *source_entry = source_dest->find (cp->SourceStation());
			if (source_entry == NULL) {
				this->ShowCargo(cargo, i, cp->SourceStation(), next, INVALID_STATION, cp->Count());
				continue;
			}

			const CargoDestNode *via_entry = source_entry->find (next);
			if (via_entry == NULL) {
				this->ShowCargo(cargo, i, cp->SourceStation(), next, INVALID_STATION, cp->Count());
				continue;
			}

			for (CargoDestNode::iterator dest_it = via_entry->begin(); dest_it != via_entry->end(); ++dest_it) {
				uint val = DivideApprox (cp->Count() * dest_it->second.count, via_entry->count);
				this->ShowCargo (cargo, i, cp->SourceStation(), next, dest_it->first, val);
			}
		}

		uint reserved = packets.ReservedCount();
		if (reserved != 0) {
			if (this->expanded_cargoes.test (i)) {
				cargo->set_transfers (true);
				cargo->update_reserved (reserved);
			} else {
				cargo->update (reserved);
			}
		}
	}

	/**
	 * Select the correct string for an entry referring to the specified station.
	 * @param station Station the entry is showing cargo for.
	 * @param here String to be shown if the entry refers to the same station as this station GUI belongs to.
	 * @param other_station String to be shown if the entry refers to a specific other station.
	 * @param any String to be shown if the entry refers to "any station".
	 * @return One of the three given strings, depending on what station the entry refers to.
	 */
	StringID GetEntryString(StationID station, StringID here, StringID other_station, StringID any)
	{
		if (station == this->window_number) {
			return here;
		} else if (station == INVALID_STATION) {
			return any;
		} else {
			SetDParam(2, station);
			return other_station;
		}
	}

	/**
	 * Determine if we need to show the special "non-stop" string.
	 * @param cd Entry we are going to show.
	 * @param station Station the entry refers to.
	 * @param column The "column" the entry will be shown in.
	 * @return either STR_STATION_VIEW_VIA or STR_STATION_VIEW_NONSTOP.
	 */
	StringID SearchNonStop (const CargoNodeEntry *cd, StationID station, int column)
	{
		const CargoNodeEntry *parent = cd->get_parent();
		for (int i = column; i > 0; --i) {
			if (this->groupings[i - 1] == GR_DESTINATION) {
				if (parent->get_station() == station) {
					return STR_STATION_VIEW_NONSTOP;
				} else {
					return STR_STATION_VIEW_VIA;
				}
			}
			parent = parent->get_parent();
		}

		if (this->groupings[column + 1] == GR_DESTINATION) {
			if (cd->has_single_child (station)) {
				return STR_STATION_VIEW_NONSTOP;
			} else {
				return STR_STATION_VIEW_VIA;
			}
		}

		return STR_STATION_VIEW_VIA;
	}

	/**
	 * Draw the cargo string for an entry in the station GUI.
	 * @param dpi Area to draw on.
	 * @param r Screen rectangle to draw into.
	 * @param y Vertical position to draw at.
	 * @param indent Extra indentation for the string.
	 * @param sym Symbol to draw at the end of the line, if not null.
	 * @param str String to draw.
	 */
	void DrawCargoString (BlitArea *dpi, const Rect &r, int y, int indent,
		const char *sym, StringID str)
	{
		bool rtl = _current_text_dir == TD_RTL;

		int text_left  = rtl ? r.left + this->expand_shrink_width : r.left + WD_FRAMERECT_LEFT + indent * this->expand_shrink_width;
		int text_right = rtl ? r.right - WD_FRAMERECT_LEFT - indent * this->expand_shrink_width : r.right - this->expand_shrink_width;
		DrawString (dpi, text_left, text_right, y, str);

		if (sym) {
			int sym_left  = rtl ? r.left + WD_FRAMERECT_LEFT : r.right - this->expand_shrink_width + WD_FRAMERECT_LEFT;
			int sym_right = rtl ? r.left + this->expand_shrink_width - WD_FRAMERECT_RIGHT : r.right - WD_FRAMERECT_RIGHT;
			DrawString (dpi, sym_left, sym_right, y, sym, TC_YELLOW);
		}
	}

	/**
	 * Draw the given cargo entries in the station GUI.
	 * @param entry Root entry for all cargo to be drawn.
	 * @param dpi Area to draw on.
	 * @param r Screen rectangle to draw into.
	 * @param pos Current row to be drawn to (counted down from 0 to -maxrows, same as vscroll->GetPosition()).
	 * @param maxrows Maximum row to be drawn.
	 * @param column Current "column" being drawn.
	 * @param cargo Current cargo being drawn.
	 * @return row (in "pos" counting) after the one we have last drawn to.
	 */
	int DrawEntries (const CargoNodeEntry *entry, BlitArea *dpi, const Rect &r,
		int pos, int maxrows, int column, CargoID cargo)
	{
		assert (entry->empty() || (entry->get_expanded() != NULL));

		typedef CargoNodeEntry::vector vector;
		vector v = entry->sort (this->sorting, this->sort_order);

		for (vector::const_iterator i = v.begin(); i != v.end(); ++i) {
			const CargoNodeEntry *cd = *i;

			bool auto_distributed = _settings_game.linkgraph.GetDistributionType(cargo) != DT_MANUAL;
			assert (auto_distributed || (column == 0));

			if (pos > -maxrows && pos <= 0) {
				StringID str = STR_EMPTY;
				int y = r.top + WD_FRAMERECT_TOP - pos * FONT_HEIGHT_NORMAL;
				SetDParam(0, cargo);
				SetDParam(1, cd->get_count());

				Grouping grouping = auto_distributed ? this->groupings[column] : GR_SOURCE;
				StationID station = cd->get_station();

				switch (grouping) {
					case GR_SOURCE:
						str = this->GetEntryString(station, STR_STATION_VIEW_FROM_HERE, STR_STATION_VIEW_FROM, STR_STATION_VIEW_FROM_ANY);
						break;
					case GR_NEXT:
						str = this->GetEntryString(station, STR_STATION_VIEW_VIA_HERE, STR_STATION_VIEW_VIA, STR_STATION_VIEW_VIA_ANY);
						if (str == STR_STATION_VIEW_VIA) str = this->SearchNonStop(cd, station, column);
						break;
					case GR_DESTINATION:
						str = this->GetEntryString(station, STR_STATION_VIEW_TO_HERE, STR_STATION_VIEW_TO, STR_STATION_VIEW_TO_ANY);
						break;
					default:
						NOT_REACHED();
				}
				if (pos == -this->scroll_to_row && Station::IsValidID(station)) {
					ScrollMainWindowToTile(Station::Get(station)->xy);
				}

				const char *sym = NULL;
				if (column < NUM_COLUMNS - 1) {
					if (!cd->empty()) {
						sym = "-";
					} else if (auto_distributed) {
						sym = "+";
					}
				}

				this->DrawCargoString (dpi, r, y, column + 1, sym, str);

				expanded_map *expand = entry->get_expanded();
				assert (expand != NULL);
				this->displayed_rows.push_back (RowDisplay (expand, station));
			}
			--pos;
			if (auto_distributed) {
				pos = this->DrawEntries (cd, dpi, r, pos, maxrows, column + 1, cargo);
			}
		}
		return pos;
	}

	/**
	 * Draw the given cargo entry in the station GUI.
	 * @param cd Cargo entry to be drawn.
	 * @param cargo Cargo type for this entry.
	 * @param dpi Area to draw on.
	 * @param r Screen rectangle to draw into.
	 * @param pos Current row to be drawn to (counted down from 0 to -maxrows, same as vscroll->GetPosition()).
	 * @param maxrows Maximum row to be drawn.
	 * @return row (in "pos" counting) after the one we have last drawn to.
	 */
	int DrawCargoEntry (const CargoRootEntry *cd, CargoID cargo,
		BlitArea *dpi, const Rect &r, int pos, int maxrows)
	{
		bool auto_distributed = _settings_game.linkgraph.GetDistributionType(cargo) != DT_MANUAL;

		if (pos > -maxrows && pos <= 0) {
			int y = r.top + WD_FRAMERECT_TOP - pos * FONT_HEIGHT_NORMAL;
			SetDParam(0, cargo);
			SetDParam(1, cd->get_count());
			StringID str = STR_STATION_VIEW_WAITING_CARGO;
			DrawCargoIcons (cargo, cd->get_count(), dpi, r.left + WD_FRAMERECT_LEFT + this->expand_shrink_width, r.right - WD_FRAMERECT_RIGHT - this->expand_shrink_width, y);

			const char *sym = NULL;
			if (!cd->empty() || (cd->get_reserved() > 0)) {
				sym = "-";
			} else if (auto_distributed) {
				sym = "+";
			} else {
				/* Only draw '+' if there is something to be shown. */
				const StationCargoList &list = Station::Get(this->window_number)->goods[cargo].cargo;
				if (list.ReservedCount() > 0 || cd->get_transfers()) {
					sym = "+";
				}
			}

			this->DrawCargoString (dpi, r, y, 0, sym, str);

			this->displayed_rows.push_back (RowDisplay (cargo));
		}

		pos = this->DrawEntries (cd, dpi, r, pos - 1, maxrows, 0, cargo);

		if (cd->get_reserved() != 0) {
			if (pos > -maxrows && pos <= 0) {
				int y = r.top + WD_FRAMERECT_TOP - pos * FONT_HEIGHT_NORMAL;
				SetDParam (0, cargo);
				SetDParam (1, cd->get_reserved());
				this->DrawCargoString (dpi, r, y, 1, NULL, STR_STATION_VIEW_RESERVED);
				this->displayed_rows.push_back (RowDisplay (INVALID_CARGO));
			}
			pos--;
		}

		return pos;
	}

	/**
	 * Draw accepted cargo in the #WID_SV_ACCEPT_RATING_LIST widget.
	 * @param dpi Area to draw on.
	 * @param r Rectangle of the widget.
	 * @return Number of lines needed for drawing the accepted cargo.
	 */
	int DrawAcceptedCargo (BlitArea *dpi, const Rect &r) const
	{
		const Station *st = Station::Get(this->window_number);

		uint32 cargo_mask = 0;
		for (CargoID i = 0; i < NUM_CARGO; i++) {
			if (HasBit(st->goods[i].status, GoodsEntry::GES_ACCEPTANCE)) SetBit(cargo_mask, i);
		}
		SetDParam(0, cargo_mask);
		int bottom = DrawStringMultiLine (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, r.top + WD_FRAMERECT_TOP, INT32_MAX, STR_STATION_VIEW_ACCEPTS_CARGO);
		return CeilDiv(bottom - r.top - WD_FRAMERECT_TOP, FONT_HEIGHT_NORMAL);
	}

	/**
	 * Draw cargo ratings in the #WID_SV_ACCEPT_RATING_LIST widget.
	 * @param dpi Area to draw on.
	 * @param r Rectangle of the widget.
	 * @return Number of lines needed for drawing the cargo ratings.
	 */
	int DrawCargoRatings (BlitArea *dpi, const Rect &r) const
	{
		const Station *st = Station::Get(this->window_number);
		int y = r.top + WD_FRAMERECT_TOP;

		if (st->town->exclusive_counter > 0) {
			SetDParam(0, st->town->exclusivity);
			y = DrawStringMultiLine (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, r.bottom, st->town->exclusivity == st->owner ? STR_STATION_VIEW_EXCLUSIVE_RIGHTS_SELF : STR_STATION_VIEW_EXCLUSIVE_RIGHTS_COMPANY);
			y += WD_PAR_VSEP_WIDE;
		}

		DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, STR_STATION_VIEW_SUPPLY_RATINGS_TITLE);
		y += FONT_HEIGHT_NORMAL;

		const CargoSpec *cs;
		FOR_ALL_SORTED_STANDARD_CARGOSPECS(cs) {
			const GoodsEntry *ge = &st->goods[cs->Index()];
			if (!ge->HasRating()) continue;

			const LinkGraph *lg = LinkGraph::GetIfValid(ge->link_graph);
			SetDParam(0, cs->name);
			SetDParam(1, lg != NULL ? lg->Monthly((*lg)[ge->node]->Supply()) : 0);
			SetDParam(2, STR_CARGO_RATING_APPALLING + (ge->rating >> 5));
			SetDParam(3, ToPercent8(ge->rating));
			DrawString (dpi, r.left + WD_FRAMERECT_LEFT + 6, r.right - WD_FRAMERECT_RIGHT - 6, y, STR_STATION_VIEW_CARGO_SUPPLY_RATING);
			y += FONT_HEIGHT_NORMAL;
		}
		return CeilDiv(y - r.top - WD_FRAMERECT_TOP, FONT_HEIGHT_NORMAL);
	}

	/**
	 * Handle a click on a specific row in the cargo view.
	 * @param row Row being clicked.
	 */
	void HandleCargoWaitingClick(int row)
	{
		if (row < 0 || (uint)row >= this->displayed_rows.size()) return;
		if (_ctrl_pressed) {
			this->scroll_to_row = row;
		} else {
			RowDisplay &display = this->displayed_rows[row];
			expanded_map *filter = display.filter;
			if (filter != NULL) {
				StationID next = display.next_station;
				expanded_map::iterator iter = filter->find (next);
				if (iter != filter->end()) {
					filter->erase (iter);
				} else {
					(*filter)[next];
				}
			} else if (display.next_cargo != INVALID_CARGO) {
				this->expanded_cargoes.flip (display.next_cargo);
			}
		}
		this->SetWidgetDirty(WID_SV_WAITING);
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_SV_WAITING:
				this->HandleCargoWaitingClick(this->vscroll->GetScrolledRowFromWidget(pt.y, this, WID_SV_WAITING, WD_FRAMERECT_TOP, FONT_HEIGHT_NORMAL) - this->vscroll->GetPosition());
				break;

			case WID_SV_LOCATION:
				if (_ctrl_pressed) {
					ShowExtraViewPortWindow(Station::Get(this->window_number)->xy);
				} else {
					ScrollMainWindowToTile(Station::Get(this->window_number)->xy);
				}
				break;

			case WID_SV_ACCEPTS_RATINGS: {
				/* Swap between 'accepts' and 'ratings' view. */
				int height_change;
				NWidgetCore *nwi = this->GetWidget<NWidgetCore>(WID_SV_ACCEPTS_RATINGS);
				if (this->GetWidget<NWidgetCore>(WID_SV_ACCEPTS_RATINGS)->widget_data == STR_STATION_VIEW_RATINGS_BUTTON) {
					nwi->SetDataTip(STR_STATION_VIEW_ACCEPTS_BUTTON, STR_STATION_VIEW_ACCEPTS_TOOLTIP); // Switch to accepts view.
					height_change = this->rating_lines - this->accepts_lines;
				} else {
					nwi->SetDataTip(STR_STATION_VIEW_RATINGS_BUTTON, STR_STATION_VIEW_RATINGS_TOOLTIP); // Switch to ratings view.
					height_change = this->accepts_lines - this->rating_lines;
				}
				this->ReInit(0, height_change * FONT_HEIGHT_NORMAL);
				break;
			}

			case WID_SV_RENAME:
				SetDParam(0, this->window_number);
				ShowQueryString(STR_STATION_NAME, STR_STATION_VIEW_RENAME_STATION_CAPTION, MAX_LENGTH_STATION_NAME_CHARS,
						this, CS_ALPHANUMERAL, QSF_ENABLE_DEFAULT | QSF_LEN_IN_CHARS);
				break;

			case WID_SV_CLOSE_AIRPORT:
				DoCommandP(0, this->window_number, 0, CMD_OPEN_CLOSE_AIRPORT);
				break;

			case WID_SV_TRAINS:   // Show list of scheduled trains to this station
			case WID_SV_ROADVEHS: // Show list of scheduled road-vehicles to this station
			case WID_SV_SHIPS:    // Show list of scheduled ships to this station
			case WID_SV_PLANES: { // Show list of scheduled aircraft to this station
				Owner owner = Station::Get(this->window_number)->owner;
				ShowVehicleListWindow(owner, (VehicleType)(widget - WID_SV_TRAINS), (StationID)this->window_number);
				break;
			}

			case WID_SV_SORT_BY: {
				/* The initial selection is composed of current mode and
				 * sorting criteria for columns 1, 2, and 3. Column 0 is always
				 * sorted by cargo ID. The others can theoretically be sorted
				 * by different things but there is no UI for that. */
				ShowDropDownMenu(this, _sort_names,
						this->current_mode * 2 + (this->sorting == ST_COUNT ? 1 : 0),
						WID_SV_SORT_BY, 0, 0);
				break;
			}

			case WID_SV_GROUP_BY: {
				ShowDropDownMenu(this, _group_names, this->grouping_index, WID_SV_GROUP_BY, 0, 0);
				break;
			}

			case WID_SV_SORT_ORDER: { // flip sorting method asc/desc
				this->SelectSortOrder (this->sort_order == SO_ASCENDING ? SO_DESCENDING : SO_ASCENDING);
				this->SetTimeout();
				this->LowerWidget(WID_SV_SORT_ORDER);
				break;
			}
		}
	}

	/**
	 * Select a new sort order for the cargo view.
	 * @param order New sort order.
	 */
	void SelectSortOrder(SortOrder order)
	{
		this->sort_order = order;
		_settings_client.gui.station_gui_sort_order = order;
		this->SetDirty();
	}

	/**
	 * Select a new sort criterium for the cargo view.
	 * @param index Row being selected in the sort criteria drop down.
	 */
	void SelectSortBy(int index)
	{
		_settings_client.gui.station_gui_sort_by = index;
		switch (_sort_names[index]) {
			case STR_STATION_VIEW_WAITING_STATION:
				this->current_mode = MODE_WAITING;
				this->sorting = ST_STATION_STRING;
				break;
			case STR_STATION_VIEW_WAITING_AMOUNT:
				this->current_mode = MODE_WAITING;
				this->sorting = ST_COUNT;
				break;
			case STR_STATION_VIEW_PLANNED_STATION:
				this->current_mode = MODE_PLANNED;
				this->sorting = ST_STATION_STRING;
				break;
			case STR_STATION_VIEW_PLANNED_AMOUNT:
				this->current_mode = MODE_PLANNED;
				this->sorting = ST_COUNT;
				break;
			default:
				NOT_REACHED();
		}
		/* Display the current sort variant */
		this->GetWidget<NWidgetCore>(WID_SV_SORT_BY)->widget_data = _sort_names[index];
		this->SetDirty();
	}

	/**
	 * Select a new grouping mode for the cargo view.
	 * @param index Row being selected in the grouping drop down.
	 */
	void SelectGroupBy(int index)
	{
		this->grouping_index = index;
		_settings_client.gui.station_gui_group_order = index;
		this->GetWidget<NWidgetCore>(WID_SV_GROUP_BY)->widget_data = _group_names[index];
		this->groupings = arrangements[index];
		this->SetDirty();
	}

	virtual void OnDropdownSelect(int widget, int index)
	{
		if (widget == WID_SV_SORT_BY) {
			this->SelectSortBy(index);
		} else {
			this->SelectGroupBy(index);
		}
	}

	virtual void OnQueryTextFinished(char *str)
	{
		if (str == NULL) return;

		DoCommandP(0, this->window_number, 0, CMD_RENAME_STATION, str);
	}

	virtual void OnResize()
	{
		this->vscroll->SetCapacityFromWidget(this, WID_SV_WAITING, WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM);
	}

	/**
	 * Some data on this window has become invalid. Invalidate the cache for the given cargo if necessary.
	 * @param data Information about the changed data. If it's a valid cargo ID, invalidate the cargo data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (gui_scope) {
			if (data >= 0 && data < NUM_CARGO) {
				this->cached_destinations_valid.reset (data);
			} else {
				this->ReInit();
			}
		}
	}
};

const StringID StationViewWindow::_sort_names[] = {
	STR_STATION_VIEW_WAITING_STATION,
	STR_STATION_VIEW_WAITING_AMOUNT,
	STR_STATION_VIEW_PLANNED_STATION,
	STR_STATION_VIEW_PLANNED_AMOUNT,
	INVALID_STRING_ID
};

const StringID StationViewWindow::_group_names[] = {
	STR_STATION_VIEW_GROUP_S_V_D,
	STR_STATION_VIEW_GROUP_S_D_V,
	STR_STATION_VIEW_GROUP_V_S_D,
	STR_STATION_VIEW_GROUP_V_D_S,
	STR_STATION_VIEW_GROUP_D_S_V,
	STR_STATION_VIEW_GROUP_D_V_S,
	INVALID_STRING_ID
};

const StationViewWindow::Grouping StationViewWindow::arrangements[6][NUM_COLUMNS] = {
	{ GR_SOURCE, GR_NEXT, GR_DESTINATION }, // S_V_D
	{ GR_SOURCE, GR_DESTINATION, GR_NEXT }, // S_D_V
	{ GR_NEXT, GR_SOURCE, GR_DESTINATION }, // V_S_D
	{ GR_NEXT, GR_DESTINATION, GR_SOURCE }, // V_D_S
	{ GR_DESTINATION, GR_SOURCE, GR_NEXT }, // D_S_V
	{ GR_DESTINATION, GR_NEXT, GR_SOURCE }, // D_V_S
};

assert_compile (lengthof(StationViewWindow::_group_names) == lengthof(StationViewWindow::arrangements) + 1);

static WindowDesc::Prefs _station_view_prefs ("view_station");

static const WindowDesc _station_view_desc(
	WDP_AUTO, 249, 117,
	WC_STATION_VIEW, WC_NONE,
	0,
	_nested_station_view_widgets, lengthof(_nested_station_view_widgets),
	&_station_view_prefs
);

/**
 * Opens StationViewWindow for given station
 *
 * @param station station which window should be opened
 */
void ShowStationViewWindow(StationID station)
{
	AllocateWindowDescFront<StationViewWindow>(&_station_view_desc, station);
}

/**
 * Find a station of the given type in the given area.
 * @param ta Base tile area of the to-be-built station
 * @param waypoint Look for a waypoint, else a station
 * @return Whether a station was found
 */
static bool FindStationInArea (const TileArea &ta, bool waypoint)
{
	TILE_AREA_LOOP(t, ta) {
		if (IsStationTile(t)) {
			BaseStation *bst = BaseStation::GetByTile(t);
			if (bst->IsWaypoint() == waypoint) return true;
		}
	}

	return false;
}

/**
 * Circulate around the to-be-built station to find stations we could join.
 * Make sure that only stations are returned where joining wouldn't exceed
 * station spread and are our own station.
 * @param ta Base tile area of the to-be-built station
 * @param distant_join Search for adjacent stations (false) or stations fully
 *                     within station spread
 * @param waypoint Look for a waypoint, else a station
 */
static void FindStationsNearby (std::vector<StationID> *list, const TileArea &ta, bool distant_join, bool waypoint)
{
	/* Look for deleted stations */
	typedef std::multimap <TileIndex, StationID> deleted_map;
	deleted_map deleted;
	const BaseStation *st;
	FOR_ALL_BASE_STATIONS(st) {
		if (st->IsWaypoint() == waypoint && !st->IsInUse() && st->owner == _local_company
				/* Include only within station spread */
				&& DistanceMax (ta.tile, st->xy) < _settings_game.station.station_spread
				&& DistanceMax (TILE_ADDXY(ta.tile, ta.w - 1, ta.h - 1), st->xy) < _settings_game.station.station_spread) {
			if (ta.Contains (st->xy)) {
				/* Add the station directly if it falls
				 * into the covered area. */
				list->push_back (st->index);
			} else {
				/* Otherwise, store it for later. */
				deleted.insert (std::make_pair (st->xy, st->index));
			}
		}
	}

	/* Only search tiles where we have a chance to stay within the station spread.
	 * The complete check needs to be done in the callback as we don't know the
	 * extent of the found station, yet. */
	uint min_dim = min (ta.w, ta.h);
	if (min_dim >= _settings_game.station.station_spread) return;

	/* Keep a set of stations already checked. */
	std::set<StationID> seen;
	CircularTileIterator iter (ta,
			distant_join ? _settings_game.station.station_spread - min_dim : 1);
	for (TileIndex tile = iter; tile != INVALID_TILE; tile = ++iter) {
		/* First check if there were deleted stations here */
		const std::pair <deleted_map::iterator, deleted_map::iterator> range = deleted.equal_range (tile);
		for (deleted_map::const_iterator i = range.first; i != range.second; i++) {
			list->push_back (i->second);
		}
		deleted.erase (range.first, range.second);

		/* Check if own station and if we stay within station spread */
		if (!IsStationTile(tile)) continue;

		StationID sid = GetStationIndex(tile);
		BaseStation *st = BaseStation::Get(sid);

		/* This station is (likely) a waypoint */
		if (st->IsWaypoint() != waypoint) continue;

		if (st->owner != _local_company) continue;

		if (seen.insert(sid).second) {
			TileArea test (ta);
			test.Add (st->rect);
			if (test.w <= _settings_game.station.station_spread
					&& test.h <= _settings_game.station.station_spread) {
				list->push_back (sid);
			}
		}
	}
}

static const NWidgetPart _nested_select_station_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_JS_CAPTION), SetDataTip(STR_JOIN_STATION_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_DEFSIZEBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN, WID_JS_PANEL), SetResize(1, 0), SetScrollbar(WID_JS_SCROLLBAR), EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_VSCROLLBAR, COLOUR_DARK_GREEN, WID_JS_SCROLLBAR),
			NWidget(WWT_RESIZEBOX, COLOUR_DARK_GREEN),
		EndContainer(),
	EndContainer(),
};

/**
 * Window for selecting stations/waypoints to (distant) join to.
 */
struct SelectStationWindow : Window {
	Command select_station_cmd; ///< Command to build new station
	const TileArea area; ///< Location of new station
	const bool waypoint; ///< Select waypoints, else stations
	std::vector<StationID> list; ///< List of nearby stations
	Scrollbar *vscroll;

	SelectStationWindow (const WindowDesc *desc, const Command &cmd, const TileArea &ta, bool waypoint, const std::vector<StationID> &list) :
		Window(desc),
		select_station_cmd(cmd),
		area(ta),
		waypoint(waypoint),
		list(list),
		vscroll(NULL)
	{
		this->CreateNestedTree();
		this->vscroll = this->GetScrollbar(WID_JS_SCROLLBAR);
		this->GetWidget<NWidgetCore>(WID_JS_CAPTION)->widget_data = waypoint ? STR_JOIN_WAYPOINT_CAPTION : STR_JOIN_STATION_CAPTION;
		this->InitNested(0);
		this->OnInvalidateData(0);
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		if (widget != WID_JS_PANEL) return;

		/* Determine the widest string */
		Dimension d = GetStringBoundingBox(this->waypoint ? STR_JOIN_WAYPOINT_CREATE_SPLITTED_WAYPOINT : STR_JOIN_STATION_CREATE_SPLITTED_STATION);
		for (uint i = 0; i < this->list.size(); i++) {
			const BaseStation *st = BaseStation::Get(this->list[i]);
			SetDParam(0, st->index);
			SetDParam(1, st->facilities);
			d = maxdim(d, GetStringBoundingBox(this->waypoint ? STR_STATION_LIST_WAYPOINT : STR_STATION_LIST_STATION));
		}

		resize->height = d.height;
		d.height *= 5;
		d.width += WD_FRAMERECT_RIGHT + WD_FRAMERECT_LEFT;
		d.height += WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM;
		*size = d;
	}

	void DrawWidget (BlitArea *dpi, const Rect &r, int widget) const OVERRIDE
	{
		if (widget != WID_JS_PANEL) return;

		uint y = r.top + WD_FRAMERECT_TOP;
		if (this->vscroll->GetPosition() == 0) {
			DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, this->waypoint ? STR_JOIN_WAYPOINT_CREATE_SPLITTED_WAYPOINT : STR_JOIN_STATION_CREATE_SPLITTED_STATION);
			y += this->resize.step_height;
		}

		for (uint i = max<uint>(1, this->vscroll->GetPosition()); i <= this->list.size(); ++i, y += this->resize.step_height) {
			/* Don't draw anything if it extends past the end of the window. */
			if (i - this->vscroll->GetPosition() >= this->vscroll->GetCapacity()) break;

			const BaseStation *st = BaseStation::Get(this->list[i - 1]);
			SetDParam(0, st->index);
			SetDParam(1, st->facilities);
			DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, this->waypoint ? STR_STATION_LIST_WAYPOINT : STR_STATION_LIST_STATION);
		}
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		if (widget != WID_JS_PANEL) return;

		uint st_index = this->vscroll->GetScrolledRowFromWidget(pt.y, this, WID_JS_PANEL, WD_FRAMERECT_TOP);
		if (st_index > this->list.size()) return;

		/* Insert station to be joined into stored command */
		SB(this->select_station_cmd.p2, 16, 16,
			(st_index > 0) ? this->list[st_index - 1] : INVALID_STATION);

		/* Execute stored Command */
		this->select_station_cmd.execp();

		/* Close Window; this might cause double frees! */
		DeleteWindowById(WC_SELECT_STATION, 0);
	}

	virtual void OnTick()
	{
		if (_thd.dirty & 2) {
			_thd.dirty &= ~2;
			this->SetDirty();
		}
	}

	virtual void OnResize()
	{
		this->vscroll->SetCapacityFromWidget(this, WID_JS_PANEL, WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM);
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (!gui_scope) return;

		this->list.clear();

		if (!FindStationInArea (this->area, this->waypoint)) {
			FindStationsNearby (&this->list, this->area, _settings_game.station.distant_join_stations, this->waypoint);
		}

		this->vscroll->SetCount (this->list.size() + 1);
		this->SetDirty();
	}
};

static WindowDesc::Prefs _select_station_prefs ("build_station_join");

static const WindowDesc _select_station_desc(
	WDP_AUTO, 200, 180,
	WC_SELECT_STATION, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_select_station_widgets, lengthof(_nested_select_station_widgets),
	&_select_station_prefs
);


/**
 * Show the station selection window when needed. If not, build the station.
 * @param cmd Command to build the station.
 * @param ta Area to build the station in
 * @param waypoint Look for waypoints, else stations
 */
void ShowSelectBaseStationIfNeeded (Command *cmd, const TileArea &ta, bool waypoint)
{
	/* If a window is already opened and we didn't ctrl-click,
	 * return true (i.e. just flash the old window) */
	Window *selection_window = FindWindowById(WC_SELECT_STATION, 0);
	if (selection_window != NULL) {
		/* Abort current distant-join and start new one */
		selection_window->Delete();
	}

	/* Only show the popup if we press ctrl and we can build there. */
	if (_ctrl_pressed && cmd->exec(CommandFlagsToDCFlags(GetCommandFlags(cmd->cmd))).Succeeded()
			/* Test for adjacent station or station below selection.
			 * If adjacent-stations is disabled and we are building
			 * next to a station, do not show the selection window
			 * but join the other station immediately. */
			&& !FindStationInArea (ta, waypoint)) {
		std::vector<StationID> list;
		FindStationsNearby (&list, ta, false, waypoint);
		if (list.size() == 0 ? _settings_game.station.distant_join_stations : _settings_game.station.adjacent_stations) {
			if (!_settings_client.gui.persistent_buildingtools) ResetPointerMode();
			new SelectStationWindow (&_select_station_desc, *cmd, ta, waypoint, list);
			return;
		}
	}

	cmd->execp();
}
