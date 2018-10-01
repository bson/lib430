// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#include "idle.h"
#include "../task.h"

Activity* Idle::_activity = NULL;
SysTimer::Future Idle::_next_due;
int32_t Idle::_deadline_margin = 0;

Idle::flags_t Idle::_flags;

void Idle::loop(const SysTimer::Future& f) {
    if (!_activity) {
        Task::wait(f);
        return;
    }

    SysTimer::Future deadline = f;
    deadline.adjust(_deadline_margin);

    while (!SysTimer::due(deadline)) {
        Task::wait(min(f, _next_due));

        if (SysTimer::due(_next_due))
            work();
    }

    if (_deadline_margin)
        Task::wait(deadline);
}

void Idle::add(Activity* activity) {
	activity->_next = _activity;
	_activity = activity;
	update();
}

void Idle::work() {
	if (_flags.active && SysTimer::due(_next_due)) {
		_flags.in_work = true;
		// Activate all that are due - could be multiple
		for (Activity* a = _activity; a; a = a->_next) {
			if (a->_flags.enabled && SysTimer::due(a->_due)) {
				if (a->_flags.repeating) {
					a->_due.adjust(a->_interval);
				} else {
					a->_flags.enabled = false;
				}
				// Important: run activity after calculations above
				// so it can reschedule itself
				a->activate();
			}
		}
		_flags.in_work = false;
		update();
	}
}

void Idle::update() {
	if (_flags.in_work)
		return;

	Activity* first_due = NULL;
	for (Activity* a = _activity; a; a = a->_next) {
		if (a->_flags.enabled && (!first_due || a->_due < first_due->_due)) {
			first_due = a;
		}
	}
	if ((_flags.active = first_due)) {
		_next_due = first_due->_due;
	}
}


