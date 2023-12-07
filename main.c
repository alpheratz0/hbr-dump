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

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "stream_reader.h"
#include "actions.h"
#include "hbr.h"

static enum { DumpMessages, DumpStadiums } mode = DumpMessages;

static void hb_player_list_add(struct hb_player_list *player_list,
		uint32_t id, char *name, bool is_admin, char *country)
{
	struct hb_player player = {0};
	player.id = id;
	strcpy(player.name, name);
	player.is_admin = is_admin;
	strcpy(player.country, country);
	player_list->players[player_list->length++] = player;
}

static void hb_player_list_remove(struct hb_player_list *player_list, uint32_t id)
{
	for (size_t i = 0; i < player_list->length; ++i) {
		if (player_list->players[i].id == id) {
			for (size_t j = i+1; j < player_list->length; ++j)
				player_list->players[j-1] = player_list->players[j];
			player_list->length -= 1;
			break;
		}
	}
}

static int hb_player_list_index_of(struct hb_player_list *player_list, uint32_t id)
{
	for (size_t i = 0; i < player_list->length; ++i)
		if (player_list->players[i].id == id)
			return (int) i;
	return -1;
}

static bool hb_player_list_contains(struct hb_player_list *player_list, uint32_t id)
{
	return hb_player_list_index_of(player_list, id) != -1;
}

static struct hb_player hb_player_list_get(struct hb_player_list *player_list, uint32_t id)
{
	int index = hb_player_list_index_of(player_list, id);
	if (index == -1) return ((struct hb_player) {0});
	return player_list->players[index];
}

static void on_player_join(struct hb_hbr *hbr, struct hb_action_player_join *act)
{
	if (mode == DumpMessages)
		printf("%s joined the room!\n", act->name);
	hb_player_list_add(&hbr->player_list, act->id, act->name, act->is_admin, act->country);
}

static void on_player_leave(struct hb_hbr *hbr, struct hb_action_player_leave *act)
{
	if (mode == DumpMessages) {
		if (!hb_player_list_contains(&hbr->player_list, act->id)) return;
		struct hb_player player = hb_player_list_get(&hbr->player_list, act->id);
		if (act->kicked || act->ban) {
			struct hb_player by = hb_player_list_get(&hbr->player_list, act->by_player);
			printf("%s %s from the room by %s (Reason: %s)\n", player.name, act->ban ? "banned" : "kicked",
					by.name, act->reason);
		} else {
			printf("%s left the room!\n", player.name);
		}
	}
	hb_player_list_remove(&hbr->player_list, act->id);
}

static void on_player_chat(struct hb_hbr *hbr, struct hb_action_player_chat *act)
{
	if (mode != DumpMessages) return;
	if (!hb_player_list_contains(&hbr->player_list, act->by_player)) return;
	struct hb_player sender = hb_player_list_get(&hbr->player_list, act->by_player);
	printf("%s: %s\n", sender.name, act->message);
}

static void on_match_start(struct hb_hbr *hbr, struct hb_action_start_match *act)
{
	if (mode != DumpMessages) return;
	if (!hb_player_list_contains(&hbr->player_list, act->by_player)) return;
	struct hb_player player = hb_player_list_get(&hbr->player_list, act->by_player);
	printf("Game started by %s!\n", player.name);
}

static void on_match_stop(struct hb_hbr *hbr, struct hb_action_stop_match *act)
{
	if (mode != DumpMessages) return;
	if (!hb_player_list_contains(&hbr->player_list, act->by_player)) return;
	struct hb_player player = hb_player_list_get(&hbr->player_list, act->by_player);
	printf("Game stopped by %s!\n", player.name);
}

static void on_player_admin_change(struct hb_hbr *hbr, struct hb_action_set_player_admin *act)
{
	if (mode != DumpMessages) return;
	if (!hb_player_list_contains(&hbr->player_list, act->id)) return;
	struct hb_player player = hb_player_list_get(&hbr->player_list, act->id);
	struct hb_player by = hb_player_list_get(&hbr->player_list, act->by_player);
	if (act->is_admin) printf("%s was given admin rights by %s.\n", player.name, by.name);
	else printf("%s's admin rights were taken away by %s.\n", player.name, by.name);
}

static void on_player_team_change(struct hb_hbr *hbr, struct hb_action_set_player_team *act)
{
	if (mode != DumpMessages) return;
	if (!hb_player_list_contains(&hbr->player_list, act->id)) return;
	struct hb_player player = hb_player_list_get(&hbr->player_list, act->id);
	struct hb_player by = hb_player_list_get(&hbr->player_list, act->by_player);
	const char *teams[] = {"spectators", "red", "blue"};
	printf("%s was moved to %s by %s\n", player.name, teams[act->team], by.name);
}

static void on_game_paused(struct hb_hbr *hbr, struct hb_action_pause_resume_game *act)
{
	if (mode != DumpMessages) return;
	if (!hb_player_list_contains(&hbr->player_list, act->by_player)) return;
	struct hb_player player = hb_player_list_get(&hbr->player_list, act->by_player);
	printf("Game %spaused by %s\n", act->paused ? "" : "un", player.name);
}

static void save_stadium(struct hb_stadium *stadium)
{
	char *hbs_data = hb_stadium_to_str(stadium);
	char filename[256];
	snprintf(filename, sizeof(filename), "%05d.hbs", rand() % 100000);
	FILE *fp = fopen(filename, "w");
	fputs(hbs_data, fp);
	fclose(fp);
	free(hbs_data);
}

static void on_stadium_change(struct hb_hbr *hbr, struct hb_action_set_stadium *act)
{
	struct hb_player player = hb_player_list_get(&hbr->player_list, act->by_player);
	if (!hb_player_list_contains(&hbr->player_list, act->by_player)) return;
	if (mode == DumpMessages) {
		printf("Stadium changed to \"%s\" by %s\n",
				act->default_stadium ? act->default_stadium : act->stadium.name,
				player.name);
	} else if (NULL == act->default_stadium){
		save_stadium(&act->stadium);
	}
}

int
main(int argc, char **argv)
{
	if (argc <= 2) return 1;

	if (!strcmp(argv[1], "-messages")) mode = DumpMessages;
	else if (!strcmp(argv[1], "-stadiums")) mode = DumpStadiums;
	else { printf("Invalid option!\n"); return 1; }

	struct hb_hbr *hbr = hb_hbr_parse(argv[2]);
	struct hb_action_generic *act;

	srand((unsigned ) getpid());

	if (mode == DumpStadiums && hbr->default_stadium == NULL) {
		save_stadium(&hbr->stadium);
	}

	if (mode == DumpMessages) {
		for (size_t i = 0; i < hbr->player_list.length; ++i)
			printf("%s was in the room.\n", hbr->player_list.players[i].name);
	}

	while ((act = hb_hbr_next_action(hbr))) {
		switch (act->type) {
		case HB_REPLAY_ACTION_PLAYER_JOIN: on_player_join(hbr, (void *)act); break;
		case HB_REPLAY_ACTION_PLAYER_LEAVE: on_player_leave(hbr, (void *)act); break;
		case HB_REPLAY_ACTION_PLAYER_CHAT: on_player_chat(hbr, (void *)act); break;
		case HB_REPLAY_ACTION_START_MATCH: on_match_start(hbr, (void *)act); break;
		case HB_REPLAY_ACTION_STOP_MATCH: on_match_stop(hbr, (void *)act); break;
		case HB_REPLAY_ACTION_SET_PLAYER_ADMIN: on_player_admin_change(hbr, (void *)act); break;
		case HB_REPLAY_ACTION_SET_PLAYER_TEAM: on_player_team_change(hbr, (void *)act); break;
		case HB_REPLAY_ACTION_PAUSE_RESUME_GAME: on_game_paused(hbr, (void *)act); break;
		case HB_REPLAY_ACTION_SET_STADIUM: on_stadium_change(hbr, (void *)act); break;
		}

		hb_action_free(act);
	}

	hb_hbr_free(hbr);

	return 0;
}
