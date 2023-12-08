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
#include "player.h"
#include "events.h"
#include "hbr.h"

// Comment this if you dont want the stadiums to be storable.
#define HBR_DUMP_MAKE_STADIUMS_STORABLES

static enum { DumpMessages, DumpStadiums } mode = DumpMessages;

static void on_player_join(struct hbr *hbr, struct hb_event_player_join *ev)
{
	if (mode == DumpMessages)
		printf("%s joined the room!\n", ev->name);
	hb_player_list_add(&hbr->player_list, ev->id, ev->name, ev->is_admin, ev->country);
}

static void on_player_leave(struct hbr *hbr, uint32_t by_player, struct hb_event_player_leave *ev)
{
	if (mode == DumpMessages) {
		if (!hb_player_list_contains(&hbr->player_list, ev->id)) return;
		struct hb_player player = hb_player_list_get(&hbr->player_list, ev->id);
		if (ev->kicked || ev->ban) {
			struct hb_player by = hb_player_list_get(&hbr->player_list, by_player);
			printf("%s %s from the room by %s (Reason: %s)\n", player.name, ev->ban ? "banned" : "kicked",
					by.name, ev->reason);
		} else {
			printf("%s left the room!\n", player.name);
		}
	}
	hb_player_list_remove(&hbr->player_list, ev->id);
}

static void on_player_chat(struct hbr *hbr, uint32_t by_player, struct hb_event_player_chat *ev)
{
	if (mode != DumpMessages) return;
	if (!hb_player_list_contains(&hbr->player_list, by_player)) return;
	struct hb_player sender = hb_player_list_get(&hbr->player_list, by_player);
	printf("%s: %s\n", sender.name, ev->message);
}

static void on_match_start(struct hbr *hbr, uint32_t by_player)
{
	if (mode != DumpMessages) return;
	if (!hb_player_list_contains(&hbr->player_list, by_player)) return;
	struct hb_player player = hb_player_list_get(&hbr->player_list, by_player);
	printf("Game started by %s!\n", player.name);
}

static void on_match_stop(struct hbr *hbr, uint32_t by_player)
{
	if (mode != DumpMessages) return;
	if (!hb_player_list_contains(&hbr->player_list, by_player)) return;
	struct hb_player player = hb_player_list_get(&hbr->player_list, by_player);
	printf("Game stopped by %s!\n", player.name);
}

static void on_player_admin_change(struct hbr *hbr, uint32_t by_player, struct hb_event_set_player_admin *ev)
{
	if (mode != DumpMessages) return;
	if (!hb_player_list_contains(&hbr->player_list, ev->id)) return;
	struct hb_player player = hb_player_list_get(&hbr->player_list, ev->id);
	struct hb_player by = hb_player_list_get(&hbr->player_list, by_player);
	if (ev->is_admin) printf("%s was given admin rights by %s.\n", player.name, by.name);
	else printf("%s's admin rights were taken away by %s.\n", player.name, by.name);
}

static void on_player_team_change(struct hbr *hbr, uint32_t by_player, struct hb_event_set_player_team *ev)
{
	if (mode != DumpMessages) return;
	if (!hb_player_list_contains(&hbr->player_list, ev->id)) return;
	struct hb_player player = hb_player_list_get(&hbr->player_list, ev->id);
	struct hb_player by = hb_player_list_get(&hbr->player_list, by_player);
	const char *teams[] = {"spectators", "red", "blue"};
	printf("%s was moved to %s by %s\n", player.name, teams[ev->team], by.name);
}

static void on_game_paused(struct hbr *hbr, uint32_t by_player, struct hb_event_pause_resume_game *ev)
{
	if (mode != DumpMessages) return;
	if (!hb_player_list_contains(&hbr->player_list, by_player)) return;
	struct hb_player player = hb_player_list_get(&hbr->player_list, by_player);
	printf("Game %spaused by %s\n", ev->paused ? "" : "un", player.name);
}

static void save_stadium(struct hb_stadium *stadium)
{
#ifdef HBR_DUMP_MAKE_STADIUMS_STORABLES
	stadium->can_be_stored = true;
#endif
	char *hbs_data = hb_stadium_to_str(stadium);
	char filename[256];
	snprintf(filename, sizeof(filename), "%05d.hbs", rand() % 100000);
	FILE *fp = fopen(filename, "w");
	fputs(hbs_data, fp);
	fclose(fp);
	free(hbs_data);
}

static void on_stadium_change(struct hbr *hbr, uint32_t by_player, struct hb_event_set_stadium *ev)
{
	struct hb_player player = hb_player_list_get(&hbr->player_list, by_player);
	if (!hb_player_list_contains(&hbr->player_list, by_player)) return;
	if (mode == DumpMessages) {
		printf("Stadium changed to \"%s\" by %s\n",
				ev->default_stadium ? ev->default_stadium : ev->stadium.name,
				player.name);
	} else if (NULL == ev->default_stadium){
		save_stadium(&ev->stadium);
	}
}

int
main(int argc, char **argv)
{
	if (argc <= 2) return 1;

	if (!strcmp(argv[1], "-messages")) mode = DumpMessages;
	else if (!strcmp(argv[1], "-stadiums")) mode = DumpStadiums;
	else { printf("Invalid option!\n"); return 1; }

	struct hbr *hbr = hbr_parse(argv[2]);
	struct hb_event ev = {0};

	srand((unsigned ) getpid());

	if (mode == DumpStadiums && hbr->default_stadium == NULL) {
		save_stadium(&hbr->stadium);
	}

	if (mode == DumpMessages) {
		printf("Room name: %s\n", hbr->room_name);
		printf("Stadium: %s.\n", hbr->default_stadium != NULL ? hbr->default_stadium : hbr->stadium.name);
		for (size_t i = 0; i < hbr->player_list.length; ++i)
			printf("%s was in the room.\n", hbr->player_list.players[i].name);
	}

	while (hbr_next_event(hbr, &ev) > 0) {
		switch (ev.type) {
		case HB_EVENT_PLAYER_JOIN: on_player_join(hbr, &ev.player_join); break;
		case HB_EVENT_PLAYER_LEAVE: on_player_leave(hbr, ev.by_player, &ev.player_leave); break;
		case HB_EVENT_PLAYER_CHAT: on_player_chat(hbr, ev.by_player, &ev.player_chat); break;
		case HB_EVENT_START_MATCH: on_match_start(hbr, ev.by_player); break;
		case HB_EVENT_STOP_MATCH: on_match_stop(hbr, ev.by_player); break;
		case HB_EVENT_SET_PLAYER_ADMIN: on_player_admin_change(hbr, ev.by_player, &ev.set_player_admin); break;
		case HB_EVENT_SET_PLAYER_TEAM: on_player_team_change(hbr, ev.by_player, &ev.set_player_team); break;
		case HB_EVENT_PAUSE_RESUME_GAME: on_game_paused(hbr, ev.by_player, &ev.pause_resume_game); break;
		case HB_EVENT_SET_STADIUM: on_stadium_change(hbr, ev.by_player, &ev.set_stadium); break;
		}
	}

	hbr_free(hbr);

	return 0;
}
