#ifndef R1
#error Do not build this directly. Build either ups-5v.ino or ups-12v.in0
#endif

#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE ( 16 )
#endif

#define VCONV(x) (x * R2 / (R1+R2))
#define VCONV_DIG(x) VCONV(x)*256/2.56

#define ON_THRESH uint8_t(VCONV_DIG(ON_THRESH_DESIRED))
#define POWERUP_THRESH uint8_t(VCONV_DIG(POWERUP_THRESH_DESIRED))
#define OFF_THRESH uint8_t(VCONV_DIG(OFF_THRESH_DESIRED))
#define SHUTDOWN_THRESH uint8_t(VCONV_DIG(SHUTDOWN_THRESH_DESIRED))

#define I2C_SLAVE_ADDRESS 0x4

#define REG_VIN_LOW 2
#define REG_VIN_HIGH 3
#define REG_VUPS_LOW 4
#define REG_VUPS_HIGH 5
#define REG_MOSFET 6
#define REG_ON_THRESH 7
#define REG_OFF_THRESH 8
#define REG_COUNTDOWN 9
#define REG_STATE 10
#define REG_CYCLE_DELAY 11
#define REG_FAIL_SHUTDOWN_DELAY 12
#define REG_RUN_COUNTER 13
#define REG_POWERUP_THRESH 14
#define REG_SHUTDOWN_THRESH 15
#define REG_R1 16
#define REG_R2 17

#define VAL_MOSFET i2c_regs[REG_MOSFET]
#define VAL_VIN_HIGH i2c_regs[REG_VIN_HIGH]
#define VAL_VUPS_HIGH i2c_regs[REG_VUPS_HIGH]
#define VAL_ON_THRESH i2c_regs[REG_ON_THRESH]
#define VAL_OFF_THRESH i2c_regs[REG_OFF_THRESH]
#define VAL_COUNTDOWN i2c_regs[REG_COUNTDOWN]
#define VAL_STATE i2c_regs[REG_STATE]
#define VAL_CYCLE_DELAY i2c_regs[REG_CYCLE_DELAY]
#define VAL_FAIL_SHUTDOWN_DELAY i2c_regs[REG_FAIL_SHUTDOWN_DELAY]
#define VAL_RUN_COUNTER i2c_regs[REG_RUN_COUNTER]
#define VAL_POWERUP_THRESH i2c_regs[REG_POWERUP_THRESH]

#define PIN_MOSFET (1<<PB1)

#define STATE_DISABLED 0
#define STATE_WAIT_OFF 1
#define STATE_WAIT_ON 2
#define STATE_POWERUP 3
#define STATE_RUNNING 4
#define STATE_FAIL_SHUTDOWN 5
#define STATE_FAIL_SHUTDOWN_DELAY 6
#define STATE_CYCLE_DELAY 7

#define ERROR_CRC 0xFF
#define ERROR_SIZE 0xFE
#define ERROR_UNINITIALIZED 0xFD
#define ERROR_SUCCESS 0

#define MAX_SAMPLES 8

volatile uint8_t i2c_regs[] =
{
    0x0,        // reserved
    0x1,        // reserved
    0x0,        // vin - low (reserved)
    0x0,        // vin - high
    0x0,        // vups - low (reserved)
    0x0,        // vups - high
    0x0,        // mosfet switch state
    ON_THRESH,  // on threshold
    OFF_THRESH, // off threshold
    0x0,        // countdown in 100ms units
    STATE_POWERUP,   // state
    0x0,        // cycle delay
    0x0,        // fail-shutdown delay
    0x0,        // run counter
    POWERUP_THRESH, // powerup threshold
    SHUTDOWN_THRESH, // software shutdown threshold
    R1/100,     // R1 resistor value
    R2/100,     // R2 resistor value
};
const byte reg_size = sizeof(i2c_regs);

uint8_t reg_position;
uint8_t receive_error;
uint8_t last_mosfet_state;
uint8_t send_crc, next_crc;
uint8_t sample_ptr, num_samples;
uint8_t vin_samples[MAX_SAMPLES];
uint8_t vups_samples[MAX_SAMPLES];

uint8_t readADC(uint8_t mux)
{
    ADMUX =
            (1 << ADLAR) |     // left shift result
            (1 << REFS2) |     // Sets ref. voltage to 2.56, bit 2
            (1 << REFS1) |     // Sets ref. voltage to 2.56, bit 1
            (0 << REFS0) |     // Sets ref. voltage to 2.56, bit 0
            (mux << MUX0);    

    ADCSRA =
            (1 << ADEN)  |     // Enable ADC
            (1 << ADPS2) |     // set prescaler to 64, bit 2
            (1 << ADPS1) |     // set prescaler to 64, bit 1
            (0 << ADPS0);      // set prescaler to 64, bit 0

    ADCSRA |= (1 << ADSC);         // start ADC measurement
    while (ADCSRA & (1 << ADSC) ) {
        TinyWireS_stop_check();
    } // wait till conversion complete

    return ADCH;
}

uint8_t computeAverage(uint8_t *samples)
{
    uint16_t accum = 0;
    uint8_t i;
    for (i=0; i<num_samples; i++) {
        accum = accum + samples[i];
    }
    return accum/num_samples;
}

inline void mosfetOff()
{
#ifdef DCDC_INSTEAD_OF_MOSFET
    // to turn the DCDC off, pull down the enable
    PORTB &= (~PIN_MOSFET);
    DDRB |= PIN_MOSFET;
#else
    PORTB |= PIN_MOSFET;
#endif
}

inline void mosfetOn()
{
#ifdef DCDC_INSTEAD_OF_MOSFET
    // to turn the DCDC off, float the enable
    DDRB &= (~PIN_MOSFET);
#else
    PORTB &= (~PIN_MOSFET);
#endif
}

void handleRegMosfet()
{
    if (i2c_regs[REG_MOSFET]) {
        // low will turn the mosfet on
        // mosfetOn()

#ifdef SLOW_START_MOSFET
        // Slow-start the mosfet.
        // Otherwise, the sudden current inrush will cause the dc/dc
        // converter to trip into overcurrent mode and shut off.
        uint8_t i;
        for (i=0; i<200; i++) {
            mosfetOn();
            delayMicroseconds(3);
            mosfetOff();
            delayMicroseconds(3);
        }
        for (i=0; i<200; i++) {
            mosfetOn();
            delayMicroseconds(6);
            mosfetOff();
            delayMicroseconds(3);
        }
#endif
        mosfetOn();
    } else {
        // high will turn the mosfet off
        mosfetOff();
    }
}

void requestEvent()
{
    uint8_t data;

    // Never more than one byte in the callback

    if (send_crc) {
        TinyWireS.send(next_crc);
        send_crc = 0;
        return;
    }

    if (receive_error) {
        // on a receive error, send a two-byte sequence (0xFF,ErrorCode)
        // NOTE: The CRC for a valid 0xFF is 0xF3, so never use 0xF3 as an errorcode
        TinyWireS.send(0xFF);
        next_crc = receive_error;
        send_crc = 1;
        return;
    }

    data = i2c_regs[reg_position];
    TinyWireS.send(data);
    next_crc = _crc8_ccitt_update(0, data);
    send_crc = 1;

    // Increment the reg position on each read, and loop back to zero
    reg_position++;
    if (reg_position >= reg_size)
    {
        reg_position = 0;
    }
}

void receiveEvent(uint8_t howMany)
{
    uint8_t buf[TWI_RX_BUFFER_SIZE];
    uint8_t i;
    uint8_t crc;

    if (howMany < 2)
    {
        // Sanity-check
        // CRC is required, so must be at least two
        receive_error = ERROR_SIZE;
        return;
    }
    if (howMany > TWI_RX_BUFFER_SIZE)
    {
        // Also insane number
        receive_error = ERROR_SIZE;
        return;
    }

    crc = 0;
    for (i=0; i<howMany; i++) {
        buf[i] = TinyWireS.receive();
        crc = _crc8_ccitt_update(crc, buf[i]);
    }

    // we included the crc byte in the computation
    // so at this point it should be zero.
    if (crc!=0) {
        // crc error
        receive_error = ERROR_CRC;
        return;
    }

    receive_error = ERROR_SUCCESS;
    send_crc = 0;

    reg_position = buf[0];

    for (i=1; i<howMany-1; i++) {
        i2c_regs[reg_position] = buf[i];

        switch (reg_position) {
            case REG_MOSFET:
                handleRegMosfet();
                break;
        }

        reg_position++;
        if (reg_position >= reg_size)
        {
            reg_position = 0;
        }
    }
}

void setup() {
    TinyWireS.begin(I2C_SLAVE_ADDRESS);
    TinyWireS.onRequest(requestEvent);
    TinyWireS.onReceive(receiveEvent);

    mosfetOff();
#ifndef DCDC_INSTEAD_OF_MOSFET
    // when using the mosfet, pin direction is always output
    DDRB |= PIN_MOSFET;
#endif

    sample_ptr = 0;
    num_samples = 0;

    reg_position = 0;

    receive_error = ERROR_UNINITIALIZED;

    VAL_RUN_COUNTER = 0;
}

void loop() {
    TinyWireS_stop_check();

    vin_samples[sample_ptr] = readADC(3);
    vups_samples[sample_ptr] = readADC(2);
    sample_ptr ++;
    if (sample_ptr >= MAX_SAMPLES) {
        sample_ptr = 0;
    }
    num_samples ++;
    if (num_samples > MAX_SAMPLES) {
        num_samples = MAX_SAMPLES;
    }

    VAL_VIN_HIGH = computeAverage(vin_samples);
    VAL_VUPS_HIGH = computeAverage(vups_samples);

    if (num_samples < MAX_SAMPLES) {
        return;
    }

    switch (VAL_STATE) {
        case STATE_DISABLED:
            // state machine is disabled -- do nothing
            break;

        case STATE_WAIT_OFF:
            if (VAL_VIN_HIGH <= VAL_OFF_THRESH) {
                VAL_STATE = STATE_WAIT_ON;
            }
            break;

        case STATE_WAIT_ON:
            if (VAL_VIN_HIGH >= VAL_ON_THRESH) {
                VAL_STATE = STATE_POWERUP;
            }
            break;

        case STATE_FAIL_SHUTDOWN:
        case STATE_POWERUP:
            if ((VAL_VIN_HIGH >= VAL_ON_THRESH) && (VAL_VUPS_HIGH >= VAL_POWERUP_THRESH)) {
                VAL_COUNTDOWN = 0;
                VAL_MOSFET = 1;
                handleRegMosfet();
                VAL_STATE = STATE_RUNNING;
            }
            break;

        case STATE_RUNNING:
            if ((VAL_VIN_HIGH <= VAL_OFF_THRESH) && (VAL_VUPS_HIGH <= VAL_OFF_THRESH)) {
                VAL_MOSFET = 0;
                handleRegMosfet();
                VAL_FAIL_SHUTDOWN_DELAY = 10;
                VAL_STATE = STATE_FAIL_SHUTDOWN_DELAY;
            }
            if (VAL_COUNTDOWN > 0) {
                if ((VAL_RUN_COUNTER%10)==0) {
                    VAL_COUNTDOWN --;
                }
                if (VAL_COUNTDOWN == 0) {
                    // turn off the mosfet and wait for powercycle before
                    // turning back on.
                    VAL_CYCLE_DELAY = 10;
                    VAL_STATE = STATE_CYCLE_DELAY;
                    VAL_MOSFET = 0;
                    handleRegMosfet();
                }
            }
            break;

        case STATE_FAIL_SHUTDOWN_DELAY:
            // shutdown due to power failure; delay so the pi can safely power cycle
            if (VAL_FAIL_SHUTDOWN_DELAY > 0) {
                VAL_FAIL_SHUTDOWN_DELAY--;
            }
            if (VAL_FAIL_SHUTDOWN_DELAY==0) {
                VAL_STATE = STATE_FAIL_SHUTDOWN;
            }

        case STATE_CYCLE_DELAY:
            // shutdown due to power-off request; delay so the pi can safely power cycle
            if (VAL_CYCLE_DELAY>0) {
                VAL_CYCLE_DELAY--;
            }
            if (VAL_CYCLE_DELAY==0) {
                VAL_STATE = STATE_WAIT_OFF;
            }
            break;
    }

    VAL_RUN_COUNTER++;

    tws_delay(10); // delay 10ms
}
