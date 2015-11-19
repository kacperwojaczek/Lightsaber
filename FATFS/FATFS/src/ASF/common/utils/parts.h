/**
 * \file
 *
 * \brief Atmel part identification macros
 *
 * Copyright (C) 2012-2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef ATMEL_PARTS_H
#define ATMEL_PARTS_H

/**
 * \defgroup part_macros_group Atmel part identification macros
 *
 * This collection of macros identify which series and families that the various
 * Atmel parts belong to. These can be used to select part-dependent sections of
 * code at compile time.
 *
 * @{
 */

/**
 * \name Convenience macros for part checking
 * @{
 */
/* ! Check GCC and IAR part definition for 8-bit AVR */
#define AVR8_PART_IS_DEFINED(part) \
	(defined(__ ## part ## __) || defined(__AVR_ ## part ## __))

/* ! Check GCC and IAR part definition for 32-bit AVR */
#define AVR32_PART_IS_DEFINED(part) \
	(defined(__AT32 ## part ## __) || defined(__AVR32_ ## part ## __))

/* ! Check GCC and IAR part definition for SAM */
#define SAM_PART_IS_DEFINED(part) (defined(__ ## part ## __))
/** @} */

/**
 * \defgroup uc3_part_macros_group AVR UC3 parts
 * @{
 */

/**
 * \defgroup xmega_part_macros_group AVR XMEGA parts
 * @{
 */

/**
 * \name AVR XMEGA AU series
 * @{
 */
#define XMEGA_A1U ( \
		AVR8_PART_IS_DEFINED(ATxmega64A1U)  || \
		AVR8_PART_IS_DEFINED(ATxmega128A1U) \
		)

#define XMEGA_A3U ( \
		AVR8_PART_IS_DEFINED(ATxmega64A3U)  || \
		AVR8_PART_IS_DEFINED(ATxmega128A3U) || \
		AVR8_PART_IS_DEFINED(ATxmega192A3U) || \
		AVR8_PART_IS_DEFINED(ATxmega256A3U) \
		)

#define XMEGA_A3BU ( \
		AVR8_PART_IS_DEFINED(ATxmega256A3BU) \
		)

#define XMEGA_A4U ( \
		AVR8_PART_IS_DEFINED(ATxmega16A4U)  || \
		AVR8_PART_IS_DEFINED(ATxmega32A4U)  || \
		AVR8_PART_IS_DEFINED(ATxmega64A4U)  || \
		AVR8_PART_IS_DEFINED(ATxmega128A4U) \
		)
/** @} */

/**
 * \name AVR XMEGA families
 * @{
 */
/** AVR XMEGA A family */
#define XMEGA_A (XMEGA_A1 || XMEGA_A3 || XMEGA_A3B || XMEGA_A4)

/** AVR XMEGA AU family */
#define XMEGA_AU (XMEGA_A1U || XMEGA_A3U || XMEGA_A3BU || XMEGA_A4U)

/** AVR XMEGA B family */
#define XMEGA_B (XMEGA_B1 || XMEGA_B3)

/** AVR XMEGA C family */
#define XMEGA_C (XMEGA_C3 || XMEGA_C4)

/** AVR XMEGA D family */
#define XMEGA_D (XMEGA_D3 || XMEGA_D4)

/** AVR XMEGA E family */
#define XMEGA_E (XMEGA_E5)
/** @} */


/** AVR XMEGA product line */
#define XMEGA (XMEGA_A || XMEGA_AU || XMEGA_B || XMEGA_C || XMEGA_D || XMEGA_E)

#endif /* ATMEL_PARTS_H */
