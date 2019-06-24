/*
 * main.h
 *
 *  Created on: 10.06.2019
 *      Author: andru
 */

#ifndef MAIN_H_
#define MAIN_H_

#define FIRMWARE	100		// версия прошивки
#define DEBUG		0

#define THD_GOOD	0b01
#define THD_INIT	0

typedef enum {
	THD_MAIN	= (1 << 0),
	THD_SWITCH	= (1 << 1),
} thd_check_t;

extern volatile thd_check_t thd_state;

#endif /* MAIN_H_ */
