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

#include <hb/team.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "player.h"

void hb_player_list_add(struct hb_player_list *list, uint32_t id,
		char *name, bool is_admin, char *country)
{
	struct hb_player player = {0};
	player.id = id;
	strcpy(player.name, name);
	player.is_admin = is_admin;
	strcpy(player.country, country);
	list->players[list->length++] = player;
}

void hb_player_list_remove(struct hb_player_list *list, uint32_t id)
{
	for (size_t i = 0; i < list->length; ++i) {
		if (list->players[i].id == id) {
			for (size_t j = i+1; j < list->length; ++j)
				list->players[j-1] = list->players[j];
			list->length -= 1;
			break;
		}
	}
}

int hb_player_list_index_of(struct hb_player_list *list, uint32_t id)
{
	for (size_t i = 0; i < list->length; ++i)
		if (list->players[i].id == id)
			return (int) i;
	return -1;
}

bool hb_player_list_contains(struct hb_player_list *list, uint32_t id)
{
	return hb_player_list_index_of(list, id) != -1;
}

struct hb_player hb_player_list_get(struct hb_player_list *list, uint32_t id)
{
	int index = hb_player_list_index_of(list, id);
	if (index == -1) return ((struct hb_player) {0});
	return list->players[index];
}
