/* 
 * IR decoder.  This decodes an Adafruit "mini remote".  The NEC 38kHz
 * demodulated protocol used by it consists of two distinct sequences:
 *
 * 1. Command: issues a command
 * 2. Repeat: repeats the last command
 *
 * We decode these into two variables:
 *   ird_command - the last command
 *   ird_repeat  - command repeat count (when the button is held)
 *
 * When a button is pressed the command is sent, followed by an
 * execute signal.  As long as the key is held down it will continue
 * sending execute signals at a pre- defined interval.  The command
 * contains a 16 bit identifier we ignore and an 8 bit command code
 * repeated twice (it's inverted the second time).
 * 
 * The signal levels here are inverted, meaning the IR demodulator
 * holds the output high except for when it receives a 38kHz
 * "carrier", at point it pulls down the output.
 * 
 * SPACE is what we call the high idle state. Both the command and
 * repeat sequences begin with a MARK, is a long negative.  On receipt
 * of a MARK the IR decoder here will drop what it's doing, no matter
 * what state it's in, and reset its state to IRD_MARK indicating it
 * has received a mark - and prepare to decode a command or repeat
 * pulse.  If the mark is followed by a short positive it's a repeat
 * sequence.  This increments the repeat counter.
 * 
 * If the mark is followed by a longer positive, it's a command
 * sequence.  The length of edge-to-edge transitions determine whether
 * it's a 0 or 1.  The 8 bit command is repeated twice in the last 16
 * bits, once inverted.  We check that this is the case, to make sure
 * we don't have a receive error.
 * 
 * The decoder uses a function ird_timer_start() that should start a
 * timer for a given max interval (in units of 0.1 msec).  The
 * interrupt dispatcher should call for the following conditions:
 *
 *    ird_rise(interval)     used to signal a positive edge, at 'interval'
 *                           (timer ticks) since the previous fall
 *    ird_fall(interval)     used to signal a falling edge, at 'interval'
 *                           (timer ticks) since the previous rise
 *    ird_timeout()          used to signal the timer ran down
 *
 * All of these should leave the interval timer stopped.  The decoder
 * will call ird_timer_start() as needed to start it up again.
 *
 * NOTE: to detect the end of the command it uses the timer timeout
 * (ird_timeout()).  Instead, it should use a pulse counter, and count
 * down from 32.  Then use the timer to simply time out the receiver.
 * It should probably also check the first 16 bits to make sure the
 * remote identifier is correct, or it might see all sorts of other
 * remotes.  This would be easy with the addition of the pulse counter
 * mentioned.
 *
 * It also needs a constant (#define or enum):
 *   IRD_TMR_FREQ - the frequency of the timer used, in Hz
 *
 */

#ifndef IRD_H
#define	IRD_H

enum IrCommand {
    IRD_CMD_NONE        = 0xff,
    IRD_CMD_VOL_DN      = 0x00,
    IRD_CMD_VOL_UP      = 0x40,
    IRD_CMD_PLAY_PAUSE  = 0x80,
    IRD_CMD_SETUP       = 0x20,
    IRD_CMD_STOP_MODE   = 0x60,
    IRD_CMD_ENTER_SAVE  = 0x90,
    IRD_CMD_BACK        = 0x70,
    IRD_CMD_UP          = 0xa0,
    IRD_CMD_LEFT        = 0x10,
    IRD_CMD_RIGHT       = 0x50,
    IRD_CMD_DOWN        = 0xb0,
    IRD_CMD_0_10        = 0x30,
    IRD_CMD_1           = 0x08,
    IRD_CMD_2           = 0x88,
    IRD_CMD_3           = 0x48,
    IRD_CMD_4           = 0x28,
    IRD_CMD_5           = 0xa8,
    IRD_CMD_6           = 0x68,
    IRD_CMD_7           = 0x18,
    IRD_CMD_8           = 0x98,
    IRD_CMD_9           = 0x58,
    
    IRD_CMD_POWER       = IRD_CMD_STOP_MODE
};

#define IRD_IS_DIGIT(CMD)  (((uint8_t)(CMD)) & 0x0f) != 0)

enum IrdState {
    IRD_STATE_IDLE = 0,         // Doing nothing, timer is stopped
    IRD_STATE_MARK,             // Received a mark
    IRD_STATE_COMMAND           // Reading a command
};


// Timer count for msec expressed as a rational (A/B)
#define IRD_MSEC(A,B)  (uint16_t)((uint32_t)(A)*(uint32_t)(IRD_TMR_FREQ)/((uint32_t)(B)*1000UL))

enum {
    IRD_MAX_INTERVAL      = IRD_MSEC(115,10), // 11.5ms max timeout (0.5ms more than the longest pulse)
    IRD_MARK_INTERVAL_MIN = IRD_MSEC(8,1),  // A mark is a rising edge after 8-11ms (meas:9.1ms)
    IRD_MARK_INTERVAL_MAX = IRD_MSEC(11,1),
    IRD_REPEAT_SPACE_MAX = IRD_MSEC(3,1),  // If the space following mark is < 3ms it's a repeat (meas:2.2ms))
    IRD_COMMAND_SPACE_MIN = IRD_MSEC(4,1),  // If the space followin mark is > 4ms it's a command (meas:4.4ms)
    IRD_COMMAND_0_MIN     = IRD_MSEC(1,3),  // Lower bound on the zero: 0.3ms (meas:0.6ms))
    IRD_COMMAND_0_MAX     = IRD_MSEC(1,1),  // If the space following mark is < 1ms it's a zero
    IRD_COMMAND_1_MIN     = IRD_MSEC(12,10),// If the space following mark is > 1.2ms and < 2.5ms it's a one (meas:1.7ms)
    IRD_COMMAND_1_MAX     = IRD_MSEC(25,10),// Upper bound on space for one (2.5ms)
    IRD_COMMAND_DONE      = IRD_MSEC(6,1),  // Command is finished if nothing is received in 6ms
    IRD_STROBE_MAX        = IRD_MSEC(9,10)  // Max 0.9ms negative between command bits and on repeat tail (meas:0.6ms))
};


void ird_start_timer(uint16_t max_count);

// Current command, repeat counter
static volatile enum IrCommand ird_command;
static volatile uint8_t ird_repeat;

static enum IrdState ird_state;
static uint16_t ird_command_bits;

static void ird_reset() {
    ird_state = IRD_STATE_IDLE;
    ird_command = IRD_CMD_NONE;
    ird_repeat = 0;
    ird_command_bits = 0;
}

inline void ird_timeout() {
    switch (ird_state) {
        case IRD_STATE_COMMAND:
            // Use timeout to detect end-of-command
            if ((ird_command_bits >> 8) == (~ird_command_bits & 0xff)) {
                ird_command = IrCommand(~ird_command_bits & 0xff);
                ird_repeat = 1;
            }
            ird_state = IRD_STATE_IDLE;
            break;
        default:
            ird_reset();
            break;
    }
}

#define ird_run()  ird_start_timer(IRD_MAX_INTERVAL)

static void ird_rise(uint16_t interval) {
#ifndef IRD_COUNT_UP
    interval = IRD_MAX_INTERVAL - interval;
#endif

    // Always handle a MARK as a state reset to STATE_MARK, no matter what state we're in
    if (interval >= IRD_MARK_INTERVAL_MIN && interval < IRD_MARK_INTERVAL_MAX) {
        ird_state = IRD_STATE_MARK;
        ird_run();
        return;
    }
    
    switch (ird_state) {
        case IRD_STATE_IDLE:
            // Short negative when idle - ignore.  The actual mark is special cased
            // above.
            return;
        case IRD_STATE_COMMAND:
            ird_run();
            break;
        default:
            ird_reset();
    }
    if (ird_state != IRD_STATE_IDLE)
        ird_start_timer(IRD_MAX_INTERVAL);
}

static void ird_fall(uint16_t interval) {
#ifndef IRD_COUNT_UP
    interval = IRD_MAX_INTERVAL - interval;
#endif

    switch (ird_state) {
        case IRD_STATE_IDLE:
            ird_run();
            return;
        case IRD_STATE_MARK:
            if (interval < IRD_REPEAT_SPACE_MAX) {
                ++ird_repeat;
                ird_state = IRD_STATE_IDLE;
            } else if (interval >= IRD_COMMAND_SPACE_MIN) {
                ird_state = IRD_STATE_COMMAND;
                ird_command_bits = 0;
            }
            break;
        case IRD_STATE_COMMAND:
            ird_command_bits <<= 1;
            if (interval >= IRD_COMMAND_1_MIN && interval < IRD_COMMAND_1_MAX) {
                ird_command_bits |= 1;
            } else if (interval < IRD_COMMAND_0_MIN || interval >= IRD_COMMAND_0_MAX) {
                ird_reset();
            }
            break;
        default:
            ird_reset();
            break;  
    }
    
    if (ird_state != IRD_STATE_IDLE)
        ird_run();
}


#endif	/* IRD_H */
