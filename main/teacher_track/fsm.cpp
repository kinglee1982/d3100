#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "fsm.h"
#include "cJSON.h"
#include "utils.h"

static double util_now()
{
	return now();
}

FSMEvent::FSMEvent(int id, const char *name)
	: id_(id)
	, name_(name)
{
	static Lock _lock;
	static int _token = 0;

	Autolock al(_lock);
	token_ = ++_token;
	stamp_ = util_now();
}

static std::vector<DetectionEvent::Rect> _parse_rects(cJSON *j)
{
	std::vector<DetectionEvent::Rect> rcs;

	cJSON *c = j->child;

	while (c) {
		cJSON *r = c->child;

		DetectionEvent::Rect rc;

		while (r) {
			if (!strcmp("x", r->string))
			{
				rc.x = r->valueint;
			}
				
			if (!strcmp("y", r->string))
				rc.y = r->valueint;
			if (!strcmp("width", r->string))
				rc.width = r->valueint;
			if (!strcmp("height", r->string))
				rc.height = r->valueint;

			r = r->next;
		}

		rcs.push_back(rc);

		c = c->next;
	}

	return rcs;
}

void DetectionEvent::parse_json(const char *str)
{
	if (who_ == "teacher") {
		/*
			{"stamp":12345,"rect":[{"x":0,"y":0,"width":100,"height":100},
	                               {"x":0,"y":0,"width":100,"height":100}]}
		 */
		cJSON *j = cJSON_Parse(str);
		if (j) {
			cJSON *c = j->child;
			while (c) {
				if (!strcmp("stamp", c->string)) {
					stamp_ = c->valuedouble;
				}
				else if (!strcmp("rect", c->string)) {
					targets_ = _parse_rects(c);
				}

				c = c->next;
			}
			cJSON_Delete(j);
		}		
	}
}

static void dump_all_state(const std::vector<FSMState*> &states)
{
	debug("fsm", "There are %u STATE\n", states.size());
	for (size_t i = 0; i < states.size(); i++) {
		debug("fsm", "\t##%d: %s\n",
				states[i]->id(), states[i]->name());
	}
}

void FSM::run(int state_start, int state_end, bool *quit)
{
	dump_all_state(states_);

	int next_state = state_start;
	FSMState *state0 = 0;
	while (!(*quit) && next_state != state_end) {
		FSMState *state = find_state(next_state);
		if (!state) {
			fatal("fsm", "can't find FSMState of %d\n", state_start);
		}

		if (state != state0) {
			info("fsm", "CHANGED: '%s'==>'%s'\n",
					state0 ? state0->name() : "None", state->name());

			if (state0) state0->when_leave(state->id());
			state->when_enter(state0 ? state->id() : state->id());

			state0 = state;
		}

		double curr = now();

		FSMEvent *evt = next_event(curr);
		if (evt) {
			//debug("fsm", "[%s]: evt=%d, name=%s\n", state->name(), evt->id(), evt->name());
			switch (evt->id()) {
				case EVT_PTZ_Completed:
					next_state = state->when_ptz_completed((PtzCompleteEvent*)evt);
					break;

				case EVT_Detection:
					next_state = state->when_detection((DetectionEvent*)evt);
					break;

				case EVT_Udp:
					next_state = state->when_udp((UdpEvent*)evt);
					break;

				default:
					next_state = state->when_custom_event(evt);
					break;
			}

			delete evt;
		}
		else {
			next_state = state->when_timeout(curr);  // 超时事件 ...
		}
	}

	if (*quit) {
		info("fsm", "normal quit ...!!!\n");
	}

	if (next_state == state_end) {
		warning("fsm", "sss to terminal state!!!\n");
	}
}

void FSM::cancel_event(int token)
{
	Autolock al(lock_);

	std::deque<std::pair<double, FSMEvent*> >::iterator it;
	for (it = fifo_ptz_complete_.begin(); it != fifo_ptz_complete_.end(); ) {
		if (it->second->token() == token) {
			delete it->second;
			it = fifo_ptz_complete_.erase(it);
		}
		else ++it;
	}

	std::deque<DetectionEvent*>::iterator it2;
	for (it2 = fifo_detection_.begin(); it2 != fifo_detection_.end(); ) {
		if ((*it2)->token() == token) {
			delete *it2;
			it2 = fifo_detection_.erase(it2);
		}
		else ++it2;
	}

	std::deque<UdpEvent*>::iterator it3;
	for (it3 = fifo_udp_.begin(); it3 != fifo_udp_.end(); ) {
		if ((*it3)->token() == token) {
			delete *it3;
			it3 = fifo_udp_.erase(it3);
		}
		else ++it3;
	}
}

