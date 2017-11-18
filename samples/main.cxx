//
// MSP430 Attenuating Switch firmware
//
// Port usage:
//  P2.7   /EN_R     enable switch reset, active low
//  P2.6   /EN_S     enable switch set, active low
//  P2.5   IRD      IR receiver
//  P2.4   Conf0
//  P2.3   Conf1
//  P2.2   DIAL_A   rotary encoder A
//  P2.1   DIAL_B   rotary encoder B
//  P2.0   /DIAL_SW  rotary encoder switch
//
//  P1.7   USB0SDA  I2C SDA
//  P1.6   UCB0SCL  I2C SCL
//  P1.5   SW_0     switch port 0 (attenuated output)
//  P1.4   SW_1     switch port 1 (input 1)
//  P1.3   SW_2     switch port 2 (input 2)
//  P1.2   SW_3     switch port 3 (input 3)
//  P1.1   SW_5     switch port 5 (input 5)
//  P1.0   SW_4     switch port 4 (input 4)
//

#include <msp430.h> 
#include <stdint.h>

#include "config.h"
#include "lib430/i2c_master/i2c.h"
#include "lib430/pcf8574.h"
#include "lib430/dac5574.h"
#include "lib430/timer.h"
#include "lib430/ssd1306/ssd1306.h"

#define IRD_PORT_SEL   P2SEL
#define IRD_PORT_SEL2  P2SEL2
#define IRD_PORT_DIR   P2DIR
#define IRD_PORT_IES   P2IES
#define IRD_PORT_IFG   P2IFG
#define IRD_PORT_IE    P2IE
#define IRD_PORT       P2IN

#define BIT(X)  (1 << (X))

enum {
    IRD_PIN      = BIT(5),           // P2.5
    IRD_TMR_PS   = 8,
    IRD_TMR_FREQ = SMCLK / IRD_TMR_PS
};

#define IRD_COUNT_UP 1
#include "lib430/ird.h"

// I2C device addresses
enum {
    I2C_PCF8574A_ADDR   = 0x3f,
    I2C_DAC5574_ADDR    = 0b1001100, // 4c
    I2C_OLED_ADDR       = 0x78/2
};


TimerA _timer;

I2CBus _i2c_bus_master(SMCLK/I2C_BUS_SPEED);

Pcf8574 _panel(_i2c_bus_master, I2C_PCF8574A_ADDR);
Dac5574 _dac(_i2c_bus_master, I2C_DAC5574_ADDR);
Ssd1306 _oled(_i2c_bus_master, I2C_OLED_ADDR);

bool _conf0 = false;
bool _conf1 = false;

// Rotary encoder bits
#define ENC_PORT_SEL  P2SEL
#define ENC_PORT_SEL2 P2SEL2
#define ENC_PORT_DIR  P2DIR
#define ENC_PORT_IES  P2IES
#define ENC_PORT_IFG  P2IFG
#define ENC_PORT_IE   P2IE
#define ENC_PORT      P2IN

enum {
    ENC_A  = BIT(2),                 // P2.2
    ENC_B  = BIT(1),                 // P2.1
    ENC_SW = BIT(0),                 // P2.0
    ENC_ALL= ENC_A | ENC_B | ENC_SW
};

// In/out switching port defs
// S_EN,R_EN need to be on the same port (SR_PORT)
// INx,OUT need to be on the same port (IO_PORT)

#define SR_PORT_SEL  P2SEL
#define SR_PORT_SEL2 P2SEL2
#define IO_PORT_SEL  P1SEL
#define IO_PORT_SEL2 P1SEL2
#define SR_PORT      P2OUT
#define IO_PORT      P1OUT

enum {
    R_EN     = BIT(7),               // P2.7
    S_EN     = BIT(6),               // P2.6
    P_OUT    = BIT(5),               // P1.5
    P_IN1    = BIT(4),               // P1.4
    P_IN2    = BIT(3),               // P1.3
    P_IN3    = BIT(2),               // P1.2
    P_IN4    = BIT(0),               // P1.0
    P_IN5    = BIT(1),               // P1.1
    P_INx    = (P_IN1 | P_IN2 | P_IN3 | P_IN4 | P_IN5),
    RELAY_HOLD     = TIMER_MSEC(5),               // Relay hold time
    RELAY_RECOVERY = TIMER_MSEC(20)               // Relay recovery time
};

#define CONF_PORT_SEL   P2SEL
#define CONF_PORT_SEL2  P2SEL2
#define CONF_PORT_DIR   P2DIR
#define CONF_PORT       P2IN

enum {
    CONF_0 = BIT(4),
    CONF_1 = BIT(3)
};
    
// Current input
static uint8_t _cur_input = 0;

static void init() {
    WDTCTL  = WDTPW | WDTHOLD;  // Stop watchdog timer
    BCSCTL1 = XT2OFF | CALBC1_16MHZ;      // Run DCO at 16MHz
    BCSCTL2 = DIVS_3;                   // SMCLK divide by 8
    DCOCTL  = CALDCO_16MHZ;

    // Make sure EN_S and EN_R come on active high
    P1OUT = 0;
    P2OUT = 0;
    SR_PORT |= R_EN | S_EN;

    // Default ports to output
    P1DIR = 0xff;
    P2DIR = 0xff;
    
    // Configuration pins are inputs
    CONF_PORT_DIR &= ~(CONF_0 | CONF_1);
    _conf0 = (CONF_PORT & CONF_0) != 0;
    _conf1 = (CONF_PORT & CONF_1) != 0;

    // Configure P1.6 as SCL, P1.7 as SDA
    P1SEL  |= BIT6 | BIT7;
    P1SEL2 |= BIT6 | BIT7;

    // Configure P1,P2 port pins as digital I/O
    SR_PORT |= S_EN | R_EN;      // Disable - active low
    IO_PORT &= ~(P_OUT | P_INx); // Disable - active high

    SR_PORT_SEL  &= ~(S_EN | R_EN);
    SR_PORT_SEL2 &= ~(S_EN | R_EN);
    IO_PORT_SEL  &= ~(P_OUT | P_INx);
    IO_PORT_SEL2 &= ~(P_OUT | P_INx);

    // Configure IRD pin as digital I/O input
    IRD_PORT_SEL  &= ~IRD_PIN;
    IRD_PORT_SEL2 &= ~IRD_PIN;
    IRD_PORT_DIR  &= ~IRD_PIN;
    
    // Enable interrupt on IRD falling edge
    IRD_PORT_IFG &= ~IRD_PIN;    // Clear any pending interrupt
    IRD_PORT_IES |= IRD_PIN;
    IRD_PORT_IE  |= IRD_PIN;
    
    // Configure ENC pins as digital I/O input
    ENC_PORT_SEL  &= ~ENC_ALL;
    ENC_PORT_SEL2 &= ~ENC_ALL;
    ENC_PORT_DIR  &= ~ENC_ALL;

    // Enable interrupt on ENC A rising edge, SW falling edge
    ENC_PORT_IFG &= ~ENC_ALL;
    ENC_PORT_IES &= ~ENC_A;
    ENC_PORT_IES |= ENC_SW;
    ENC_PORT_IE  |= ENC_A | ENC_SW;

    // Configure timer 1
    TA1CTL = TASSEL_2 | ID_3; // Prescale by 8
    __enable_interrupt();
    _timer.init();
    ird_reset();
    _i2c_bus_master.init();
}


// Reset a specific input
static void reset_relay(uint8_t n) {
    return;

    SR_PORT |= S_EN;
    SR_PORT &= ~R_EN;
    IO_PORT = (IO_PORT & ~P_INx) | n;
    _timer.delay(RELAY_HOLD);
    SR_PORT |= R_EN | S_EN;
    IO_PORT &= ~P_INx;
    _timer.delay(RELAY_RECOVERY);
}

// Set a specific switch
static void set_relay(uint8_t n) {
    return;

    SR_PORT |= R_EN;
    SR_PORT &= ~S_EN;
    IO_PORT = (IO_PORT & ~P_INx) | n;
    _timer.delay(RELAY_HOLD);
    SR_PORT |= R_EN | S_EN;
    IO_PORT &= ~P_INx;
    _timer.delay(RELAY_RECOVERY);
}

// Reset all inputs
static void reset_relays() {
    reset_relay(P_OUT);
    reset_relay(P_IN1);
    reset_relay(P_IN2);
    reset_relay(P_IN3);
    reset_relay(P_IN4);
    reset_relay(P_IN5);

    _cur_input = 0;
    _panel.set(~_cur_input);
}

// Select specific input
static void change_input(const uint8_t new_input) {
    if (new_input == _cur_input)
        return;

    // Reset previous input
    if (_cur_input) {
        reset_relay(_cur_input);
    }

    // Select new input
    set_relay(new_input);

    _cur_input = new_input;
    _panel.set(~_cur_input);
}

void ird_start_timer(const uint16_t max_count) {
    // Start timer 1 counting up to max_count and interrupting if it does
    TA1CTL  |= TACLR;
    // Prescaled by IRD_TMR_PS (8)
    TA1CTL   = (TA1CTL & ~MC_3) | TASSEL_2 | MC_2 | ID_3; // Mode 2: continuous, use SMCLK
    TA1CCR0  = max_count;
    TA1CCTL0 = CCIE;
}

static void port_intr() {
    // Check IR decoder pin
    if (IRD_PORT_IFG & IRD_PIN) {
        IRD_PORT_IFG &= ~IRD_PIN;

        const uint8_t pin_state = IRD_PORT & IRD_PIN;

        // Get timer value and stop
        const uint16_t count = TA1R;
        TA1CTL |= TACLR;
        TA1CTL    = MC_0; // Mode 0: stopped
        TA1CCTL0 &= ~CCIE;

        if (pin_state != 0) {
            ird_rise(count);
            IRD_PORT_IES |= IRD_PIN;
        } else {
            ird_fall(count);
            IRD_PORT_IES &= ~IRD_PIN;
        }
        IRD_PORT_IE  |= IRD_PIN;
    }

    // Check encoder pins
    if (ENC_PORT_IFG & ENC_SW) {
        ENC_PORT &= ~ENC_SW;

        ird_repeat = 1;
        ird_command = IRD_CMD_POWER;
    }

    if (ENC_PORT_IFG & ENC_A) {
        ENC_PORT_IFG &= ~ENC_A;

        const uint8_t enc_a = (ENC_PORT & ENC_A) / ENC_A;
        const uint8_t enc_b = (ENC_PORT & ENC_B) / ENC_B;

        ird_repeat = 1;
        if (enc_a == enc_b) {
            ird_command = IRD_CMD_VOL_DN;
        } else {
            ird_command = IRD_CMD_VOL_UP;
        }

        // Swap port pin edge sensitivity
        if (enc_a) {
            ENC_PORT_IES |= ENC_A;
        } else {
            ENC_PORT_IES &= ~ENC_A;
        }
        ENC_PORT_IE |= ENC_A;
    }
}

#pragma vector=PORT1_VECTOR
__interrupt void _port1_intr() {
    port_intr();
}

#pragma vector=PORT2_VECTOR
__interrupt void _port2_intr() {
    port_intr();
}

// Interrupt for Timer1 CC0
#pragma vector=TIMER1_A0_VECTOR
__interrupt void __timer1_intr() {
    TA1CTL |= TACLR;
    TA1CTL  = MC_0; // Mode 0: stopped

    ird_timeout();
}

enum {
    VOL_STEP = 10,
    MAX_VOLUME = 100
};

int8_t _volume = 0;             // 0-127

static void render(int n) {
    if (_oled.state() != I2CDevice::ATTACHED) {
        return;
    }

    int x = Ssd1306::PANEL_WIDTH - RUNE_BLANK_WIDTH - 8;
    int8_t v = MAX_VOLUME - _volume;
    do {
        const int rune = RUNE_DIGIT0 + v % 10;
        _oled.render(x, 48, (Rune)rune, RUNE_BLANK_WIDTH);
        x -= RUNE_BLANK_WIDTH + 8;
        v /= 10;
    } while (v);
    if (_volume < 100) {
        _oled.render(x, 48, (Rune)RUNE_BLANK, RUNE_BLANK_WIDTH);
    }
}

#define DACVOL(V) ((V)+(V)+(V)/2)
#define DACVOL_MAX  DACVOL(MAX_VOLUME)

static void update_vol_dac() {
    _dac.update_all(DACVOL(_volume), DACVOL(_volume), DACVOL(_volume), DACVOL(_volume));
}

// amount = 0 will refresh the display without changing volume
static void change_volume(int8_t amount) {
    if (_volume >= MAX_VOLUME && amount > 0)
        return;

    if (_volume == 0 && amount < 0) {
        return;
    }

    _volume += amount * VOL_STEP;
    render(_volume);

    if (amount) {
        update_vol_dac();
    }
}

uint32_t _sec = 0;

static void i2c_bus_probe() {
    _dac.probe();
    _oled.probe();
    _panel.probe();
}

static void idle_heartbeat() {
    i2c_bus_probe();

    change_volume(0);
}

int main(void) {
	init();
	
	i2c_bus_probe();

	reset_relays();

    _panel.set(~0);
	_oled.clear();

    update_vol_dac();
    _timer.delay(TIMER_MSEC(10));  // Wait for LDRs to update before unmuting

	set_relay(P_OUT);
	change_input(P_IN1);

    for (;;) {
        if (_timer.ticks() >= TIMER_SEC(2)) {
            ++_sec;
            _timer.remove(TIMER_SEC(1));
            idle_heartbeat();
        }

        if (ird_repeat) {
            bool repeat = true;
            switch (ird_command) {
            case IRD_CMD_1: change_input(P_IN1); repeat = false; break;
            case IRD_CMD_2: change_input(P_IN2); repeat = false; break;
            case IRD_CMD_3: change_input(P_IN3); repeat = false; break;
            case IRD_CMD_4: change_input(P_IN4); repeat = false; break;
            case IRD_CMD_5: change_input(P_IN5); repeat = false; break;
            //case IRD_CMD_POWER: reset_inputs(); break;
            case IRD_CMD_VOL_UP: change_volume(ird_repeat); ird_repeat = 0; break;
            case IRD_CMD_VOL_DN: change_volume(-ird_repeat); ird_repeat = 0; break;
            }
            if (!repeat) {
                ird_command = IRD_CMD_NONE;
                ird_repeat = 0;
            }
        }
    }
}
