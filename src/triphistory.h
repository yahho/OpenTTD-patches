/** @file triphistory.h */

#ifndef TRIPHISTORY_H
#define TRIPHISTORY_H

#include <deque>
#include "window_gui.h"
#include "strings_type.h"
#include "economy_type.h"
#include "date_type.h"
#include "station_type.h"

// entries to save
#define TRIP_LENGTH 30

enum StType {
	ST_STATION,
	ST_WAYPOINT,
	ST_DEPOT,
};

struct TripHistoryEntry {
	Money profit; // Saved
	Date date; // Saved
	StationID station; //Saved
	uint16 ticks; //Saved
	int32 late; //Saved
	uint32 trip_time; //Saved
	byte station_type; //Saved
	bool live;  //Saved

	TripHistoryEntry():
		profit(0),
		date(0),
		station(INVALID_STATION),
		ticks(0),
		late(0),
		trip_time(0),
		station_type(ST_STATION),
		live(false)
		{ };
};

/** Structure to hold data for each vehicle */
struct TripHistory {
	// a lot of saveload stuff for std::deque. So...
	TripHistoryEntry t[ TRIP_LENGTH ];
	byte top;

	TripHistory( ) : top(0) {}

	void AddValue(Date dvalue, uint16 ticks, StationID station, uint32 trip_time, byte station_type, bool live);
	void AddProfit(Money mvalue);
	void AddLateness(int32 late);
};

#endif /* TRIPHISTORY_H */
