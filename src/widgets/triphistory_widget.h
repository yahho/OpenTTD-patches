/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file triphistory_widget.h Types related to the trip history window widgets. */

#ifndef WIDGETS_TRIPHISTORY_WIDGET_H
#define WIDGETS_TRIPHISTORY_WIDGET_H

enum VehicleTripWidgets {
	WID_VTH_CAPTION,

	// Labels
	WID_VTH_LABEL_STATION,		//Station Column label
	WID_VTH_LABEL_DATERECEIVED,	//Date column label
	WID_VTH_LABEL_PROFIT,		//Profit column label
	WID_VTH_LABEL_TIMETAKEN,	//Trip time column label
	WID_VTH_LABEL_LATE,		//Lateness column label

	// Data rows
	WID_VTH_MATRIX_STATION,		//Station Data
	WID_VTH_MATRIX_RECEIVED,	//Date Received Data
	WID_VTH_MATRIX_PROFIT,		//Profit Data
	WID_VTH_MATRIX_TIMETAKEN,	//Trip Time Data
	WID_VTH_MATRIX_LATE,		//Lateness Data

	WID_VTH_SCROLLBAR,		//Scroll Bar
};

#endif /* WIDGETS_TRIPHISTORY_WIDGET_H */
