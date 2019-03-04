/*!
 *  @file Adafruit_TLC59711.cpp
 *
 *  @mainpage Adafruit TLC59711 - 24-channel PWM/LED driver
 *
 *  @section intro_sec Introduction
 *
 *  This is a library for our Adafruit 24-channel PWM/LED driver
 *
 *  Pick one up today in the adafruit shop!
 *  ------> http://www.adafruit.com/products/1455
 *
 *  These drivers uses SPI to communicate, 3 pins are required to
 *  interface: Data, Clock and Latch. The boards are chainable
 *
 *  Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing
 *  products from Adafruit!
 *
 *  @section author Author
 *
 *  Limor Fried/Ladyada
 *
 *  @section license License
 *
 *  BSD license, all text above must be included in any redistribution
 */

#include <Adafruit_TLC59711.h>
#include <SPI.h>

/*!
 *  @brief  Instantiates a new Adafruit_TLC59711 class
 *  @param  n
 *          Number of drivers
 *  @param  c
 *	    CLK pin
 *  @param  d
 *	    Data pin
 */
Adafruit_TLC59711::Adafruit_TLC59711(uint8_t n, uint8_t c, uint8_t d) {
  numdrivers = n;
  _clk = c;
  _dat = d;

  BCr = BCg = BCb = 0x7F;

  pwmbuffer = (uint16_t *)calloc(2, 12 * n);
}

/*!
 *  @brief  Instantiates a new Adafruit_TLC59711 class
 *  @param  n
 *          Number of drivers
 */
Adafruit_TLC59711::Adafruit_TLC59711(uint8_t n) {
  numdrivers = n;
  _clk = -1;
  _dat = -1;

  SPI.setBitOrder(MSBFIRST);
#ifdef __arm__
  SPI.setClockDivider(42);
#else
  SPI.setClockDivider(SPI_CLOCK_DIV8);
#endif
  SPI.setDataMode(SPI_MODE0);
  BCr = BCg = BCb = 0x7F;

  pwmbuffer = (uint16_t *)calloc(2, 12 * n);
}

/*!
 *  @brief  Writes data to SPI
 *  @param  d
 *          Data to write
 */
void Adafruit_TLC59711::spiwriteMSB(uint32_t d) {

  if (_clk >= 0) {
    uint32_t b = 0x80;
    //  b <<= (bits-1);
    for (; b != 0; b >>= 1) {
      digitalWrite(_clk, LOW);
      if (d & b)
        digitalWrite(_dat, HIGH);
      else
        digitalWrite(_dat, LOW);
      digitalWrite(_clk, HIGH);
    }
  } else {
    SPI.transfer(d);
  }
}

/*!
 *  @brief  Commites the data sent to TLC59711
 */
void Adafruit_TLC59711::write() {
  uint32_t command;

  // Magic word for write
  command = 0x25;

  command <<= 5;
  // OUTTMG = 1, EXTGCK = 0, TMGRST = 1, DSPRPT = 1, BLANK = 0 -> 0x16
  command |= 0x16;

  command <<= 7;
  command |= BCr;

  command <<= 7;
  command |= BCg;

  command <<= 7;
  command |= BCb;

  noInterrupts();
  for (uint8_t n = 0; n < numdrivers; n++) {
    spiwriteMSB(command >> 24);
    spiwriteMSB(command >> 16);
    spiwriteMSB(command >> 8);
    spiwriteMSB(command);

    // 12 channels per TLC59711
    for (int8_t c = 11; c >= 0; c--) {
      // 16 bits per channel, send MSB first
      spiwriteMSB(pwmbuffer[n * 12 + c] >> 8);
      spiwriteMSB(pwmbuffer[n * 12 + c]);
    }
  }

  if (_clk >= 0)
    delayMicroseconds(200);
  else
    delayMicroseconds(2);
  interrupts();
}

/*!
 *  @brief  Sets PWM
 *  @param  chan
 *          The channel to set
 *  @param  pwm
 *	    PWM value to set
 */
void Adafruit_TLC59711::setPWM(uint8_t chan, uint16_t pwm) {
  if (chan > 12 * numdrivers)
    return;
  pwmbuffer[chan] = pwm;
}

/*!
 *  @brief  Sets RGB color of specified LED
 *  @param  lednum
 *          Index of the LED
 *  @param  r
 *  @param  g
 *  @param  b
 */
void Adafruit_TLC59711::setLED(uint8_t lednum, uint16_t r, uint16_t g,
                               uint16_t b) {
  setPWM(lednum * 3, r);
  setPWM(lednum * 3 + 1, g);
  setPWM(lednum * 3 + 2, b);
}


/*!
 *  @brief  Sets up the HW
 *  @return True if process is successful
 */
boolean Adafruit_TLC59711::begin() {
  if (!pwmbuffer)
    return false;

  if (_clk >= 0) {
    pinMode(_clk, OUTPUT);
    pinMode(_dat, OUTPUT);
  } else {
    SPI.begin();
  }
  return true;
}
