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

#include <math.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hb/background.h>
#include <hb/goal.h>
#include <hb/plane.h>
#include <hb/player_physics.h>
#include <hb/segment.h>
#include <hb/team.h>
#include <hb/vertex.h>
#include <hb/disc.h>
#include <hb/shirt.h>
#include <hb/stadium.h>
#include "player.h"
#include "stream_reader.h"
#include "events.h"
#include "hbr.h"

static enum hb_team hb_stream_reader_team(struct hb_stream_reader *s)
{
	// FIXME: this is not ok, seems like team id is parsed in different ways
	switch (hb_stream_reader_uint8(s)) {
	case 0:  return HB_TEAM_BLUE;
	case 1:  return HB_TEAM_RED;
	default: return HB_TEAM_SPECTATOR;
	}
}

static double hb_stream_reader_curve(struct hb_stream_reader *s)
{
	double curve_f = hb_stream_reader_double(s);
	if (isnan(curve_f)) curve_f = INFINITY;
	if (curve_f == 0.0) return 180.0;
	return fmod(((atan(1 / curve_f) * 360.0) / M_PI) + 360.0, 360.0);
}

static void hb_stream_reader_disc(struct hb_stream_reader *s,
		struct hb_disc *disc)
{
	disc->pos.x              = hb_stream_reader_double(s);
	disc->pos.y              = hb_stream_reader_double(s);
	disc->speed.x            = hb_stream_reader_double(s);
	disc->speed.y            = hb_stream_reader_double(s);
	disc->radius             = hb_stream_reader_double(s);
	disc->b_coef             = hb_stream_reader_double(s);
	disc->inv_mass           = hb_stream_reader_double(s);
	disc->damping            = hb_stream_reader_double(s);
	disc->color              = hb_stream_reader_uint32(s);
	disc->c_mask             = hb_stream_reader_uint32(s);
	disc->c_group            = hb_stream_reader_uint32(s);
}

static void hb_stream_reader_goal(struct hb_stream_reader *s,
		struct hb_goal *goal)
{
	goal->p0.x               = hb_stream_reader_double(s);
	goal->p0.y               = hb_stream_reader_double(s);
	goal->p1.x               = hb_stream_reader_double(s);
	goal->p1.y               = hb_stream_reader_double(s);
	goal->team               = hb_stream_reader_team(s);
}

static void hb_stream_reader_plane(struct hb_stream_reader *s,
		struct hb_plane *plane)
{
	plane->normal.x          = hb_stream_reader_double(s);
	plane->normal.y          = hb_stream_reader_double(s);
	plane->dist              = hb_stream_reader_double(s);
	plane->b_coef            = hb_stream_reader_double(s);
	plane->c_mask            = hb_stream_reader_uint32(s);
	plane->c_group           = hb_stream_reader_uint32(s);
}

static void hb_stream_reader_segment(struct hb_stream_reader *s,
		struct hb_segment *segment)
{
	segment->v0              = hb_stream_reader_uint8(s);
	segment->v1              = hb_stream_reader_uint8(s);
	segment->b_coef          = hb_stream_reader_double(s);
	segment->c_mask          = hb_stream_reader_uint32(s);
	segment->c_group         = hb_stream_reader_uint32(s);
	segment->curve           = hb_stream_reader_curve(s);
	segment->vis             = hb_stream_reader_bool(s);
	segment->color           = hb_stream_reader_uint32(s);
}

static void hb_stream_reader_vertex(struct hb_stream_reader *s,
		struct hb_vertex *vertex)
{
	vertex->x                = hb_stream_reader_double(s);
	vertex->y                = hb_stream_reader_double(s);
	vertex->b_coef           = hb_stream_reader_double(s);
	vertex->c_mask           = hb_stream_reader_uint32(s);
	vertex->c_group          = hb_stream_reader_uint32(s);
}

static void hb_stream_reader_bg(struct hb_stream_reader *s,
		struct hb_background *bg)
{
	bg->type                 = hb_stream_reader_uint8(s);
	bg->width                = hb_stream_reader_double(s);
	bg->height               = hb_stream_reader_double(s);
	bg->kick_off_radius      = hb_stream_reader_double(s);
	bg->corner_radius        = hb_stream_reader_double(s);
	bg->goal_line            = hb_stream_reader_double(s);
	bg->color                = hb_stream_reader_uint32(s);
}

static void hb_stream_reader_player_physics(struct hb_stream_reader *s,
		struct hb_player_physics *pp)
{
	pp->b_coef               = hb_stream_reader_double(s);
	pp->inv_mass             = hb_stream_reader_double(s);
	pp->damping              = hb_stream_reader_double(s);
	pp->acceleration         = hb_stream_reader_double(s);
	pp->kicking_acceleration = hb_stream_reader_double(s);
	pp->kicking_damping      = hb_stream_reader_double(s);
	pp->kick_strength        = hb_stream_reader_double(s);
	pp->radius               = 15.0;
}

static void hb_stream_reader_player(struct hb_stream_reader *s,
		uint32_t version, struct hb_player *player)
{
	player->id               = hb_stream_reader_uint32(s);

	hb_stream_reader_string_ascii_auto(s, player->name);

	player->is_admin         = hb_stream_reader_bool(s);
	player->team             = hb_stream_reader_team(s);
	player->number           = hb_stream_reader_uint8(s);

	hb_stream_reader_string_ascii_auto(s, player->avatar);

	player->input            = hb_stream_reader_uint32(s);
	player->kicking          = hb_stream_reader_uint8(s);
	player->desynced         = hb_stream_reader_uint8(s);

	hb_stream_reader_string_ascii_auto(s, player->country);

	player->handicap         = version >= 11 ? hb_stream_reader_uint16(s) : 0;
	player->disc_id          = hb_stream_reader_uint32(s);
}


static struct hb_shirt hb_stream_reader_shirt(struct hb_stream_reader *s)
{
	struct hb_shirt shirt;

	shirt.angle             = (double)hb_stream_reader_uint16(s);
	shirt.avatar_color      = hb_stream_reader_uint32(s);
	shirt.num_colors        = hb_stream_reader_uint8(s);

	assert(shirt.num_colors <= 3);

	for (size_t i = 0; i < shirt.num_colors; ++i) {
		shirt.colors[i] = hb_stream_reader_uint32(s);
	}

	return shirt;
}

static void hb_stream_reader_vertex_list(struct hb_stream_reader *s,
		size_t count, struct hb_vertex_list *list)
{
	assert(list->length + count <= HB_VERTEX_LIST_MAX_VERTEXES);
	while (count-- > 0)
		hb_stream_reader_vertex(s, &list->vertexes[list->length++]);
}

static void hb_stream_reader_segment_list(struct hb_stream_reader *s,
		size_t count, struct hb_segment_list *list)
{
	assert(list->length + count <= HB_SEGMENT_LIST_MAX_SEGMENTS);
	while (count-- > 0)
		hb_stream_reader_segment(s, &list->segments[list->length++]);
}

static void hb_stream_reader_plane_list(struct hb_stream_reader *s,
		size_t count, struct hb_plane_list *list)
{
	assert(list->length + count <= HB_PLANE_LIST_MAX_PLANES);
	while (count-- > 0)
		hb_stream_reader_plane(s, &list->planes[list->length++]);
}

static void hb_stream_reader_goal_list(struct hb_stream_reader *s,
		size_t count, struct hb_goal_list *list)
{
	assert(list->length + count <= HB_GOAL_LIST_MAX_GOALS);
	while (count-- > 0)
		hb_stream_reader_goal(s, &list->goals[list->length++]);
}

static void hb_stream_reader_disc_list(struct hb_stream_reader *s,
		size_t count, struct hb_disc_list *list)
{
	assert(list->length + count <= HB_DISC_LIST_MAX_DISCS);
	while (count-- > 0)
		hb_stream_reader_disc(s, &list->discs[list->length++]);
}

static void hb_stream_reader_player_list(struct hb_stream_reader *s,
		uint32_t version, size_t count, struct hb_player_list *list)
{
	assert(list->length + count <= HB_PLAYER_LIST_MAX_PLAYERS);
	while (count-- > 0)
		hb_stream_reader_player(s, version, &list->players[list->length++]);
}

static void hb_stream_reader_stadium(struct hb_stream_reader *s,
		const char **default_stadium, struct hb_stadium *stadium)
{
	static const char *default_stadium_names[] = {
		"Classic", "Easy", "Small",
		"Big"," Rounded", "Hockey",
		"Big Hockey", "Big Easy",
		"Big Rounded", "Huge"
	};

	static size_t default_stadium_names_count = sizeof(default_stadium_names) /
		sizeof(default_stadium_names[0]);

	uint8_t stadium_id = hb_stream_reader_uint8(s);

	if (stadium_id < default_stadium_names_count) {
		*default_stadium = default_stadium_names[stadium_id];
		return;
	}

	// Custom stadium
	*default_stadium = NULL;
	memset(stadium, 0, sizeof(*stadium));
	stadium->disc_list.length = 1;
	hb_stream_reader_string_ascii_auto(s, stadium->name);
	hb_stream_reader_bg(s, &stadium->bg);
	stadium->width = hb_stream_reader_double(s);
	stadium->height = hb_stream_reader_double(s);
	stadium->spawn_distance = hb_stream_reader_double(s);
	hb_stream_reader_vertex_list(s, hb_stream_reader_uint8(s), &stadium->vertex_list);
	hb_stream_reader_segment_list(s, hb_stream_reader_uint8(s), &stadium->segment_list);
	hb_stream_reader_plane_list(s, hb_stream_reader_uint8(s), &stadium->plane_list);
	hb_stream_reader_goal_list(s, hb_stream_reader_uint8(s), &stadium->goal_list);
	hb_stream_reader_disc_list(s, hb_stream_reader_uint8(s), &stadium->disc_list);
	hb_stream_reader_player_physics(s, &stadium->player_physics);
	struct hb_disc *ball_physics = &stadium->disc_list.discs[0];
	hb_stream_reader_disc(s, ball_physics);
	ball_physics->c_group |= HB_COLLISION_KICK|HB_COLLISION_SCORE|HB_COLLISION_BALL;
}

struct hbr *hbr_parse(const char *path)
{
	struct hbr *hbr = calloc(1, sizeof(*hbr));
	struct hb_stream_reader *s = hbr->stream = hb_stream_reader_from_file(path);

	hbr->version            = hb_stream_reader_uint32(s);
	assert(hbr->version >= HBR_MIN_VERSION && hbr->version <= HBR_MAX_VERSION);

	hbr->magic              = hb_stream_reader_uint32(s);
	assert(hbr->magic == HBR_MAGIC);

	hbr->total_frames       = hb_stream_reader_uint32(s);

	hb_stream_reader_inflate(s, false);

	hbr->start_frame        = hb_stream_reader_uint32(s);

	hb_stream_reader_string_ascii_auto(s, hbr->room_name);

	hbr->teams_lock         = hb_stream_reader_bool(s);
	hbr->score_limit        = hb_stream_reader_uint8(s);
	hbr->time_limit         = hb_stream_reader_uint8(s);
	hbr->rules_timer        = hb_stream_reader_uint32(s);
	hbr->kick_off_taken     = hb_stream_reader_uint8(s);
	hbr->kick_off_team      = hb_stream_reader_uint8(s);
	hbr->ball_x             = hb_stream_reader_double(s);
	hbr->ball_y             = hb_stream_reader_double(s);
	hbr->score_red          = hb_stream_reader_uint32(s);
	hbr->score_blue         = hb_stream_reader_uint32(s);
	hbr->match_time         = hb_stream_reader_double(s);
	hbr->pause_timer        = hb_stream_reader_uint8(s);

	hb_stream_reader_stadium(s, &hbr->default_stadium,
			&hbr->stadium);

	hbr->in_progress = hb_stream_reader_bool(s);
	if (hbr->in_progress) {
		hb_stream_reader_disc_list(s, hb_stream_reader_uint32(s),
				&hbr->in_game_disc_list);
	}
	hb_stream_reader_player_list(s, hbr->version, hb_stream_reader_uint32(s),
			&hbr->player_list);
	if (hbr->version < 12)
		return hbr;
	hbr->red_shirt = hb_stream_reader_shirt(s);
	hbr->blue_shirt = hb_stream_reader_shirt(s);

	return hbr;
}

static void parse_event_player_join(struct hb_stream_reader *s, struct hb_event *ev)
{
	ev->player_join.id = hb_stream_reader_uint32(s);
	hb_stream_reader_string_ascii_auto(s, ev->player_join.name);
	ev->player_join.is_admin = hb_stream_reader_bool(s);
	hb_stream_reader_string_ascii_auto(s, ev->player_join.country);
}

static void parse_event_player_leave(struct hb_stream_reader *s, struct hb_event *ev)
{
	ev->player_leave.id = hb_stream_reader_uint16(s);
	ev->player_leave.kicked = hb_stream_reader_bool(s);
	ev->player_leave.reason[0] = '\0';
	if (ev->player_leave.kicked) hb_stream_reader_string_ascii_auto(s, ev->player_leave.reason);
	ev->player_leave.ban = hb_stream_reader_bool(s);
}

static void parse_event_player_chat(struct hb_stream_reader *s, struct hb_event *ev)
{
	hb_stream_reader_string_ascii_auto(s, ev->player_chat.message);
}

static void parse_event_set_player_input(struct hb_stream_reader *s, struct hb_event *ev)
{
	ev->set_player_input.input = hb_stream_reader_uint8(s);
}

static void parse_event_set_player_team(struct hb_stream_reader *s, struct hb_event *ev)
{
	ev->set_player_team.id = hb_stream_reader_uint32(s);
	ev->set_player_team.team = hb_stream_reader_team(s);
}

static void parse_event_set_teams_lock(struct hb_stream_reader *s, struct hb_event *ev)
{
	ev->set_teams_lock.teams_lock = hb_stream_reader_bool(s);
}

static void parse_event_set_game_setting(struct hb_stream_reader *s, struct hb_event *ev)
{
	ev->set_game_setting.setting_id = hb_stream_reader_uint8(s);
	ev->set_game_setting.setting_value = hb_stream_reader_uint32(s);
}

static void parse_event_set_player_avatar(struct hb_stream_reader *s, struct hb_event *ev)
{
	hb_stream_reader_string_ascii_auto(s, ev->set_player_avatar.avatar);
}

static void parse_event_set_player_admin(struct hb_stream_reader *s, struct hb_event *ev)
{
	ev->set_player_admin.id = hb_stream_reader_uint32(s);
	ev->set_player_admin.is_admin = hb_stream_reader_bool(s);
}

static void parse_event_set_stadium(struct hb_stream_reader *s, struct hb_event *ev)
{
	uint32_t chunk_size = hb_stream_reader_uint32(s);
	struct hb_stream_reader *stadium_stream = hb_stream_reader_slice(s, chunk_size);
	hb_stream_reader_inflate(stadium_stream, true);
	hb_stream_reader_stadium(stadium_stream, &ev->set_stadium.default_stadium, &ev->set_stadium.stadium);
	hb_stream_reader_free(stadium_stream);
}

static void parse_event_pause_resume_game(struct hb_stream_reader *s, struct hb_event *ev)
{
	ev->pause_resume_game.paused = hb_stream_reader_bool(s);
}

static void parse_event_ping_update(struct hb_stream_reader *s, struct hb_event *ev)
{
	ev->ping_update.ping_count = hb_stream_reader_uint8(s);
	for (size_t i = 0; i < ev->ping_update.ping_count; ++i)
		ev->ping_update.pings[i] = ((uint32_t)hb_stream_reader_uint8(s)) * 4;
}

static void parse_event_set_player_handicap(struct hb_stream_reader *s, struct hb_event *ev)
{
	ev->set_player_handicap.handicap = hb_stream_reader_uint16(s);
}

static void parse_event_set_team_shirt(struct hb_stream_reader *s, struct hb_event *ev)
{
	ev->set_team_shirt.team = hb_stream_reader_team(s);
	ev->set_team_shirt.shirt.num_colors = (size_t) hb_stream_reader_uint8(s);
	assert(ev->set_team_shirt.shirt.num_colors <= 3);
	for (size_t i = 0; i < ev->set_team_shirt.shirt.num_colors; ++i)
		ev->set_team_shirt.shirt.colors[i] = hb_stream_reader_uint32(s);
	ev->set_team_shirt.shirt.angle = (double) hb_stream_reader_uint16(s);
	ev->set_team_shirt.shirt.avatar_color = hb_stream_reader_uint32(s);
}

int hbr_next_event(struct hbr *hbr, struct hb_event *ev)
{
	struct hb_stream_reader *s = hbr->stream;

	if (s->offset >= s->len) return 0;
	if (hb_stream_reader_bool(s)) hbr->current_frame += hb_stream_reader_uint32(s);

	ev->by_player = hb_stream_reader_uint32(s);
	ev->type = hb_stream_reader_uint8(s);

	switch (ev->type) {
	case HB_EVENT_PLAYER_JOIN: parse_event_player_join(s, ev); break;
	case HB_EVENT_PLAYER_LEAVE: parse_event_player_leave(s, ev); break;
	case HB_EVENT_PLAYER_CHAT: parse_event_player_chat(s, ev); break;
	case HB_EVENT_SET_PLAYER_INPUT: parse_event_set_player_input(s, ev); break;
	case HB_EVENT_SET_PLAYER_TEAM: parse_event_set_player_team(s, ev); break;
	case HB_EVENT_SET_TEAMS_LOCK: parse_event_set_teams_lock(s, ev); break;
	case HB_EVENT_SET_GAME_SETTING: parse_event_set_game_setting(s, ev); break;
	case HB_EVENT_SET_PLAYER_AVATAR: parse_event_set_player_avatar(s, ev); break;
	case HB_EVENT_SET_PLAYER_ADMIN: parse_event_set_player_admin(s, ev); break;
	case HB_EVENT_SET_STADIUM: parse_event_set_stadium(s, ev); break;
	case HB_EVENT_PAUSE_RESUME_GAME: parse_event_pause_resume_game(s, ev); break;
	case HB_EVENT_PING_UPDATE: parse_event_ping_update(s, ev); break;
	case HB_EVENT_SET_PLAYER_HANDICAP: parse_event_set_player_handicap(s, ev); break;
	case HB_EVENT_SET_TEAM_SHIRT: parse_event_set_team_shirt(s, ev); break;

	case HB_EVENT_SET_PLAYER_DESYNC: /* No data */ break;
	case HB_EVENT_LOGIC_UPDATE: /* No data */ break;
	case HB_EVENT_START_MATCH: /* No data */ break;
	case HB_EVENT_STOP_MATCH: /* No data */ break;
	default: return 0;
	}

	return 1;
}

void hbr_free(struct hbr *hbr)
{
	hb_stream_reader_free(hbr->stream);
	free(hbr);
}
