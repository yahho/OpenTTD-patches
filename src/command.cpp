/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file command.cpp Handling of commands. */

#include "stdafx.h"
#include "landscape.h"
#include "error.h"
#include "gui.h"
#include "command_func.h"
#include "network/network_type.h"
#include "network/network.h"
#include "genworld.h"
#include "strings_func.h"
#include "texteff.hpp"
#include "town.h"
#include "date_func.h"
#include "company_func.h"
#include "company_base.h"
#include "signal_func.h"
#include "core/backup_type.hpp"
#include "object_base.h"
#include "ai/ai_instance.hpp"
#include "game/game_instance.hpp"

#include "table/strings.h"

CommandProc CmdBuildRailroadTrack;
CommandProc CmdRemoveRailroadTrack;
CommandProc CmdBuildSingleRail;
CommandProc CmdRemoveSingleRail;

CommandProc CmdLandscapeClear;

CommandProc CmdBuildBridge;

CommandProc CmdBuildRailStation;
CommandProc CmdRemoveFromRailStation;
CommandProc CmdConvertRail;

CommandProc CmdBuildSingleSignal;
CommandProc CmdRemoveSingleSignal;

CommandProc CmdTerraformLand;

CommandProc CmdBuildObject;
CommandProc CmdSellLandArea;

CommandProc CmdBuildTunnel;

CommandProc CmdBuildTrainDepot;
CommandProc CmdBuildRailWaypoint;
CommandProc CmdRenameWaypoint;
CommandProc CmdRemoveFromRailWaypoint;

CommandProc CmdBuildRoadStop;
CommandProc CmdRemoveRoadStop;

CommandProc CmdBuildLongRoad;
CommandProc CmdRemoveLongRoad;
CommandProc CmdBuildRoad;

CommandProc CmdBuildRoadDepot;

CommandProc CmdBuildAirport;

CommandProc CmdBuildDock;

CommandProc CmdBuildShipDepot;

CommandProc CmdBuildBuoy;

CommandProc CmdPlantTree;

CommandProc CmdMoveRailVehicle;

CommandProc CmdBuildVehicle;
CommandProc CmdSellVehicle;
CommandProc CmdRefitVehicle;
CommandProc CmdSendVehicleToDepot;
CommandProc CmdSetVehicleVisibility;

CommandProc CmdForceTrainProceed;
CommandProc CmdReverseTrainDirection;

CommandProc CmdClearOrderBackup;
CommandProc CmdModifyOrder;
CommandProc CmdSkipToOrder;
CommandProc CmdDeleteOrder;
CommandProc CmdInsertOrder;
CommandProc CmdChangeServiceInt;

CommandProc CmdBuildIndustry;

CommandProc CmdSetCompanyManagerFace;
CommandProc CmdSetCompanyColour;

CommandProc CmdIncreaseLoan;
CommandProc CmdDecreaseLoan;

CommandProc CmdWantEnginePreview;

CommandProc CmdRenameVehicle;
CommandProc CmdRenameEngine;

CommandProc CmdRenameCompany;
CommandProc CmdRenamePresident;

CommandProc CmdRenameStation;
CommandProc CmdRenameDepot;

CommandProc CmdPlaceSign;
CommandProc CmdRenameSign;

CommandProc CmdTurnRoadVeh;

CommandProc CmdPause;

CommandProc CmdBuyShareInCompany;
CommandProc CmdSellShareInCompany;
CommandProc CmdBuyCompany;

CommandProc CmdFoundTown;
CommandProc CmdRenameTown;
CommandProc CmdDoTownAction;
CommandProc CmdTownGrowthRate;
CommandProc CmdTownCargoGoal;
CommandProc CmdTownSetText;
CommandProc CmdExpandTown;
CommandProc CmdDeleteTown;

CommandProc CmdChangeSetting;
CommandProc CmdChangeCompanySetting;

CommandProc CmdOrderRefit;
CommandProc CmdCloneOrder;

CommandProc CmdClearArea;

CommandProc CmdGiveMoney;
CommandProc CmdMoneyCheat;
CommandProc CmdChangeBankBalance;
CommandProc CmdBuildCanal;
CommandProc CmdBuildLock;

CommandProc CmdCreateSubsidy;
CommandProc CmdCompanyCtrl;
CommandProc CmdCustomNewsItem;
CommandProc CmdCreateGoal;
CommandProc CmdRemoveGoal;
CommandProc CmdSetGoalText;
CommandProc CmdSetGoalProgress;
CommandProc CmdSetGoalCompleted;
CommandProc CmdGoalQuestion;
CommandProc CmdGoalQuestionAnswer;
CommandProc CmdCreateStoryPage;
CommandProc CmdCreateStoryPageElement;
CommandProc CmdUpdateStoryPageElement;
CommandProc CmdSetStoryPageTitle;
CommandProc CmdSetStoryPageDate;
CommandProc CmdShowStoryPage;
CommandProc CmdRemoveStoryPage;
CommandProc CmdRemoveStoryPageElement;

CommandProc CmdLevelLand;

CommandProc CmdBuildSignalTrack;
CommandProc CmdRemoveSignalTrack;

CommandProc CmdSetAutoReplace;

CommandProc CmdCloneVehicle;
CommandProc CmdStartStopVehicle;
CommandProc CmdMassStartStopVehicle;
CommandProc CmdAutoreplaceVehicle;
CommandProc CmdDepotSellAllVehicles;
CommandProc CmdDepotMassAutoReplace;

CommandProc CmdCreateGroup;
CommandProc CmdAlterGroup;
CommandProc CmdDeleteGroup;
CommandProc CmdAddVehicleGroup;
CommandProc CmdAddSharedVehicleGroup;
CommandProc CmdRemoveAllVehiclesGroup;
CommandProc CmdSetGroupReplaceProtection;

CommandProc CmdMoveOrder;
CommandProc CmdChangeTimetable;
CommandProc CmdSetVehicleOnTime;
CommandProc CmdAutofillTimetable;
CommandProc CmdSetTimetableStart;

CommandProc CmdOpenCloseAirport;

template <StringID str>
StringID GetErrConstant (TileIndex tile, uint32 p1, uint32 p2, const char *text)
{
	return str;
}

CommandErrstrF GetErrTerraformLand;
CommandErrstrF GetErrLevelLand;
CommandErrstrF GetErrRenameSign;

CommandErrstrF GetErrBuildSingleRail;
CommandErrstrF GetErrBuildSignals;

CommandErrstrF GetErrBuildRoadStop;
CommandErrstrF GetErrRemoveRoadStop;
CommandErrstrF GetErrBuildRoad;
CommandErrstrF GetErrRemoveRoad;
CommandErrstrF GetErrBuildRoadDepot;

CommandErrstrF GetErrBuildCanal;

CommandErrstrF GetErrBuildBridge;
CommandErrstrF GetErrBuildObject;

CommandErrstrF GetErrRenameEngine;

CommandErrstrF GetErrBuildVehicle;
CommandErrstrF GetErrSellVehicle;
CommandErrstrF GetErrRefitVehicle;
CommandErrstrF GetErrCloneVehicle;
CommandErrstrF GetErrRenameVehicle;
CommandErrstrF GetErrStartStopVehicle;
CommandErrstrF GetErrSendVehicleToDepot;
CommandErrstrF GetErrMoveRailVehicle;
CommandErrstrF GetErrReverseTrain;

CommandErrstrF GetErrCloneOrder;
CommandErrstrF GetErrSkipToOrder;

CommandErrstrF GetErrAlterGroup;

CommandErrstrF GetErrFoundTown;

#define DEF_CMD(proc, flags, type, callback, errorf) {proc, #proc, (CommandFlags)flags, type, callback, errorf}

/**
 * The master command table
 *
 * This table contains all possible CommandProc functions with
 * the flags which belongs to it. The indices are the same
 * as the value from the CMD_* enums.
 */
static const CommandClass _command_proc_table[] = {
	DEF_CMD(CmdBuildRailroadTrack,         CMDF_NO_WATER | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, NULL,               GetErrConstant<STR_ERROR_CAN_T_BUILD_RAILROAD_TRACK>),      // CMD_BUILD_RAILROAD_TRACK
	DEF_CMD(CmdRemoveRailroadTrack,                        CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, NULL,               GetErrConstant<STR_ERROR_CAN_T_REMOVE_RAILROAD_TRACK>),     // CMD_REMOVE_RAILROAD_TRACK
	DEF_CMD(CmdBuildSingleRail,            CMDF_NO_WATER | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcSingleRail,       GetErrBuildSingleRail),                                     // CMD_BUILD_SINGLE_RAIL
	DEF_CMD(CmdRemoveSingleRail,                           CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcSingleRail,       GetErrConstant<STR_ERROR_CAN_T_REMOVE_RAILROAD_TRACK>),     // CMD_REMOVE_SINGLE_RAIL
	DEF_CMD(CmdLandscapeClear,                                     0, CMDT_LANDSCAPE_CONSTRUCTION, NULL,               NULL),                                                      // CMD_LANDSCAPE_CLEAR
	DEF_CMD(CmdBuildBridge,   CMDF_DEITY | CMDF_NO_WATER | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcBuildBridge,      GetErrBuildBridge),                                         // CMD_BUILD_BRIDGE
	DEF_CMD(CmdBuildRailStation,           CMDF_NO_WATER | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcStation,          GetErrConstant<STR_ERROR_CAN_T_BUILD_RAILROAD_STATION>),    // CMD_BUILD_RAIL_STATION
	DEF_CMD(CmdBuildTrainDepot,            CMDF_NO_WATER | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcRailDepot,        GetErrConstant<STR_ERROR_CAN_T_BUILD_TRAIN_DEPOT>),         // CMD_BUILD_TRAIN_DEPOT
	DEF_CMD(CmdBuildSingleSignal,                          CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcPlaySound1E,      GetErrBuildSignals),                                        // CMD_BUILD_SIGNALS
	DEF_CMD(CmdRemoveSingleSignal,                         CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcPlaySound1E,      GetErrConstant<STR_ERROR_CAN_T_REMOVE_SIGNALS_FROM>),       // CMD_REMOVE_SIGNALS
	DEF_CMD(CmdTerraformLand,             CMDF_ALL_TILES | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcTerraformLand,    GetErrTerraformLand),                                       // CMD_TERRAFORM_LAND
	DEF_CMD(CmdBuildObject,                CMDF_NO_WATER | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcBuildObject,      GetErrBuildObject),                                         // CMD_BUILD_OBJECT
	DEF_CMD(CmdBuildTunnel,                   CMDF_DEITY | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcBuildTunnel,      GetErrConstant<STR_ERROR_CAN_T_BUILD_TUNNEL_HERE>),         // CMD_BUILD_TUNNEL
	DEF_CMD(CmdRemoveFromRailStation,                              0, CMDT_LANDSCAPE_CONSTRUCTION, CcPlaySound1E,      GetErrConstant<STR_ERROR_CAN_T_REMOVE_PART_OF_STATION>),    // CMD_REMOVE_FROM_RAIL_STATION
	DEF_CMD(CmdConvertRail,                                        0, CMDT_LANDSCAPE_CONSTRUCTION, CcPlaySound10,      GetErrConstant<STR_ERROR_CAN_T_CONVERT_RAIL>),              // CMD_CONVERT_RAIL
	DEF_CMD(CmdBuildRailWaypoint,                                  0, CMDT_LANDSCAPE_CONSTRUCTION, CcPlaySound1E,      GetErrConstant<STR_ERROR_CAN_T_BUILD_TRAIN_WAYPOINT>),      // CMD_BUILD_RAIL_WAYPOINT
	DEF_CMD(CmdRenameWaypoint,                                     0, CMDT_OTHER_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_CHANGE_WAYPOINT_NAME>),      // CMD_RENAME_WAYPOINT
	DEF_CMD(CmdRemoveFromRailWaypoint,                             0, CMDT_LANDSCAPE_CONSTRUCTION, CcPlaySound1E,      GetErrConstant<STR_ERROR_CAN_T_REMOVE_TRAIN_WAYPOINT>),     // CMD_REMOVE_FROM_RAIL_WAYPOINT

	DEF_CMD(CmdBuildRoadStop,              CMDF_NO_WATER | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcRoadStop,         GetErrBuildRoadStop),                                       // CMD_BUILD_ROAD_STOP
	DEF_CMD(CmdRemoveRoadStop,                                     0, CMDT_LANDSCAPE_CONSTRUCTION, CcPlaySound1D,      GetErrRemoveRoadStop),                                      // CMD_REMOVE_ROAD_STOP
	DEF_CMD(CmdBuildLongRoad, CMDF_DEITY | CMDF_NO_WATER | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcPlaySound1D,      GetErrBuildRoad),                                           // CMD_BUILD_LONG_ROAD
	DEF_CMD(CmdRemoveLongRoad,              CMDF_NO_TEST | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcPlaySound1D,      GetErrRemoveRoad),                                          // CMD_REMOVE_LONG_ROAD; towns may disallow removing road bits (as they are connected) in test, but in exec they're removed and thus removing is allowed.
	DEF_CMD(CmdBuildRoad,     CMDF_DEITY | CMDF_NO_WATER | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, NULL,               NULL),                                                      // CMD_BUILD_ROAD
	DEF_CMD(CmdBuildRoadDepot,             CMDF_NO_WATER | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcRoadDepot,        GetErrBuildRoadDepot),                                      // CMD_BUILD_ROAD_DEPOT

	DEF_CMD(CmdBuildAirport,               CMDF_NO_WATER | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcBuildAirport,     GetErrConstant<STR_ERROR_CAN_T_BUILD_AIRPORT_HERE>),        // CMD_BUILD_AIRPORT
	DEF_CMD(CmdBuildDock,                                  CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcBuildDocks,       GetErrConstant<STR_ERROR_CAN_T_BUILD_DOCK_HERE>),           // CMD_BUILD_DOCK
	DEF_CMD(CmdBuildShipDepot,                             CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcBuildDocks,       GetErrConstant<STR_ERROR_CAN_T_BUILD_SHIP_DEPOT>),          // CMD_BUILD_SHIP_DEPOT
	DEF_CMD(CmdBuildBuoy,                                  CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcBuildDocks,       GetErrConstant<STR_ERROR_CAN_T_POSITION_BUOY_HERE>),        // CMD_BUILD_BUOY
	DEF_CMD(CmdPlantTree,                                  CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, NULL,               GetErrConstant<STR_ERROR_CAN_T_PLANT_TREE_HERE>),           // CMD_PLANT_TREE

	DEF_CMD(CmdBuildVehicle,                          CMDF_CLIENT_ID, CMDT_VEHICLE_CONSTRUCTION,   CcBuildVehicle,     GetErrBuildVehicle),                                        // CMD_BUILD_VEHICLE
	DEF_CMD(CmdSellVehicle,                           CMDF_CLIENT_ID, CMDT_VEHICLE_CONSTRUCTION,   NULL,               GetErrSellVehicle),                                         // CMD_SELL_VEHICLE
	DEF_CMD(CmdRefitVehicle,                                       0, CMDT_VEHICLE_CONSTRUCTION,   NULL,               GetErrRefitVehicle),                                        // CMD_REFIT_VEHICLE
	DEF_CMD(CmdSendVehicleToDepot,                                 0, CMDT_VEHICLE_MANAGEMENT,     NULL,               GetErrSendVehicleToDepot),                                  // CMD_SEND_VEHICLE_TO_DEPOT
	DEF_CMD(CmdSetVehicleVisibility,                               0, CMDT_COMPANY_SETTING,        NULL,               NULL),                                                      // CMD_SET_VEHICLE_VISIBILITY

	DEF_CMD(CmdMoveRailVehicle,                                    0, CMDT_VEHICLE_CONSTRUCTION,   NULL,               GetErrMoveRailVehicle),                                     // CMD_MOVE_RAIL_VEHICLE
	DEF_CMD(CmdForceTrainProceed,                                  0, CMDT_VEHICLE_MANAGEMENT,     NULL,               GetErrConstant<STR_ERROR_CAN_T_MAKE_TRAIN_PASS_SIGNAL>),    // CMD_FORCE_TRAIN_PROCEED
	DEF_CMD(CmdReverseTrainDirection,                              0, CMDT_VEHICLE_MANAGEMENT,     NULL,               GetErrReverseTrain),                                        // CMD_REVERSE_TRAIN_DIRECTION

	DEF_CMD(CmdClearOrderBackup,                      CMDF_CLIENT_ID, CMDT_SERVER_SETTING,         NULL,               NULL),                                                      // CMD_CLEAR_ORDER_BACKUP
	DEF_CMD(CmdModifyOrder,                                        0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_MODIFY_THIS_ORDER>),         // CMD_MODIFY_ORDER
	DEF_CMD(CmdSkipToOrder,                                        0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrSkipToOrder),                                         // CMD_SKIP_TO_ORDER
	DEF_CMD(CmdDeleteOrder,                                        0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_DELETE_THIS_ORDER>),         // CMD_DELETE_ORDER
	DEF_CMD(CmdInsertOrder,                                        0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_INSERT_NEW_ORDER>),          // CMD_INSERT_ORDER

	DEF_CMD(CmdChangeServiceInt,                                   0, CMDT_VEHICLE_MANAGEMENT,     NULL,               GetErrConstant<STR_ERROR_CAN_T_CHANGE_SERVICING>),          // CMD_CHANGE_SERVICE_INT

	DEF_CMD(CmdBuildIndustry,                             CMDF_DEITY, CMDT_LANDSCAPE_CONSTRUCTION, CcBuildIndustry,    GetErrConstant<STR_ERROR_CAN_T_CONSTRUCT_THIS_INDUSTRY>),   // CMD_BUILD_INDUSTRY
	DEF_CMD(CmdSetCompanyManagerFace,                              0, CMDT_OTHER_MANAGEMENT,       NULL,               NULL),                                                      // CMD_SET_COMPANY_MANAGER_FACE
	DEF_CMD(CmdSetCompanyColour,                                   0, CMDT_OTHER_MANAGEMENT,       NULL,               NULL),                                                      // CMD_SET_COMPANY_COLOUR

	DEF_CMD(CmdIncreaseLoan,                                       0, CMDT_MONEY_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_BORROW_ANY_MORE_MONEY>),     // CMD_INCREASE_LOAN
	DEF_CMD(CmdDecreaseLoan,                                       0, CMDT_MONEY_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_REPAY_LOAN>),                // CMD_DECREASE_LOAN

	DEF_CMD(CmdWantEnginePreview,                                  0, CMDT_VEHICLE_MANAGEMENT,     NULL,               NULL),                                                      // CMD_WANT_ENGINE_PREVIEW

	DEF_CMD(CmdRenameVehicle,                                      0, CMDT_OTHER_MANAGEMENT,       NULL,               GetErrRenameVehicle),                                       // CMD_RENAME_VEHICLE
	DEF_CMD(CmdRenameEngine,                             CMDF_SERVER, CMDT_OTHER_MANAGEMENT,       NULL,               GetErrRenameEngine),                                        // CMD_RENAME_ENGINE

	DEF_CMD(CmdRenameCompany,                                      0, CMDT_OTHER_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_CHANGE_COMPANY_NAME>),       // CMD_RENAME_COMPANY
	DEF_CMD(CmdRenamePresident,                                    0, CMDT_OTHER_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_CHANGE_PRESIDENT>),          // CMD_RENAME_PRESIDENT

	DEF_CMD(CmdRenameStation,                                      0, CMDT_OTHER_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_RENAME_STATION>),            // CMD_RENAME_STATION
	DEF_CMD(CmdRenameDepot,                                        0, CMDT_OTHER_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_RENAME_DEPOT>),              // CMD_RENAME_DEPOT

	DEF_CMD(CmdPlaceSign,                                 CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       CcPlaceSign,        GetErrConstant<STR_ERROR_CAN_T_PLACE_SIGN_HERE>),           // CMD_PLACE_SIGN
	DEF_CMD(CmdRenameSign,                                CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               GetErrRenameSign),                                          // CMD_RENAME_SIGN

	DEF_CMD(CmdTurnRoadVeh,                                        0, CMDT_VEHICLE_MANAGEMENT,     NULL,               GetErrConstant<STR_ERROR_CAN_T_MAKE_ROAD_VEHICLE_TURN>),    // CMD_TURN_ROADVEH

	DEF_CMD(CmdPause,                                    CMDF_SERVER, CMDT_SERVER_SETTING,         NULL,               NULL),                                                      // CMD_PAUSE

	DEF_CMD(CmdBuyShareInCompany,                                  0, CMDT_MONEY_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_BUY_25_SHARE_IN_THIS>),      // CMD_BUY_SHARE_IN_COMPANY
	DEF_CMD(CmdSellShareInCompany,                                 0, CMDT_MONEY_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_SELL_25_SHARE_IN>),          // CMD_SELL_SHARE_IN_COMPANY
	DEF_CMD(CmdBuyCompany,                                         0, CMDT_MONEY_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_BUY_COMPANY>),               // CMD_BUY_COMPANY

	DEF_CMD(CmdFoundTown,                  CMDF_DEITY | CMDF_NO_TEST, CMDT_LANDSCAPE_CONSTRUCTION, CcFoundTown,        GetErrFoundTown),                                           // CMD_FOUND_TOWN; founding random town can fail only in exec run
	DEF_CMD(CmdRenameTown,                  CMDF_DEITY | CMDF_SERVER, CMDT_OTHER_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_RENAME_TOWN>),               // CMD_RENAME_TOWN
	DEF_CMD(CmdDoTownAction,                                       0, CMDT_LANDSCAPE_CONSTRUCTION, NULL,               GetErrConstant<STR_ERROR_CAN_T_DO_THIS>),                   // CMD_DO_TOWN_ACTION
	DEF_CMD(CmdTownCargoGoal,                             CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_TOWN_CARGO_GOAL
	DEF_CMD(CmdTownGrowthRate,                            CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_TOWN_GROWTH_RATE
	DEF_CMD(CmdTownSetText,               CMDF_STR_CTRL | CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_TOWN_SET_TEXT
	DEF_CMD(CmdExpandTown,                                CMDF_DEITY, CMDT_LANDSCAPE_CONSTRUCTION, NULL,               GetErrConstant<STR_ERROR_CAN_T_EXPAND_TOWN>),               // CMD_EXPAND_TOWN
	DEF_CMD(CmdDeleteTown,                              CMDF_OFFLINE, CMDT_LANDSCAPE_CONSTRUCTION, NULL,               GetErrConstant<STR_ERROR_TOWN_CAN_T_DELETE>),               // CMD_DELETE_TOWN

	DEF_CMD(CmdOrderRefit,                                         0, CMDT_ROUTE_MANAGEMENT,       NULL,               NULL),                                                      // CMD_ORDER_REFIT
	DEF_CMD(CmdCloneOrder,                                         0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrCloneOrder),                                          // CMD_CLONE_ORDER

	DEF_CMD(CmdClearArea,                               CMDF_NO_TEST, CMDT_LANDSCAPE_CONSTRUCTION, CcPlaySound10,      GetErrConstant<STR_ERROR_CAN_T_CLEAR_THIS_AREA>),           // CMD_CLEAR_AREA; destroying multi-tile houses makes town rating differ between test and execution

	DEF_CMD(CmdMoneyCheat,                              CMDF_OFFLINE, CMDT_CHEAT,                  NULL,               NULL),                                                      // CMD_MONEY_CHEAT
	DEF_CMD(CmdChangeBankBalance,                         CMDF_DEITY, CMDT_MONEY_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_CHANGE_BANK_BALANCE
	DEF_CMD(CmdBuildCanal,                                 CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcBuildCanal,       GetErrBuildCanal),                                          // CMD_BUILD_CANAL
	DEF_CMD(CmdCreateSubsidy,                             CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_CREATE_SUBSIDY
	DEF_CMD(CmdCompanyCtrl,          CMDF_SPECTATOR | CMDF_CLIENT_ID, CMDT_SERVER_SETTING,         NULL,               NULL),                                                      // CMD_COMPANY_CTRL
	DEF_CMD(CmdCustomNewsItem,            CMDF_STR_CTRL | CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_CUSTOM_NEWS_ITEM
	DEF_CMD(CmdCreateGoal,                CMDF_STR_CTRL | CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_CREATE_GOAL
	DEF_CMD(CmdRemoveGoal,                                CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_REMOVE_GOAL
	DEF_CMD(CmdSetGoalText,               CMDF_STR_CTRL | CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_SET_GOAL_TEXT
	DEF_CMD(CmdSetGoalProgress,           CMDF_STR_CTRL | CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_SET_GOAL_PROGRESS
	DEF_CMD(CmdSetGoalCompleted,          CMDF_STR_CTRL | CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_SET_GOAL_COMPLETED
	DEF_CMD(CmdGoalQuestion,              CMDF_STR_CTRL | CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_GOAL_QUESTION
	DEF_CMD(CmdGoalQuestionAnswer,                        CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL),                                                      // CMD_GOAL_QUESTION_ANSWER
	DEF_CMD(CmdCreateStoryPage,           CMDF_STR_CTRL | CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_CREATE_STORY_PAGE
	DEF_CMD(CmdCreateStoryPageElement,    CMDF_STR_CTRL | CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_CREATE_STORY_PAGE_ELEMENT
	DEF_CMD(CmdUpdateStoryPageElement,    CMDF_STR_CTRL | CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_UPDATE_STORY_PAGE_ELEMENT
	DEF_CMD(CmdSetStoryPageTitle,         CMDF_STR_CTRL | CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_SET_STORY_PAGE_TITLE
	DEF_CMD(CmdSetStoryPageDate,                          CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_SET_STORY_PAGE_DATE
	DEF_CMD(CmdShowStoryPage,                             CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_SHOW_STORY_PAGE
	DEF_CMD(CmdRemoveStoryPage,                           CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_REMOVE_STORY_PAGE
	DEF_CMD(CmdRemoveStoryPageElement,                    CMDF_DEITY, CMDT_OTHER_MANAGEMENT,       NULL,               NULL /* unused */),                                         // CMD_REMOVE_STORY_PAGE_ELEMENT

	DEF_CMD(CmdLevelLand,  CMDF_ALL_TILES | CMDF_NO_TEST | CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcTerraform,        GetErrLevelLand),                                           // CMD_LEVEL_LAND; test run might clear tiles multiple times, in execution that only happens once

	DEF_CMD(CmdBuildLock,                                  CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcBuildDocks,       GetErrConstant<STR_ERROR_CAN_T_BUILD_LOCKS>),               // CMD_BUILD_LOCK

	DEF_CMD(CmdBuildSignalTrack,                           CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcPlaySound1E,      GetErrConstant<STR_ERROR_CAN_T_BUILD_SIGNALS_HERE>),        // CMD_BUILD_SIGNAL_TRACK
	DEF_CMD(CmdRemoveSignalTrack,                          CMDF_AUTO, CMDT_LANDSCAPE_CONSTRUCTION, CcPlaySound1E,      GetErrConstant<STR_ERROR_CAN_T_REMOVE_SIGNALS_FROM>),       // CMD_REMOVE_SIGNAL_TRACK

	DEF_CMD(CmdGiveMoney,                                          0, CMDT_MONEY_MANAGEMENT,       CcGiveMoney,        GetErrConstant<STR_ERROR_INSUFFICIENT_FUNDS>),              // CMD_GIVE_MONEY
	DEF_CMD(CmdChangeSetting,                            CMDF_SERVER, CMDT_SERVER_SETTING,         NULL,               NULL),                                                      // CMD_CHANGE_SETTING
	DEF_CMD(CmdChangeCompanySetting,                               0, CMDT_COMPANY_SETTING,        NULL,               NULL),                                                      // CMD_CHANGE_COMPANY_SETTING
	DEF_CMD(CmdSetAutoReplace,                                     0, CMDT_VEHICLE_MANAGEMENT,     NULL,               NULL),                                                      // CMD_SET_AUTOREPLACE
	DEF_CMD(CmdCloneVehicle,                            CMDF_NO_TEST, CMDT_VEHICLE_CONSTRUCTION,   CcCloneVehicle,     GetErrCloneVehicle),                                        // CMD_CLONE_VEHICLE; NewGRF callbacks influence building and refitting making it impossible to correctly estimate the cost
	DEF_CMD(CmdStartStopVehicle,                                   0, CMDT_VEHICLE_MANAGEMENT,     CcStartStopVehicle, GetErrStartStopVehicle),                                    // CMD_START_STOP_VEHICLE
	DEF_CMD(CmdMassStartStopVehicle,                               0, CMDT_VEHICLE_MANAGEMENT,     NULL,               NULL),                                                      // CMD_MASS_START_STOP
	DEF_CMD(CmdAutoreplaceVehicle,                                 0, CMDT_VEHICLE_MANAGEMENT,     NULL,               NULL /* unused */),                                         // CMD_AUTOREPLACE_VEHICLE
	DEF_CMD(CmdDepotSellAllVehicles,                               0, CMDT_VEHICLE_CONSTRUCTION,   NULL,               NULL),                                                      // CMD_DEPOT_SELL_ALL_VEHICLES
	DEF_CMD(CmdDepotMassAutoReplace,                               0, CMDT_VEHICLE_CONSTRUCTION,   NULL,               NULL),                                                      // CMD_DEPOT_MASS_AUTOREPLACE
	DEF_CMD(CmdCreateGroup,                                        0, CMDT_ROUTE_MANAGEMENT,       CcCreateGroup,      GetErrConstant<STR_ERROR_GROUP_CAN_T_CREATE>),              // CMD_CREATE_GROUP
	DEF_CMD(CmdDeleteGroup,                                        0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_GROUP_CAN_T_DELETE>),              // CMD_DELETE_GROUP
	DEF_CMD(CmdAlterGroup,                                         0, CMDT_OTHER_MANAGEMENT,       NULL,               GetErrAlterGroup),                                          // CMD_ALTER_GROUP
	DEF_CMD(CmdAddVehicleGroup,                                    0, CMDT_ROUTE_MANAGEMENT,       CcAddVehicleGroup,  GetErrConstant<STR_ERROR_GROUP_CAN_T_ADD_VEHICLE>),         // CMD_ADD_VEHICLE_GROUP
	DEF_CMD(CmdAddSharedVehicleGroup,                              0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_GROUP_CAN_T_ADD_SHARED_VEHICLE>),  // CMD_ADD_SHARED_VEHICLE_GROUP
	DEF_CMD(CmdRemoveAllVehiclesGroup,                             0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_GROUP_CAN_T_REMOVE_ALL_VEHICLES>), // CMD_REMOVE_ALL_VEHICLES_GROUP
	DEF_CMD(CmdSetGroupReplaceProtection,                          0, CMDT_ROUTE_MANAGEMENT,       NULL,               NULL),                                                      // CMD_SET_GROUP_REPLACE_PROTECTION
	DEF_CMD(CmdMoveOrder,                                          0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_MOVE_THIS_ORDER>),           // CMD_MOVE_ORDER
	DEF_CMD(CmdChangeTimetable,                                    0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_TIMETABLE_VEHICLE>),         // CMD_CHANGE_TIMETABLE
	DEF_CMD(CmdSetVehicleOnTime,                                   0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_TIMETABLE_VEHICLE>),         // CMD_SET_VEHICLE_ON_TIME
	DEF_CMD(CmdAutofillTimetable,                                  0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_TIMETABLE_VEHICLE>),         // CMD_AUTOFILL_TIMETABLE
	DEF_CMD(CmdSetTimetableStart,                                  0, CMDT_ROUTE_MANAGEMENT,       NULL,               GetErrConstant<STR_ERROR_CAN_T_TIMETABLE_VEHICLE>),         // CMD_SET_TIMETABLE_START

	DEF_CMD(CmdOpenCloseAirport,                                   0, CMDT_ROUTE_MANAGEMENT,       NULL,               NULL),                                                      // CMD_OPEN_CLOSE_AIRPORT
};

/*!
 * This function range-checks a cmd, and checks if the cmd is not NULL
 *
 * @param cmd The integer value of a command
 * @return true if the command is valid (and got a CommandProc function)
 */
bool IsValidCommand(uint32 cmd)
{
	return cmd < lengthof(_command_proc_table) && _command_proc_table[cmd].proc != NULL;
}

/*!
 * This function returns the flags which belongs to the given command.
 *
 * @param cmd The integer value of the command
 * @return The flags for this command
 */
CommandFlags GetCommandFlags(uint32 cmd)
{
	assert(IsValidCommand(cmd));

	return _command_proc_table[cmd].flags;
}

/*!
 * This function returns the name which belongs to the given command.
 *
 * @param cmd The integer value of the command
 * @return The name for this command
 */
const char *GetCommandName(uint32 cmd)
{
	assert(IsValidCommand(cmd));

	return _command_proc_table[cmd].name;
}

/**
 * Returns whether the command is allowed while the game is paused.
 * @param cmd The command to check.
 * @return True if the command is allowed while paused, false otherwise.
 */
bool IsCommandAllowedWhilePaused(uint32 cmd)
{
	/* Lookup table for the command types that are allowed for a given pause level setting. */
	static const int command_type_lookup[] = {
		CMDPL_ALL_ACTIONS,     ///< CMDT_LANDSCAPE_CONSTRUCTION
		CMDPL_NO_LANDSCAPING,  ///< CMDT_VEHICLE_CONSTRUCTION
		CMDPL_NO_LANDSCAPING,  ///< CMDT_MONEY_MANAGEMENT
		CMDPL_NO_CONSTRUCTION, ///< CMDT_VEHICLE_MANAGEMENT
		CMDPL_NO_CONSTRUCTION, ///< CMDT_ROUTE_MANAGEMENT
		CMDPL_NO_CONSTRUCTION, ///< CMDT_OTHER_MANAGEMENT
		CMDPL_NO_CONSTRUCTION, ///< CMDT_COMPANY_SETTING
		CMDPL_NO_ACTIONS,      ///< CMDT_SERVER_SETTING
		CMDPL_NO_ACTIONS,      ///< CMDT_CHEAT
	};
	assert_compile(lengthof(command_type_lookup) == CMDT_END);

	assert(IsValidCommand(cmd));
	return _game_mode == GM_EDITOR || command_type_lookup[_command_proc_table[cmd].type] <= _settings_game.construction.command_pause_level;
}


static int _docommand_recursive = 0;

/**
 * This function executes a given command with the parameters from the #CommandProc parameter list.
 * Depending on the flags parameter it execute or test a command.
 *
 * @param flags Flags for the command and how to execute the command
 * @see CommandProc
 * @return the cost
 */
CommandCost Command::exec (DoCommandFlag flags) const
{
	CommandCost res;

	/* Do not even think about executing out-of-bounds tile-commands */
	if (this->tile != 0 && (this->tile >= MapSize() || (!IsValidTile(this->tile) && (flags & DC_ALL_TILES) == 0))) return CMD_ERROR;

	CommandProc *proc = _command_proc_table[this->cmd].proc;

	_docommand_recursive++;

	/* only execute the test call if it's toplevel, or we're not execing. */
	if (_docommand_recursive == 1 || !(flags & DC_EXEC) ) {
		if (_docommand_recursive == 1) _cleared_object_areas.Clear();
		SetTownRatingTestMode(true);
		res = proc(this->tile, flags & ~DC_EXEC, this->p1, this->p2, this->text);
		SetTownRatingTestMode(false);
		if (res.Failed()) {
			goto error;
		}

		if (_docommand_recursive == 1 &&
				!(flags & DC_QUERY_COST) &&
				!(flags & DC_BANKRUPT) &&
				!CheckCompanyHasMoney(res)) { // CheckCompanyHasMoney() modifies 'res' to an error if it fails.
			goto error;
		}

		if (!(flags & DC_EXEC)) {
			_docommand_recursive--;
			return res;
		}
	}

	/* Execute the command here. All cost-relevant functions set the expenses type
	 * themselves to the cost object at some point */
	if (_docommand_recursive == 1) _cleared_object_areas.Clear();
	res = proc(this->tile, flags, this->p1, this->p2, this->text);
	if (res.Failed()) {
error:
		_docommand_recursive--;
		return res;
	}

	/* if toplevel, subtract the money. */
	if (--_docommand_recursive == 0 && !(flags & DC_BANKRUPT)) {
		SubtractMoneyFromCompany(res);
	}

	return res;
}

/*!
 * This functions returns the money which can be used to execute a command.
 * This is either the money of the current company or INT64_MAX if there
 * is no such a company "at the moment" like the server itself.
 *
 * @return The available money of a company or INT64_MAX
 */
Money GetAvailableMoneyForCommand()
{
	CompanyID company = _current_company;
	if (!Company::IsValidID(company)) return INT64_MAX;
	return Company::Get(company)->money;
}

/**
 * Toplevel network safe docommand function for the current company. Must not be called recursively.
 *
 * @param cmdsrc Source of the command
 * @return \c true if the command succeeded, else \c false.
 */
bool Command::execp (CommandSource cmdsrc)
{
	/* Cost estimation is generally only done when the
	 * local user presses shift while doing somthing.
	 * However, in case of incoming network commands,
	 * map generation or the pause button we do want
	 * to execute. */
	bool estimate_only = _shift_pressed && IsLocalCompany() &&
			!_generating_world &&
			cmdsrc_is_local(cmdsrc) &&
			this->cmd != CMD_PAUSE;

	/* We're only sending the command, so don't do
	 * fancy things for 'success'. */
	bool only_sending = _networking && cmdsrc_is_local(cmdsrc);

	/* Where to show the message? */
	int x = TileX(this->tile) * TILE_SIZE;
	int y = TileY(this->tile) * TILE_SIZE;

	if (_pause_mode != PM_UNPAUSED && !IsCommandAllowedWhilePaused(this->cmd)) {
		CommandErrstrF *errorstrf = _command_proc_table[this->cmd].errorstrf;
		StringID errorstr = (errorstrf == NULL) ? 0 : errorstrf (this->tile, this->p1, this->p2, this->text);
		ShowErrorMessage (errorstr, STR_ERROR_NOT_ALLOWED_WHILE_PAUSED, WL_INFO, x, y);
		return false;
	}

#ifdef ENABLE_NETWORK
	/* Only set p2 when the command does not come from the network. */
	if (cmdsrc_is_local(cmdsrc) && GetCommandFlags(this->cmd) & CMDF_CLIENT_ID && this->p2 == 0) this->p2 = CLIENT_ID_SERVER;
#endif

	CommandCost res = this->execp_internal (estimate_only, cmdsrc);
	if (res.Failed()) {
		/* Only show the error when it's for us. */
		CommandErrstrF *errorstrf = _command_proc_table[this->cmd].errorstrf;
		StringID error_part1 = (errorstrf == NULL) ? 0 : errorstrf (this->tile, this->p1, this->p2, this->text);
		if (estimate_only || (IsLocalCompany() && error_part1 != 0 && cmdsrc_get_type(cmdsrc) == CMDSRC_SELF)) {
			ShowErrorMessage(error_part1, res.GetErrorMessage(), WL_INFO, x, y, res.GetTextRefStackGRF(), res.GetTextRefStackSize(), res.GetTextRefStack());
		}
	} else if (estimate_only) {
		ShowEstimatedCostOrIncome(res.GetCost(), x, y);
	} else if (!only_sending && res.GetCost() != 0 && this->tile != 0 && IsLocalCompany() && _game_mode != GM_EDITOR) {
		/* Only show the cost animation when we did actually
		 * execute the command, i.e. we're not sending it to
		 * the server, when it has cost the local company
		 * something. Furthermore in the editor there is no
		 * concept of cost, so don't show it there either. */
		ShowCostOrIncomeAnimation(x, y, GetSlopePixelZ(x, y), res.GetCost());
	}

	if (!estimate_only && !only_sending) {
		switch (cmdsrc_get_type(cmdsrc)) {
			case CMDSRC_SELF: {
				CommandCallback *callback = _command_proc_table[this->cmd].callback;
				if (callback != NULL) {
					callback(res, this->tile, this->p1, this->p2);
				}
				break;
			}

			case CMDSRC_AI:  CcAI  (res); break;
			case CMDSRC_GS:  CcGame(res); break;
			default:  break;
		}
	}

	return res.Succeeded();
}


/**
 * Helper to deduplicate the code for returning.
 * @param cmd   the command cost to return.
 * @param clear whether to keep the storage changes or not.
 */
#define return_dcpi(cmd) { _docommand_recursive = 0; return cmd; }

/*!
 * Helper function for the toplevel network safe docommand function for the current company.
 *
 * @param estimate_only whether to give only the estimate or also execute the command
 * @param cmdsrc Source of the command
 * @return the command cost of this function.
 */
CommandCost Command::execp_internal (bool estimate_only, CommandSource cmdsrc) const
{
	assert(!estimate_only || cmdsrc_is_local(cmdsrc));

	/* Prevent recursion; it gives a mess over the network */
	assert(_docommand_recursive == 0);
	_docommand_recursive = 1;

	/* Reset the state. */
	_additional_cash_required = 0;

	/* Get pointer to command handler */
	assert(this->cmd < lengthof(_command_proc_table));
	CommandProc *proc = _command_proc_table[this->cmd].proc;
	/* Shouldn't happen, but you never know when someone adds
	 * NULLs to the _command_proc_table. */
	assert(proc != NULL);

	/* Command flags are used internally */
	CommandFlags cmd_flags = GetCommandFlags(this->cmd);
	/* Flags get send to the DoCommand */
	DoCommandFlag flags = CommandFlagsToDCFlags(cmd_flags);

#ifdef ENABLE_NETWORK
	/* Make sure p2 is properly set to a ClientID. */
	assert(!(cmd_flags & CMDF_CLIENT_ID) || this->p2 != 0);
#endif

	/* Do not even think about executing out-of-bounds tile-commands */
	if (this->tile != 0 && (this->tile >= MapSize() || (!IsValidTile(this->tile) && (cmd_flags & CMDF_ALL_TILES) == 0))) return_dcpi(CMD_ERROR);

	/* Always execute server and spectator commands as spectator */
	bool exec_as_spectator = (cmd_flags & (CMDF_SPECTATOR | CMDF_SERVER)) != 0;

	/* If the company isn't valid it may only do server command or start a new company!
	 * The server will ditch any server commands a client sends to it, so effectively
	 * this guards the server from executing functions for an invalid company. */
	if (_game_mode == GM_NORMAL && !exec_as_spectator && !Company::IsValidID(_current_company) && !(_current_company == OWNER_DEITY && (cmd_flags & CMDF_DEITY) != 0)) {
		return_dcpi(CMD_ERROR);
	}

	Backup<CompanyByte> cur_company(_current_company, FILE_LINE);
	if (exec_as_spectator) cur_company.Change(COMPANY_SPECTATOR);

	bool test_and_exec_can_differ = (cmd_flags & CMDF_NO_TEST) != 0;

	/* Test the command. */
	_cleared_object_areas.Clear();
	SetTownRatingTestMode(true);
	BasePersistentStorageArray::SwitchMode(PSM_ENTER_TESTMODE);
	CommandCost res = proc(this->tile, flags, this->p1, this->p2, this->text);
	BasePersistentStorageArray::SwitchMode(PSM_LEAVE_TESTMODE);
	SetTownRatingTestMode(false);

	/* Make sure we're not messing things up here. */
	assert(exec_as_spectator ? _current_company == COMPANY_SPECTATOR : cur_company.Verify());

	/* If the command fails, we're doing an estimate
	 * or the player does not have enough money
	 * (unless it's a command where the test and
	 * execution phase might return different costs)
	 * we bail out here. */
	if (res.Failed() || estimate_only ||
			(!test_and_exec_can_differ && !CheckCompanyHasMoney(res))) {
		if (!_networking || _generating_world || !cmdsrc_is_local(cmdsrc)) {
			/* Log the failed command as well. Just to be able to be find
			 * causes of desyncs due to bad command test implementations. */
			DEBUG (desync, 1, "cmdf: %08x.%02x %02x %06x %08x %08x %08x \"%s\" (%s)",
				_date, _date_fract, (int)_current_company,
				this->tile, this->p1, this->p2, this->cmd, this->text, GetCommandName(this->cmd));
		}
		cur_company.Restore();
		return_dcpi(res);
	}

#ifdef ENABLE_NETWORK
	/*
	 * If we are in network, and the command is not from the network
	 * send it to the command-queue and abort execution
	 */
	if (_networking && !_generating_world && cmdsrc_is_local(cmdsrc)) {
		NetworkSendCommand(this, _current_company, cmdsrc);
		cur_company.Restore();

		/* Don't return anything special here; no error, no costs.
		 * This way it's not handled by DoCommand and only the
		 * actual execution of the command causes messages. Also
		 * reset the storages as we've not executed the command. */
		return_dcpi(CommandCost());
	}
#endif /* ENABLE_NETWORK */
	DEBUG (desync, 1, "cmd: %08x.%02x %02x %06x %08x %08x %08x \"%s\" (%s)",
		_date, _date_fract, (int)_current_company,
		this->tile, this->p1, this->p2, this->cmd, this->text, GetCommandName(this->cmd));

	/* Actually try and execute the command. If no cost-type is given
	 * use the construction one */
	_cleared_object_areas.Clear();
	BasePersistentStorageArray::SwitchMode(PSM_ENTER_COMMAND);
	CommandCost res2 = proc(this->tile, flags | DC_EXEC, this->p1, this->p2, this->text);
	BasePersistentStorageArray::SwitchMode(PSM_LEAVE_COMMAND);

	if (this->cmd == CMD_COMPANY_CTRL) {
		cur_company.Trash();
		/* We are a new company                  -> Switch to new local company.
		 * We were closed down                   -> Switch to spectator
		 * Some other company opened/closed down -> The outside function will switch back */
		_current_company = _local_company;
	} else {
		/* Make sure nothing bad happened, like changing the current company. */
		assert(exec_as_spectator ? _current_company == COMPANY_SPECTATOR : cur_company.Verify());
		cur_company.Restore();
	}

	/* If the test and execution can differ we have to check the
	 * return of the command. Otherwise we can check whether the
	 * test and execution have yielded the same result,
	 * i.e. cost and error state are the same. */
	if (!test_and_exec_can_differ) {
		assert(res.GetCost() == res2.GetCost() && res.Failed() == res2.Failed()); // sanity check
	} else if (res2.Failed()) {
		return_dcpi(res2);
	}

	/* If we're needing more money and we haven't done
	 * anything yet, ask for the money! */
	if (_additional_cash_required != 0 && res2.GetCost() == 0) {
		/* It could happen we removed rail, thus gained money, and deleted something else.
		 * So make sure the signal buffer is empty even in this case */
		UpdateSignalsInBuffer();
		SetDParam(0, _additional_cash_required);
		return_dcpi(CommandCost(STR_ERROR_NOT_ENOUGH_CASH_REQUIRES_CURRENCY));
	}

	/* update last build coordinate of company. */
	if (this->tile != 0) {
		Company *c = Company::GetIfValid(_current_company);
		if (c != NULL) c->last_build_coordinate = this->tile;
	}

	SubtractMoneyFromCompany(res2);

	/* update signals if needed */
	UpdateSignalsInBuffer();

	return_dcpi(res2);
}
#undef return_dcpi


/**
 * Adds the cost of the given command return value to this cost.
 * Also takes a possible error message when it is set.
 * @param ret The command to add the cost of.
 */
void CommandCost::AddCost(const CommandCost &ret)
{
	this->AddCost(ret.cost);
	if (this->success && !ret.success) {
		this->message = ret.message;
		this->success = false;
	}
}

/**
 * Values to put on the #TextRefStack for the error message.
 * There is only one static instance of the array, just like there is only one
 * instance of normal DParams.
 */
uint32 CommandCost::textref_stack[16];

/**
 * Activate usage of the NewGRF #TextRefStack for the error message.
 * @param grffile NewGRF that provides the #TextRefStack
 * @param num_registers number of entries to copy from the temporary NewGRF registers
 */
void CommandCost::UseTextRefStack(const GRFFile *grffile, uint num_registers)
{
	extern TemporaryStorageArray<int32, 0x110> _temp_store;

	assert(num_registers < lengthof(textref_stack));
	this->textref_stack_grffile = grffile;
	this->textref_stack_size = num_registers;
	for (uint i = 0; i < num_registers; i++) {
		textref_stack[i] = _temp_store.GetValue(0x100 + i);
	}
}
