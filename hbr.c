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
#include "stream_reader.h"
#include "actions.h"
#include "hbr.h"

static enum hb_team hb_stream_reader_team(struct hb_stream_reader *s)
{
	switch (hb_stream_reader_uint8(s)) {
	case 0:  return HB_TEAM_BLUE;
	case 1:  return HB_TEAM_RED;
	default: return HB_TEAM_SPECTATOR;
	}
}

static uint32_t hb_stream_reader_color(struct hb_stream_reader *s)
{
	uint32_t color = hb_stream_reader_uint32(s);
	if (color == 0xffffffff) color = 0x00000000;
	else color |= 0xff << 24;
	return color;
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
	disc->color              = hb_stream_reader_color(s);
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
	segment->color           = hb_stream_reader_color(s);
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
	bg->color                = hb_stream_reader_color(s);
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


static struct hb_shirt hb_stream_reader_team_colors(struct hb_stream_reader *s)
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

struct hb_hbr *hb_hbr_parse(const char *path)
{
	struct hb_hbr *hbr = calloc(1, sizeof(*hbr));
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
	hbr->red_shirt = hb_stream_reader_team_colors(s);
	hbr->blue_shirt = hb_stream_reader_team_colors(s);

	return hbr;
}

static struct hb_action_generic *hb_stream_reader_action_player_join(struct hb_stream_reader *s)
{
	struct hb_action_player_join *act = malloc(sizeof(*act));
	act->id = hb_stream_reader_uint32(s);
	hb_stream_reader_string_ascii_auto(s, act->name);
	act->is_admin = hb_stream_reader_bool(s);
	hb_stream_reader_string_ascii_auto(s, act->country);
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_player_leave(struct hb_stream_reader *s)
{
	struct hb_action_player_leave *act = malloc(sizeof(*act));
	act->id = hb_stream_reader_uint16(s);
	act->kicked = hb_stream_reader_bool(s);
	act->reason[0] = '\0';
	if (act->kicked) hb_stream_reader_string_ascii_auto(s, act->reason);
	act->ban = hb_stream_reader_bool(s);
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_player_chat(struct hb_stream_reader *s)
{
	struct hb_action_player_chat *act = malloc(sizeof(*act));
	hb_stream_reader_string_ascii_auto(s, act->message);
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_logic_update(__attribute((unused)) struct hb_stream_reader *s)
{
	struct hb_action_logic_update *act = malloc(sizeof(*act));
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_start_match(__attribute((unused)) struct hb_stream_reader *s)
{
	struct hb_action_start_match *act = malloc(sizeof(*act));
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_stop_match(__attribute((unused)) struct hb_stream_reader *s)
{
	struct hb_action_stop_match *act = malloc(sizeof(*act));
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_set_player_input(struct hb_stream_reader *s)
{
	struct hb_action_set_player_input *act = malloc(sizeof(*act));
	act->input = hb_stream_reader_uint8(s);
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_set_player_team(struct hb_stream_reader *s)
{
	struct hb_action_set_player_team *act = malloc(sizeof(*act));
	act->id = hb_stream_reader_uint32(s);
	act->team = hb_stream_reader_team(s);
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_set_teams_lock(struct hb_stream_reader *s)
{
	struct hb_action_set_teams_lock *act = malloc(sizeof(*act));
	act->teams_lock = hb_stream_reader_bool(s);
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_set_game_setting(struct hb_stream_reader *s)
{
	struct hb_action_set_game_setting *act = malloc(sizeof(*act));
	act->setting_id = hb_stream_reader_uint8(s);
	act->setting_value = hb_stream_reader_uint32(s);
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_set_player_avatar(struct hb_stream_reader *s)
{
	struct hb_action_set_player_avatar *act = malloc(sizeof(*act));
	hb_stream_reader_string_ascii_auto(s, act->avatar);
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_set_player_desync(__attribute((unused)) struct hb_stream_reader *s)
{
	struct hb_action_set_player_desync *act = malloc(sizeof(*act));
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_set_player_admin(struct hb_stream_reader *s)
{
	struct hb_action_set_player_admin *act = malloc(sizeof(*act));
	act->id = hb_stream_reader_uint32(s);
	act->is_admin = hb_stream_reader_bool(s);
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_set_stadium(struct hb_stream_reader *s)
{
	struct hb_action_set_stadium *act = malloc(sizeof(*act));
	uint32_t chunk_size = hb_stream_reader_uint32(s);
	struct hb_stream_reader *stadium_stream = hb_stream_reader_slice(s, chunk_size);
	hb_stream_reader_inflate(stadium_stream, true);
	hb_stream_reader_stadium(stadium_stream, &act->default_stadium, &act->stadium);
	hb_stream_reader_free(stadium_stream);
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_pause_resume_game(struct hb_stream_reader *s)
{
	struct hb_action_pause_resume_game *act = malloc(sizeof(*act));
	act->paused = hb_stream_reader_bool(s);
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_ping_update(struct hb_stream_reader *s)
{
	struct hb_action_ping_update *act = malloc(sizeof(*act));
	act->ping_count = hb_stream_reader_uint8(s);
	for (size_t i = 0; i < act->ping_count; ++i)
		act->pings[i] = ((uint32_t)hb_stream_reader_uint8(s)) * 4;
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_set_player_handicap(struct hb_stream_reader *s)
{
	struct hb_action_set_player_handicap *act = malloc(sizeof(*act));
	act->handicap = hb_stream_reader_uint16(s);
	return (void *)act;
}

static struct hb_action_generic *hb_stream_reader_action_set_team_colors(struct hb_stream_reader *s)
{
	struct hb_action_set_team_colors *act = malloc(sizeof(*act));
	act->team = hb_stream_reader_team(s);
	act->num_stripes = hb_stream_reader_uint8(s);
	assert(act->num_stripes <= 3 && act->num_stripes >= 0);
	for (size_t i = 0; i < act->num_stripes; ++i)
		act->stripes[i] = hb_stream_reader_uint32(s);
	act->angle = hb_stream_reader_uint16(s);
	act->avatar_color = hb_stream_reader_uint32(s);
	return (void *)act;
}

struct hb_action_generic *hb_hbr_next_action(struct hb_hbr *hbr)
{
	struct hb_stream_reader *s = hbr->stream;
	if (hbr->stream->offset >= hbr->stream->len)
		return NULL;
	bool is_new_frame = hb_stream_reader_bool(s);
	if (is_new_frame) hbr->current_frame += hb_stream_reader_uint32(s);

	struct hb_action_generic *act;
	uint32_t by_player = hb_stream_reader_uint32(s);
	uint8_t action_id = hb_stream_reader_uint8(s);

	switch (action_id) {
	case HB_REPLAY_ACTION_PLAYER_JOIN: act = hb_stream_reader_action_player_join(s); break;
	case HB_REPLAY_ACTION_PLAYER_LEAVE: act = hb_stream_reader_action_player_leave(s); break;
	case HB_REPLAY_ACTION_PLAYER_CHAT: act = hb_stream_reader_action_player_chat(s); break;
	case HB_REPLAY_ACTION_LOGIC_UPDATE: act = hb_stream_reader_action_logic_update(s); break;
	case HB_REPLAY_ACTION_START_MATCH: act = hb_stream_reader_action_start_match(s); break;
	case HB_REPLAY_ACTION_STOP_MATCH: act = hb_stream_reader_action_stop_match(s); break;
	case HB_REPLAY_ACTION_SET_PLAYER_INPUT: act = hb_stream_reader_action_set_player_input(s); break;
	case HB_REPLAY_ACTION_SET_PLAYER_TEAM: act = hb_stream_reader_action_set_player_team(s); break;
	case HB_REPLAY_ACTION_SET_TEAMS_LOCK: act = hb_stream_reader_action_set_teams_lock(s); break;
	case HB_REPLAY_ACTION_SET_GAME_SETTING: act = hb_stream_reader_action_set_game_setting(s); break;
	case HB_REPLAY_ACTION_SET_PLAYER_AVATAR: act = hb_stream_reader_action_set_player_avatar(s); break;
	case HB_REPLAY_ACTION_SET_PLAYER_DESYNC: act = hb_stream_reader_action_set_player_desync(s); break;
	case HB_REPLAY_ACTION_SET_PLAYER_ADMIN: act = hb_stream_reader_action_set_player_admin(s); break;
	case HB_REPLAY_ACTION_SET_STADIUM: act = hb_stream_reader_action_set_stadium(s); break;
	case HB_REPLAY_ACTION_PAUSE_RESUME_GAME: act = hb_stream_reader_action_pause_resume_game(s); break;
	case HB_REPLAY_ACTION_PING_UPDATE: act = hb_stream_reader_action_ping_update(s); break;
	case HB_REPLAY_ACTION_SET_PLAYER_HANDICAP: act = hb_stream_reader_action_set_player_handicap(s); break;
	case HB_REPLAY_ACTION_SET_TEAM_COLORS: act = hb_stream_reader_action_set_team_colors(s); break;
	default: return NULL;
	}

	act->by_player = by_player;
	act->type = action_id;

	return act;
}

void hb_hbr_free(struct hb_hbr *hbr)
{
	hb_stream_reader_free(hbr->stream);
	free(hbr);
}

void hb_action_free(struct hb_action_generic *action)
{
	free(action);
}
