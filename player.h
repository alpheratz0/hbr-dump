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

#include <hb/team.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define HB_PLAYER_LIST_MAX_PLAYERS (60)

struct hb_player
{
	uint32_t id, input, disc_id;
	char name[128], country[10], avatar[10];
	bool is_admin;
	enum hb_team team;
	uint8_t number, kicking, desynced;
	uint16_t handicap;
};

struct hb_player_list
{
	struct hb_player players[HB_PLAYER_LIST_MAX_PLAYERS];
	size_t length;
};

void hb_player_list_add(struct hb_player_list *list, uint32_t id,
		char *name, bool is_admin, char *country);
void hb_player_list_remove(struct hb_player_list *list, uint32_t id);
int hb_player_list_index_of(struct hb_player_list *list, uint32_t id);
bool hb_player_list_contains(struct hb_player_list *list, uint32_t id);
struct hb_player hb_player_list_get(struct hb_player_list *list, uint32_t id);
