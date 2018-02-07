#include "idle.h"

Activity* Idle::_activity = NULL;
SysTimer::Future Idle::_next_due;

Idle::flags_t Idle::_flags;

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
				// so it can schedule itself
				a->activate();

				// Only run one activity per invocation
//				break;
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
