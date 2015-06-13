/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file news_func.h Functions related to news. */

#ifndef NEWS_FUNC_H
#define NEWS_FUNC_H

#include "news_type.h"
#include "vehicle_type.h"
#include "station_type.h"
#include "industry_type.h"

void InsertNewsItem (NewsItem *ni);

template <class T, typename T1>
void AddNewsItem (T1 t1)
{
	if (_game_mode == GM_MENU) return;
	InsertNewsItem (new T (t1));
}

template <class T, typename T1, typename T2>
void AddNewsItem (T1 t1, T2 t2)
{
	if (_game_mode == GM_MENU) return;
	InsertNewsItem (new T (t1, t2));
}

template <class T, typename T1, typename T2, typename T3>
void AddNewsItem (T1 t1, T2 t2, T3 t3)
{
	if (_game_mode == GM_MENU) return;
	InsertNewsItem (new T (t1, t2, t3));
}

template <class T, typename T1, typename T2, typename T3, typename T4>
void AddNewsItem (T1 t1, T2 t2, T3 t3, T4 t4)
{
	if (_game_mode == GM_MENU) return;
	InsertNewsItem (new T (t1, t2, t3, t4));
}

template <class T, typename T1, typename T2, typename T3, typename T4, typename T5>
void AddNewsItem (T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
{
	if (_game_mode == GM_MENU) return;
	InsertNewsItem (new T (t1, t2, t3, t4, t5));
}

template <class T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
void AddNewsItem (T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
{
	if (_game_mode == GM_MENU) return;
	InsertNewsItem (new T (t1, t2, t3, t4, t5, t6));
}

void NewsLoop();
void InitNewsItemStructs();

extern const NewsItem *_statusbar_news_item;

void DeleteInvalidEngineNews();
void DeleteVehicleNews(VehicleID vid, StringID news);
void DeleteStationNews(StationID sid);
void DeleteIndustryNews(IndustryID iid);

#endif /* NEWS_FUNC_H */
