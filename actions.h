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

enum hb_replay_action_kind {
	HB_REPLAY_ACTION_PLAYER_JOIN = 0,
	HB_REPLAY_ACTION_PLAYER_LEAVE = 1,
	HB_REPLAY_ACTION_PLAYER_CHAT = 2,
	HB_REPLAY_ACTION_LOGIC_UPDATE = 3,
	HB_REPLAY_ACTION_START_MATCH = 4,
	HB_REPLAY_ACTION_STOP_MATCH = 5,
	HB_REPLAY_ACTION_SET_PLAYER_INPUT = 6,
	HB_REPLAY_ACTION_SET_PLAYER_TEAM = 7,
	HB_REPLAY_ACTION_SET_TEAMS_LOCK = 8,
	HB_REPLAY_ACTION_SET_GAME_SETTING = 9,
	HB_REPLAY_ACTION_SET_PLAYER_AVATAR = 10,
	HB_REPLAY_ACTION_SET_PLAYER_DESYNC = 11,
	HB_REPLAY_ACTION_SET_PLAYER_ADMIN = 12,
	HB_REPLAY_ACTION_SET_STADIUM = 13,
	HB_REPLAY_ACTION_PAUSE_RESUME_GAME = 14,
	HB_REPLAY_ACTION_PING_UPDATE = 15,
	HB_REPLAY_ACTION_SET_PLAYER_HANDICAP = 16,
	HB_REPLAY_ACTION_SET_TEAM_COLORS = 17
};

#define HB_ACTION_GENERIC_PROPS() \
	uint32_t by_player; \
	uint8_t type;

struct hb_action_generic {
	HB_ACTION_GENERIC_PROPS();
};

struct hb_action_player_join {
	HB_ACTION_GENERIC_PROPS();
	uint32_t id;
	char name[128], country[5];
	bool is_admin;
};

struct hb_action_player_leave {
	HB_ACTION_GENERIC_PROPS();
	uint16_t id;
	bool kicked, ban;
	char reason[256];
};

struct hb_action_player_chat {
	HB_ACTION_GENERIC_PROPS();
	char message[256];
};

struct hb_action_logic_update {
	HB_ACTION_GENERIC_PROPS();
	/* No data */
};

struct hb_action_start_match {
	HB_ACTION_GENERIC_PROPS();
	/* No data */
};

struct hb_action_stop_match {
	HB_ACTION_GENERIC_PROPS();
	/* No data */
};

struct hb_action_set_player_input {
	HB_ACTION_GENERIC_PROPS();
	uint8_t input;
};

struct hb_action_set_player_team {
	HB_ACTION_GENERIC_PROPS();
	uint32_t id;
	enum hb_team team;
};

struct hb_action_set_teams_lock {
	HB_ACTION_GENERIC_PROPS();
	bool teams_lock;
};

struct hb_action_set_game_setting {
	HB_ACTION_GENERIC_PROPS();
	uint8_t setting_id;
	uint32_t setting_value;
};

struct hb_action_set_player_avatar {
	HB_ACTION_GENERIC_PROPS();
	char avatar[10];
};

struct hb_action_set_player_desync {
	HB_ACTION_GENERIC_PROPS();
	/* No data */
};

struct hb_action_set_player_admin {
	HB_ACTION_GENERIC_PROPS();
	uint32_t id;
	bool is_admin;
};

struct hb_action_set_stadium {
	HB_ACTION_GENERIC_PROPS();
	const char *default_stadium;
	struct hb_stadium stadium;
};

struct hb_action_pause_resume_game {
	HB_ACTION_GENERIC_PROPS();
	bool paused;
};

struct hb_action_ping_update {
	HB_ACTION_GENERIC_PROPS();
	uint8_t ping_count;
	uint32_t pings[255];
};

struct hb_action_set_player_handicap {
	HB_ACTION_GENERIC_PROPS();
	uint16_t handicap;
};

struct hb_action_set_team_colors {
	HB_ACTION_GENERIC_PROPS();
	enum hb_team team;
	uint16_t angle;
	uint32_t avatar_color;
	uint8_t num_stripes;
	uint32_t stripes[3];
};

void hb_action_free(struct hb_action_generic *action);
