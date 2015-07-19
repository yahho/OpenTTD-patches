/** @file triphistory_gui.cpp */

#include "stdafx.h"
#include "triphistory.h"
#include "strings_func.h"
#include "window_func.h"
#include "gfx_func.h"
#include "date_func.h"
#include "vehicle_base.h"
#include "table/strings.h"
#include "widgets/triphistory_widget.h"
#include "settings_type.h"

#define TRIP_LENGTH_VISIBLE 10

static const NWidgetPart _vehicle_trip_history_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, WID_VTH_CAPTION), SetDataTip(STR_TRIP_HISTORY_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_VTH_LABEL_STATION), SetMinimalSize(110, 0), SetMinimalTextLines(1, 2), SetResize(1, 0), SetFill(1, 0),
					SetDataTip(STR_TRIP_HISTORY_STATION_LABEL, STR_TRIP_HISTORY_STATION_LABEL_TIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_VTH_LABEL_DATERECEIVED), SetMinimalSize(110, 0), SetMinimalTextLines(1, 2), SetResize(1, 0), SetFill(1, 0),
				SetDataTip(STR_TRIP_HISTORY_RECEIVED_LABEL, STR_TRIP_HISTORY_RECEIVED_LABEL_TIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_VTH_LABEL_PROFIT), SetMinimalSize(110, 0), SetMinimalTextLines(1, 2), SetResize(1, 0), SetFill(1, 0),
					SetDataTip(STR_TRIP_HISTORY_PROFIT_LABEL,       STR_TRIP_HISTORY_PROFIT_LABEL_TIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_VTH_LABEL_TIMETAKEN), SetMinimalSize(50, 0), SetMinimalTextLines(1, 2), SetResize(1, 0),SetFill(1, 0),
					SetDataTip(STR_TRIP_HISTORY_TRIP_TIME_LABEL,    STR_TRIP_HISTORY_TRIP_TIME_LABEL_TIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_VTH_LABEL_LATE), SetMinimalSize(70, 0), SetMinimalTextLines(1, 2), SetResize(1, 0),SetFill(1, 0),
					SetDataTip(STR_TRIP_HISTORY_LATE_LABEL, STR_TRIP_HISTORY_LATE_LABEL_TIP),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_MATRIX, COLOUR_GREY, WID_VTH_MATRIX_STATION), SetMinimalSize(110, 0), SetDataTip((10 << MAT_ROW_START) | (1 << MAT_COL_START), STR_NULL), SetResize(1, 1),SetFill(1, 0),
				NWidget(WWT_MATRIX, COLOUR_GREY, WID_VTH_MATRIX_RECEIVED), SetMinimalSize(110, 0), SetDataTip((10 << MAT_ROW_START) | (1 << MAT_COL_START), STR_NULL), SetResize(1, 1),SetFill(1, 0),
				NWidget(WWT_MATRIX, COLOUR_GREY, WID_VTH_MATRIX_PROFIT), SetMinimalSize(50, 0), SetDataTip((10 << MAT_ROW_START) | (1 << MAT_COL_START), STR_NULL), SetResize(1, 1),SetFill(1, 0),
				NWidget(WWT_MATRIX, COLOUR_GREY, WID_VTH_MATRIX_TIMETAKEN), SetMinimalSize(70, 0), SetDataTip((10 << MAT_ROW_START) | (1 << MAT_COL_START), STR_NULL), SetResize(1, 1),SetFill(1, 0),
				NWidget(WWT_MATRIX, COLOUR_GREY, WID_VTH_MATRIX_LATE), SetMinimalSize(50, 0), SetDataTip((10 << MAT_ROW_START) | (1 << MAT_COL_START), STR_NULL), SetResize(1, 1),SetFill(1, 0),
			EndContainer(),
		EndContainer(),
		NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_VTH_SCROLLBAR),
	EndContainer(),
};

struct VehicleTripHistoryWindow : Window
{
private:
	Scrollbar *vscroll;

public:
	VehicleTripHistoryWindow(WindowDesc *desc, WindowNumber window_number) :
		Window(desc)
	{
		const Vehicle *v = Vehicle::Get(window_number);
		this->CreateNestedTree(desc);
		
		this->FinishInitNested(window_number);
		this->owner = v->owner;

		this->vscroll = this->GetScrollbar(WID_VTH_SCROLLBAR);
		this->vscroll->SetCount(TRIP_LENGTH);
		this->vscroll->SetStepSize(1);
		this->vscroll->SetCapacity(TRIP_LENGTH_VISIBLE);
		this->vscroll->SetPosition(0);

		InvalidateData();
	}

	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		this->SetDirty();
	}

	virtual void SetStringParameters(int widget) const
	{
		switch (widget) {
			case WID_VTH_CAPTION: SetDParam(0, this->window_number); break;
		}
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		const Vehicle *v = Vehicle::Get(window_number);

		switch (widget) {

			case WID_VTH_LABEL_DATERECEIVED: {
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_RECEIVED_LABEL);
				uint32 max = text_dim.width;

				for(int i = v->trip_history.top + TRIP_LENGTH; i > v->trip_history.top; i--) {
					if(v->trip_history.t[i % TRIP_LENGTH].date > 0) {
						if(_settings_client.gui.time_in_minutes) {
							SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].date * DAY_TICKS + v->trip_history.t[i % TRIP_LENGTH].ticks);
							text_dim = GetStringBoundingBox(STR_JUST_DATE_WALLCLOCK_LONG);
						}else {
							SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].date);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_DATE);
							}
						}
						
						if(max < text_dim.width) max = text_dim.width;
					}
				size->width = max + WD_MATRIX_LEFT + WD_MATRIX_RIGHT + 10;
				}
				break;

			case WID_VTH_MATRIX_RECEIVED: {
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_RECEIVED_LABEL);
				uint32 max = text_dim.width;

				for(int i = v->trip_history.top + TRIP_LENGTH; i > v->trip_history.top; i--) {
					if(v->trip_history.t[i % TRIP_LENGTH].date > 0) {
						if(_settings_client.gui.time_in_minutes) {
							SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].date * DAY_TICKS + v->trip_history.t[i % TRIP_LENGTH].ticks);
							text_dim = GetStringBoundingBox(STR_JUST_DATE_WALLCLOCK_LONG);
						}else {
							SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].date);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_DATE);
							}
						}
						
						if(max < text_dim.width) max = text_dim.width;
					}
				size->width = max + WD_MATRIX_LEFT + WD_MATRIX_RIGHT + 10;
				resize->height = FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM;
				size->height = 10 * (FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM);//* resize->height;
				}
				break;

			case WID_VTH_LABEL_PROFIT: {
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_PROFIT_LABEL);
				uint32 max = text_dim.width;

				for(int i = v->trip_history.top + TRIP_LENGTH; i > v->trip_history.top; i--) {
					if(v->trip_history.t[i % TRIP_LENGTH].date > 0) {
						if(v->trip_history.t[i % TRIP_LENGTH].profit == 0)
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_PROFIT_NONE);
						else if(v->trip_history.t[i % TRIP_LENGTH].profit > 0) {
							SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].profit);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_PROFIT);
						} else {
							SetDParam(0, -v->trip_history.t[i % TRIP_LENGTH].profit);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_VIRTUAL_PROFIT);
							}
						}
						
						if(max < text_dim.width) max = text_dim.width;
					}
				size->width = max + WD_MATRIX_LEFT + WD_MATRIX_RIGHT + 10;
				}
				break;

			case WID_VTH_MATRIX_PROFIT: {
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_PROFIT_LABEL);
				uint32 max = text_dim.width;

				for(int i = v->trip_history.top + TRIP_LENGTH; i > v->trip_history.top; i--) {
					if(v->trip_history.t[i % TRIP_LENGTH].date > 0) {
						if(v->trip_history.t[i % TRIP_LENGTH].profit == 0)
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_PROFIT_NONE);
						else if(v->trip_history.t[i % TRIP_LENGTH].profit > 0) {
							SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].profit);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_PROFIT);
						} else {
							SetDParam(0, -v->trip_history.t[i % TRIP_LENGTH].profit);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_VIRTUAL_PROFIT);
							}
						}
						
						if(max < text_dim.width) max = text_dim.width;
					}
				size->width = max + WD_MATRIX_LEFT + WD_MATRIX_RIGHT + 10;
				resize->height = FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM;
				size->height = 10 * (FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM);//resize->height;
				}
				break;

			case WID_VTH_LABEL_TIMETAKEN: {
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_TRIP_TIME_LABEL);
				uint32 max = text_dim.width;

				for(int i = v->trip_history.top + TRIP_LENGTH; i > v->trip_history.top; i--) {
					if(v->trip_history.t[i % TRIP_LENGTH].date > 0) {
						if(_settings_client.gui.time_in_minutes) {
							SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].trip_time / _settings_client.gui.ticks_per_minute / 60);
							SetDParam(1, v->trip_history.t[i % TRIP_LENGTH].trip_time / _settings_client.gui.ticks_per_minute % 60);
							SetDParam(2, v->trip_history.t[i % TRIP_LENGTH].trip_time / _settings_client.gui.ticks_per_minute);
							SetDParam(3, v->trip_history.t[i % TRIP_LENGTH].trip_time / DAY_TICKS);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_DATETIME);
						} else {
							SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].trip_time / DATE_UNIT_SIZE);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_ONLYDATE);
							}
						}
						
						if(max < text_dim.width) max = text_dim.width;
					}
				size->width = max + WD_MATRIX_LEFT + WD_MATRIX_RIGHT + 10;
				}
				break;

			case WID_VTH_MATRIX_TIMETAKEN: {
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_TRIP_TIME_LABEL);
				uint32 max = text_dim.width;

				for(int i = v->trip_history.top + TRIP_LENGTH; i > v->trip_history.top; i--) {
					if(v->trip_history.t[i % TRIP_LENGTH].date > 0) {
						if(_settings_client.gui.time_in_minutes) {
							SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].trip_time / _settings_client.gui.ticks_per_minute / 60);
							SetDParam(1, v->trip_history.t[i % TRIP_LENGTH].trip_time / _settings_client.gui.ticks_per_minute % 60);
							SetDParam(2, v->trip_history.t[i % TRIP_LENGTH].trip_time / _settings_client.gui.ticks_per_minute);
							SetDParam(3, v->trip_history.t[i % TRIP_LENGTH].trip_time / DAY_TICKS);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_DATETIME);
						} else {
							SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].trip_time / DATE_UNIT_SIZE);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_ONLYDATE);
							}
						}
						
						if(max < text_dim.width) max = text_dim.width;
					}
				size->width = max + WD_MATRIX_LEFT + WD_MATRIX_RIGHT + 10;
				resize->height = FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM;
				size->height = 10 * (FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM);//resize->height;
				} break;

			case WID_VTH_LABEL_LATE: {
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_LATE_LABEL);
				uint32 max = text_dim.width;

				for(int i = v->trip_history.top + TRIP_LENGTH; i > v->trip_history.top; i--) {
					if(v->trip_history.t[i % TRIP_LENGTH].date > 0) {
						if(_settings_client.gui.time_in_minutes) {
							if(v->trip_history.t[i % TRIP_LENGTH].late / DATE_UNIT_SIZE == 0)
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_ONTIME);
							else if( v->trip_history.t[i % TRIP_LENGTH].late > 0) {
								SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].late / _settings_client.gui.ticks_per_minute);
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_LATE_MIN);
							} else {
								SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].late / _settings_client.gui.ticks_per_minute);
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_NOTLATE_MIN);
							}
						} else {
							if(v->trip_history.t[i % TRIP_LENGTH].late / DATE_UNIT_SIZE == 0)
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_ONTIME);
							else if( v->trip_history.t[i % TRIP_LENGTH].late > 0) {
								SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].late / DAY_TICKS);
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_LATE_DAYS);
							} else {
								SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].late / DAY_TICKS);
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_NOTLATE_DAYS);
							}
							}
						}
						
						if(max < text_dim.width) max = text_dim.width;
					}
				size->width = max + WD_MATRIX_LEFT + WD_MATRIX_RIGHT + 10;
				}
				break;

			case WID_VTH_MATRIX_LATE: {
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_LATE_LABEL);
				uint32 max = text_dim.width;

				for(int i = v->trip_history.top + TRIP_LENGTH; i > v->trip_history.top; i--) {
					if(v->trip_history.t[i % TRIP_LENGTH].date > 0) {
						if(_settings_client.gui.time_in_minutes) {
							if(v->trip_history.t[i % TRIP_LENGTH].late / DATE_UNIT_SIZE == 0)
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_ONTIME);
							else if( v->trip_history.t[i % TRIP_LENGTH].late > 0) {
								SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].late / _settings_client.gui.ticks_per_minute);
								//SetDParam(1, v->trip_history.t[i % TRIP_LENGTH].late);
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_LATE_MIN);
							} else {
								SetDParam(0, -v->trip_history.t[i % TRIP_LENGTH].late / _settings_client.gui.ticks_per_minute);
								//SetDParam(1, -v->trip_history.t[i % TRIP_LENGTH].late);
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_NOTLATE_MIN);
							}
						} else {
							if(v->trip_history.t[i % TRIP_LENGTH].late / DATE_UNIT_SIZE == 0)
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_ONTIME);
							else if( v->trip_history.t[i % TRIP_LENGTH].late > 0) {
								SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].late / DAY_TICKS);
								//SetDParam(1, v->trip_history.t[i % TRIP_LENGTH].late);
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_LATE_DAYS);
							} else {
								SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].late / DAY_TICKS);
								//SetDParam(1, v->trip_history.t[i % TRIP_LENGTH].late);
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_NOTLATE_DAYS);
							}
							}
						}
						
						if(max < text_dim.width) max = text_dim.width;
					}
				size->width = max + WD_MATRIX_LEFT + WD_MATRIX_RIGHT + 10;
				resize->height = FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM;
				size->height = 10 * (FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM);//resize->height;
				}
				break;

			case WID_VTH_LABEL_STATION: {
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_STATION_LABEL);
				uint32 max = text_dim.width;

				for(int i = v->trip_history.top + TRIP_LENGTH; i > v->trip_history.top; i--) {
					if(v->trip_history.t[i % TRIP_LENGTH].date > 0) {
						if(v->trip_history.t[i % TRIP_LENGTH].station_type == ST_DEPOT) {
							SetDParam(0, v->type);
							SetDParam(1, v->trip_history.t[i % TRIP_LENGTH].station);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_DEPOT);
						} else if(v->trip_history.t[i % TRIP_LENGTH].station_type == ST_STATION){
							if(v->trip_history.t[i % TRIP_LENGTH].live) {
								SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].station);
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_STATION_LIVE);
							} else {
								SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].station);
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_STATION);
								}
						} else {
							SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].station);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_WAYPOINT);
							}
						}
						
						if(max < text_dim.width) max = text_dim.width;
					}
				size->width = max + WD_MATRIX_LEFT + WD_MATRIX_RIGHT + 10;
				}
				break;

			case WID_VTH_MATRIX_STATION: {
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_STATION_LABEL);
				uint32 max = text_dim.width;

				for(int i = v->trip_history.top + TRIP_LENGTH; i > v->trip_history.top; i--) {
					if(v->trip_history.t[i % TRIP_LENGTH].date > 0) {
						if(v->trip_history.t[i % TRIP_LENGTH].station_type == ST_DEPOT) {
							SetDParam(0, v->type);
							SetDParam(1, v->trip_history.t[i % TRIP_LENGTH].station);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_DEPOT);
						} else if(v->trip_history.t[i % TRIP_LENGTH].station_type == ST_STATION){
							if(v->trip_history.t[i % TRIP_LENGTH].live) {
								SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].station);
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_STATION_LIVE);
							} else {
								SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].station);
								text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_STATION);
								}
						} else {
							SetDParam(0, v->trip_history.t[i % TRIP_LENGTH].station);
							text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_WAYPOINT);
							}
						}

						if(max < text_dim.width) max = text_dim.width;
					}
				size->width = max + WD_MATRIX_LEFT + WD_MATRIX_RIGHT + 10;
				resize->height = FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM;
				size->height = 10 * (FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM);//resize->height;
				}
				break;
		}
	}

	virtual void DrawWidget(const Rect &r, int widget) const {
		const Vehicle *v = Vehicle::Get(this->window_number);
		int y = WD_FRAMERECT_TOP;
		int pos = vscroll->GetPosition();


		switch( widget ) {
			case WID_VTH_MATRIX_RECEIVED: {
				y = WD_MATRIX_TOP;
				for(int i = 0; i < TRIP_LENGTH_VISIBLE; i++) {
					int j = v->trip_history.top + TRIP_LENGTH - pos - i;
					j %= TRIP_LENGTH;

					if(v->trip_history.t[j].date > 0) {
						if(_settings_client.gui.time_in_minutes) {
							SetDParam(0, v->trip_history.t[j].date * DAY_TICKS + v->trip_history.t[j].ticks);
							DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_JUST_DATE_WALLCLOCK_LONG, TC_BLACK, SA_HOR_CENTER);
						}else {
							SetDParam(0, v->trip_history.t[j].date);
							DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_DATE, TC_BLACK, SA_HOR_CENTER);
							}
						}

					y += FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM;
					}
				}
				break;

			case WID_VTH_MATRIX_PROFIT: {
				y = WD_MATRIX_TOP;
				for(int i = 0; i < TRIP_LENGTH_VISIBLE; i++) {
					int j = v->trip_history.top + TRIP_LENGTH - pos - i;
					j %= TRIP_LENGTH;

					if(v->trip_history.t[j].date > 0) {
						if(v->trip_history.t[j].profit == 0)
							DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_PROFIT_NONE, TC_BLACK, SA_HOR_CENTER);
						else if(v->trip_history.t[j].profit > 0) {
							SetDParam(0, v->trip_history.t[j].profit);
							DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_PROFIT, TC_BLACK, SA_HOR_CENTER);
						} else {
							SetDParam(0, -v->trip_history.t[j].profit);
							DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_VIRTUAL_PROFIT, TC_BLACK, SA_HOR_CENTER);
							}
						}

					y += FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM;
					}
				}
				break;

			case WID_VTH_MATRIX_STATION: {
				y = WD_MATRIX_TOP;
				for(int i = 0; i < TRIP_LENGTH_VISIBLE; i++) {
					int j = v->trip_history.top + TRIP_LENGTH - pos - i;
					j %= TRIP_LENGTH;

					if(v->trip_history.t[j].date > 0) {
						if(v->trip_history.t[j].station_type == ST_DEPOT) {
							SetDParam(0, v->type);
							SetDParam(1, v->trip_history.t[j].station);
							DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_DEPOT);
						} else if(v->trip_history.t[j].station_type == ST_STATION){
							if(v->trip_history.t[j].live) {
								SetDParam(0, v->trip_history.t[j].station);
								DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_STATION_LIVE);
							} else {
								SetDParam(0, v->trip_history.t[j].station);
								DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_STATION);
								}
						} else {
							SetDParam(0, v->trip_history.t[j].station);
							DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_WAYPOINT);
							}
						}

					y += FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM;
					}
				}
				break;

			case WID_VTH_MATRIX_TIMETAKEN: {
				y = WD_MATRIX_TOP;
				for(int i = 0; i < TRIP_LENGTH_VISIBLE; i++) {
					int j = v->trip_history.top + TRIP_LENGTH - pos - i;
					j %= TRIP_LENGTH;

					if(v->trip_history.t[j].date > 0) {
						if(_settings_client.gui.time_in_minutes) {
							SetDParam(0, v->trip_history.t[j].trip_time / _settings_client.gui.ticks_per_minute / 60);
							SetDParam(1, v->trip_history.t[j].trip_time / _settings_client.gui.ticks_per_minute % 60);
							SetDParam(2, v->trip_history.t[j].trip_time / _settings_client.gui.ticks_per_minute);
							SetDParam(3, v->trip_history.t[j].trip_time / DAY_TICKS);
							DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_DATETIME, TC_BLACK, SA_HOR_CENTER);
						} else {
							SetDParam(0, v->trip_history.t[j].trip_time / DATE_UNIT_SIZE);
							DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_ONLYDATE, TC_BLACK, SA_HOR_CENTER);
							}
						}

					y += FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM;
					}
				}
				break;

			case WID_VTH_MATRIX_LATE: {
				y = WD_MATRIX_TOP;
				for(int i = 0; i < TRIP_LENGTH_VISIBLE; i++) {
					int j = v->trip_history.top + TRIP_LENGTH - pos - i;
					j %= TRIP_LENGTH;

					if(v->trip_history.t[j].date > 0) {
						if(_settings_client.gui.time_in_minutes) {
							if(v->trip_history.t[j].late / DATE_UNIT_SIZE == 0)
								DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_ONTIME, TC_BLACK, SA_HOR_CENTER);
							else if( v->trip_history.t[j].late > 0) {
								SetDParam(0, v->trip_history.t[j].late / _settings_client.gui.ticks_per_minute);
								//SetDParam(1, v->trip_history.t[j].late);
								DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_LATE_MIN, TC_BLACK, SA_HOR_CENTER);
							} else {
								SetDParam(0, -v->trip_history.t[j].late / _settings_client.gui.ticks_per_minute);
								//SetDParam(1, -v->trip_history.t[j].late);
								DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_NOTLATE_MIN, TC_BLACK, SA_HOR_CENTER);
							}
						} else {
							if(v->trip_history.t[j].late / DATE_UNIT_SIZE == 0)
								DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_ONTIME, TC_BLACK, SA_HOR_CENTER);
							else if( v->trip_history.t[j].late > 0) {
								SetDParam(0, v->trip_history.t[j].late / DAY_TICKS);
								//SetDParam(1, v->trip_history.t[j].late);
								DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_LATE_DAYS, TC_BLACK, SA_HOR_CENTER);
							} else {
								SetDParam(0, -v->trip_history.t[j].late / DAY_TICKS);
								//SetDParam(1, -v->trip_history.t[j].late);
								DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_NOTLATE_DAYS, TC_BLACK, SA_HOR_CENTER);
							}
							}
						}

					y += FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM;
					}
				}
				break;
		}
	}
};

static WindowDesc _vehicle_trip_history(
	WDP_AUTO, "trip_history", 100, 50,
	WC_VEHICLE_TRIP_HISTORY,WC_VEHICLE_DETAILS,
	0,
	_vehicle_trip_history_widgets,
	lengthof(_vehicle_trip_history_widgets)
);

void ShowTripHistoryWindow(const Vehicle *v)
{
	if (!BringWindowToFrontById(WC_VEHICLE_TRIP_HISTORY, v->index)) {
		AllocateWindowDescFront<VehicleTripHistoryWindow>(&_vehicle_trip_history, v->index);
	}
}
