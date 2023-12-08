// ISC License (C) 2023 <alpheratz99@protonmail.com>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
// OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#pragma once

#include <hb/stadium.h>
#include <hb/team.h>
#include <stdint.h>

enum hb_event_kind
{
	HB_EVENT_PLAYER_JOIN         =    0,
	HB_EVENT_PLAYER_LEAVE        =    1,
	HB_EVENT_PLAYER_CHAT         =    2,
	HB_EVENT_LOGIC_UPDATE        =    3,
	HB_EVENT_START_MATCH         =    4,
	HB_EVENT_STOP_MATCH          =    5,
	HB_EVENT_SET_PLAYER_INPUT    =    6,
	HB_EVENT_SET_PLAYER_TEAM     =    7,
	HB_EVENT_SET_TEAMS_LOCK      =    8,
	HB_EVENT_SET_GAME_SETTING    =    9,
	HB_EVENT_SET_PLAYER_AVATAR   =   10,
	HB_EVENT_SET_PLAYER_DESYNC   =   11,
	HB_EVENT_SET_PLAYER_ADMIN    =   12,
	HB_EVENT_SET_STADIUM         =   13,
	HB_EVENT_PAUSE_RESUME_GAME   =   14,
	HB_EVENT_PING_UPDATE         =   15,
	HB_EVENT_SET_PLAYER_HANDICAP =   16,
	HB_EVENT_SET_TEAM_COLORS     =   17
};

struct hb_event {
	uint32_t by_player;
	uint8_t type;
	union {
		struct hb_event_player_join { uint32_t id; char name[128], country[5]; bool is_admin; } player_join;
		struct hb_event_player_leave { uint16_t id; bool kicked, ban; char reason[256]; } player_leave;
		struct hb_event_player_chat { char message[256]; } player_chat;
		struct hb_event_set_player_input { uint8_t input; } set_player_input;
		struct hb_event_set_player_team { uint32_t id; enum hb_team team; } set_player_team;
		struct hb_event_set_teams_lock { bool teams_lock; } set_teams_lock;
		struct hb_event_set_game_setting { uint8_t setting_id; uint32_t setting_value; } set_game_setting;
		struct hb_event_set_player_avatar { char avatar[10]; } set_player_avatar;
		struct hb_event_set_player_admin { uint32_t id; bool is_admin; } set_player_admin;
		struct hb_event_set_stadium { const char *default_stadium; struct hb_stadium stadium; } set_stadium;
		struct hb_event_pause_resume_game { bool paused; } pause_resume_game;
		struct hb_event_ping_update { uint8_t ping_count; uint32_t pings[255]; } ping_update;
		struct hb_event_set_player_handicap { uint16_t handicap; } set_player_handicap;
		struct hb_event_set_team_colors { enum hb_team team; struct hb_shirt shirt; } set_team_colors;
	} ev;
}
