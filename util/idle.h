#ifndef _IDLE_H_
#define _IDLE_H_

#include <lib430/common.h>
#include <lib430/timer.h>
#include <lib430/cpu/cpu.h>
#include "devices.h"

// Idle activity scheduler
class Activity;

class Idle {
protected:
	friend class Activity;

	static Activity* _activity;        // Activity chain
	static SysTimer::Future _next_due; // Next due
	static int32_t _deadline_margin;  // Don't start work within deadline margin
	static struct flags_t {
		bool active:1;               // Any activity due
		bool in_work:1;              // In work loop
	} _flags;

public:
	static void add(Activity* activity);
	static void work();              // Do idle work if any
	static void update();            // Update _next_due
	static void loop(const SysTimer::Future& f);   // Main idle "loop" - sleep or work until f
	static bool have_work() { return _activity != NULL; }
	static const SysTimer::Future& next_due() { return _next_due; }
	static void set_deadline_margin(int32_t margin) { _deadline_margin = margin; }
} _packed_;

class Activity {
protected:
	friend class Idle;

	Activity *_next;          // Next in chain, or NULL if last
	SysTimer::Future _due;    // When activity is due
	uint32_t _interval;
	struct {
		bool enabled:1;            // True if enabled
		bool repeating:1;          // Repeats on _interval
	} _flags;

public:
	Activity()
	: _next(NULL) {
	  _flags.enabled =  false;
	  _flags.repeating = false;
	}

	void enable(bool state) {
		_flags.enabled = state;
		Idle::update();
	}

	void schedule_once(const SysTimer::Future when) {
		_flags.enabled   = true;
		_flags.repeating = false;
		_due       = when;
		Idle::update();
	}

	void schedule_repeat(const SysTimer::Future when, int32_t interval) {
		_flags.enabled   = true;
		_due       = when;
		_interval  = interval;
		_flags.repeating = true;
		Idle::update();
	}

	virtual ~Activity() { }
protected:
	// Called when due
	virtual void activate() = 0;
private:
	Activity(const Activity&);
	Activity& operator=(const Activity&);
} _packed_;

#endif // _IDLE_H_
