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

#include <hb/disc.h>
#include <hb/shirt.h>
#include <hb/stadium.h>
#include <hb/team.h>
#include <stdint.h>

#include "player.h"
#include "stream_reader.h"
#include "events.h"

#define HBR_MAGIC (0x48425250)
#define HBR_MIN_VERSION (7)
#define HBR_MAX_VERSION (12)

struct hbr
{
	uint32_t version;
	uint32_t magic;
	uint32_t total_frames;
	uint32_t start_frame;
	char room_name[128];
	bool teams_lock;
	uint8_t score_limit;
	uint8_t time_limit;
	uint32_t rules_timer;
	uint8_t kick_off_taken;
	uint8_t kick_off_team;
	double ball_x, ball_y;
	uint32_t score_red, score_blue;
	double match_time;
	uint8_t pause_timer;
	const char *default_stadium;
	struct hb_stadium stadium;
	bool in_progress;
	struct hb_disc_list in_game_disc_list;
	struct hb_player_list player_list;
	struct hb_shirt red_shirt, blue_shirt;
	uint32_t current_frame;
	struct hb_stream_reader *stream;
};

struct hbr *hbr_parse(const char *path);
int hbr_next_event(struct hbr *hbr, struct hb_event *ev);
void hbr_free(struct hbr *hbr);
