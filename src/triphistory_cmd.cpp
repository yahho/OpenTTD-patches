/** @file triphistory_cmd.cpp */

#include "stdafx.h"
#include "triphistory.h"
#include "table/strings.h"

void TripHistory::AddValue(Date dvalue, uint16 ticks, StationID station, uint32 trip_time, byte station_type, bool live)
{
	top++;
	if(top == TRIP_LENGTH)
		top = 0;

	t[top].profit = 0;
	t[top].date = dvalue;
	t[top].station = station;
	t[top].ticks = ticks;
	t[top].late = 0;
	t[top].trip_time = trip_time;
	t[top].station_type = station_type;
	t[top].live = live;
}

void TripHistory::AddProfit(Money mvalue)
{
	t[top].profit = mvalue;
}

void TripHistory::AddLateness(int32 late)
{
	t[top].late = late;
}