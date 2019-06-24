/*
 * main.c
 *
 * 2 channel MODBUS switch
 *
 *  Created on: 24.05.2019
 *      Author: andru
 */

#include <string.h>

#include "ch.h"
#include "hal.h"

#include "main.h"
#include "usbcfg.h"
#include "printfs.h"

#define LINELEN			6
#define USB_WRITE_MS	100

static const char LF		= '\r';
static const char R1[]		= "r1";
static const char R1ASK[]	= "r1:?";
static const char R1ON[]	= "r1:1";
static const char R1OFF[]	= "r1:0";
static const char R2[]		= "r2";
static const char R2ASK[]	= "r2:?";
static const char R2ON[]	= "r2:1";
static const char R2OFF[]	= "r2:0";
static const char USAGE[]	= "usage:\r\n r1:?,r2:? - ask r1,r2\r\n r1:1,r1:0 - r1 on/off\r\n r2:1,r2:0 - r2 on/off\r\n";

#define LineSet(l, n)	((n) ? palSetLine(l) : palClearLine(l))
#define LineRead(l)		(palReadLine(l))

#define RELAY1			PAL_LINE(GPIOA, GPIOA_RELAY1)
#define RELAY2			PAL_LINE(GPIOA, GPIOA_RELAY2)
#define RELAY1_PWRUP	1
#define RELAY2_PWRUP	1

volatile thd_check_t thd_state = THD_INIT;		// thread state mask

/*
 * Watchdog deadline set to 2000 / (LSI=40000 / 64) = ~3.2s.
 */
static const WDGConfig wdgcfg = {
  STM32_IWDG_PR_64,
  STM32_IWDG_RL(1000),
};

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

// called on kernel panic
static void halt(void) {
  port_disable();
  while (true)	{
	chThdSleepMilliseconds(200);
  }
}

// return usb state
static bool usb_active(void) {
	return SDU1.config->usbp->state == USB_ACTIVE;
}

static void usb_write(const char *str, uint8_t value) {
	char line[8];
	if (usb_active()) {
		sprintf(line, "%s:%d\r\n", str, value);
		chnWriteTimeout(&SDU1, (const uint8_t *) line, strlen(line), TIME_MS2I(USB_WRITE_MS));
	}
}

static THD_WORKING_AREA(wausbReadThd, 512);
static THD_FUNCTION(usbReadThd, arg) {
	(void) arg;
	char rbuf[LINELEN];
	char *p = rbuf;

	chRegSetThreadName("USBRead");

	sduObjectInit(&SDU1);
	sduStart(&SDU1, &serusbcfg);

	/*
	 * Activates the USB driver and then the USB bus pull-up on D+.
	 * Note, a delay is inserted in order to not have to disconnect the cable
	 * after a reset.
	 */
	usbDisconnectBus(serusbcfg.usbp);
	chThdSleepMilliseconds(1500);
	usbStart(serusbcfg.usbp, &usbcfg);
	usbConnectBus(serusbcfg.usbp);

	usb_write(R1, LineRead(RELAY1));
	usb_write(R2, LineRead(RELAY2));

	while (TRUE) {
		char c = chnGetTimeout(&SDU1, TIME_INFINITE);
    	// wait for LF
	    if (c == LF) {
    		*p = 0;

    		if (strlen(rbuf) == 0) continue;

    		if (strstr(rbuf, R1ASK)) {
   				usb_write(R1, LineRead(RELAY1));
       	    } else if(strstr(rbuf, R2ASK)) {
   				usb_write(R2, LineRead(RELAY2));
       	    } else if (strstr(rbuf, R1ON)) {
       	    	LineSet(RELAY1, 1);
   				usb_write(R1, LineRead(RELAY1));
       	    } else if (strstr(rbuf, R1OFF)) {
       	    	LineSet(RELAY1, 0);
   				usb_write(R1, LineRead(RELAY1));
       	    } else if(strstr(rbuf, R2ON)) {
       	    	LineSet(RELAY2, 1);
   				usb_write(R2, LineRead(RELAY2));
       	    } else if(strstr(rbuf, R2OFF)) {
       	    	LineSet(RELAY2, 0);
   				usb_write(R2, LineRead(RELAY2));
       	    } else {
       	    	if (usb_active()) {
           	    	char line[85];
       	    		sprintf(line, "%s\r\n", USAGE);
       	    		chnWriteTimeout(&SDU1, (const uint8_t *) line, strlen(line), TIME_MS2I(USB_WRITE_MS));
       	    	}
       	    }

    		p = rbuf;
       	    continue;
	    }
	    if (c < 0x20) continue;
	    if (p < (rbuf + LINELEN - 1)) {
	    	*p++ = (char) c;
	    } else {
	    	p = rbuf;
	    }
	}
}

/*
 * Application entry point.
 */
int main(void) {
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  palSetLineMode(RELAY1, PAL_MODE_OUTPUT_PUSHPULL);
  LineSet(RELAY1, RELAY1_PWRUP);

  palSetLineMode(RELAY2, PAL_MODE_OUTPUT_PUSHPULL);
  LineSet(RELAY2, RELAY2_PWRUP);

  wdgStart(&WDGD1, &wdgcfg);

  chThdCreateStatic(wausbReadThd, sizeof(wausbReadThd), NORMALPRIO + 1, usbReadThd, NULL);

  systime_t time = chVTGetSystemTime();
  while (TRUE) {
	time += TIME_MS2I(1000);

    thd_state |= THD_MAIN;
    if (thd_state == THD_GOOD) {
    	wdgReset(&WDGD1);
    }

    chThdSleepUntil(time);
  }
}
