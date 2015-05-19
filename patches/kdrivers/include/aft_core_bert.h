/******************************************************************************//**
 * \file aft_core_bert.h
 * \brief	Definitions and implementation of 
 *			Software BERT for Sangoma AFT cards.
 *
 * Authors: David Rokhvarg <davidr@sangoma.com>
 *
 * Copyright (c) 2007 - 2010, Sangoma Technologies
 *								All rights reserved.
 *
 *	* Redistribution and use in source and binary forms, with or without
 *	  modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the Sangoma Technologies nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Sangoma Technologies ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Sangoma Technologies BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * ===============================================================================
 */


#ifndef _AFT_CORE_BERT_H
#define _AFT_CORE_BERT_H


#ifdef WAN_KERNEL

# include "aft_core_options.h"
# include "if_wanpipe_common.h"	/* wanpipe_common_t */


typedef struct _wp_bert {

	wp_bert_sequence_type_t m_eSequenceType;
	size_t m_uiNumberOfIdenticalValues;
	size_t m_uiErrors;
	size_t m_uiSynchronizedCount;
	u8	m_bSynchronized;
	size_t m_uiNumberOfIdenticalValuesRequireToSynchronize;

	u8 *m_pNextExpectedValue;
	u8 *m_pNextValue;

	u8 *m_pSequenceBegin;
	u8 *m_pSequenceEnd;

	u8 *m_pContext; /* an optional context */
}wp_bert_t;

#define WP_BERT_SEQUENCE_LENGTH 257

/* Sequences supported by BERT */
static u8 const wp_bert_random_sequence[WP_BERT_SEQUENCE_LENGTH]
  = { 15, 164, 118, 194, 69, 52, 47, 152, 122, 117, 44, 99, 150,
      185, 197, 226, 235, 146, 250, 135, 18, 76, 207, 115, 81, 
      130, 232, 98, 153, 151, 145, 53, 253, 154, 224, 27, 14, 26,
      212, 131, 85, 95, 160, 241, 68, 203, 114, 62, 138, 83, 71,
      105, 3, 240, 156, 208, 175, 205, 107, 30, 251, 198, 40, 159,
      63, 173, 149, 169, 227, 75, 57, 46, 254, 113, 191, 24, 129,
      245, 142, 174, 100, 190, 37, 147, 28, 246, 77, 8, 230, 165,
      223, 93, 244, 111, 215, 4, 195, 242, 163, 58, 96, 219, 157,
      202, 210, 143, 13, 88, 181, 158, 243, 16, 120, 11, 214, 31,
      166, 255, 228, 91, 204, 218, 36, 19, 42, 148, 67, 29, 201, 
      66, 23, 128, 87, 167, 136, 7, 193, 222, 200, 92, 144, 176,
      82, 108, 65, 172, 141, 252, 10, 234, 233, 171, 101, 54, 132,
      103, 236, 73, 61, 192, 79, 211, 221, 74, 50, 64, 180, 220,
      196, 187, 12, 102, 70, 104, 206, 0, 116, 133, 225, 170, 125,
      127, 124, 22, 209, 137, 126, 106, 186, 112, 162, 183, 237,
      238, 199, 33, 121, 155, 168, 139, 34, 216, 48, 231, 38, 51,
      5, 239, 1, 229, 84, 178, 35, 94, 43, 189, 161, 179, 90, 97,
      32, 134, 217, 6, 49, 2, 17, 56, 248, 123, 213, 89, 188, 86,
      72, 184, 39, 21, 25, 247, 55, 182, 119, 249, 140, 109, 41,
      177, 80, 45, 59, 60, 78, 20, 110, 111 };

static u8 const wp_bert_ascendant_sequence[WP_BERT_SEQUENCE_LENGTH]
  = { 'a', 's', 'c', 'e', 'n', 'd', 'a', 'n', 't', 1, 2, 3, 4,
      5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 
      17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
      30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
      43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
      56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68,
      69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
      82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
      95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
      108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 
      121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
      134, 135, 136, 137, 138, 139, 140, 141, 142, 142, 144, 145, 146,
      147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
      160, 161, 162, 163, 165, 166, 167, 168, 169, 170, 171, 172, 173,
      174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185,
      186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198,
      199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
      213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226,
      227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
      240, 241, 242, 243, 244, 245, 246, 247, 248 };
 
static u8 const wp_bert_descendant_sequence[WP_BERT_SEQUENCE_LENGTH]
  = { 'd', 'e', 's', 'c', 'd', 'a', 'n', 't', 249,
      248, 247, 246, 245, 244, 243, 242, 241, 240, 239,
      238, 237, 236, 235, 234, 233, 232, 231, 230, 229,
      228, 227, 226, 225, 224, 223, 222, 221, 220, 219,
      218, 217, 216, 215, 214, 213, 212, 211, 210, 209,
      208, 207, 206, 205, 204, 203, 202, 201, 200, 199,
      198, 197, 196, 195, 194, 193, 192, 191, 190, 189,
      188, 187, 186, 185, 184, 183, 182, 181, 180, 179,
      178, 177, 176, 175, 174, 173, 172, 171, 170, 169,
      168, 167, 166, 165, 164, 163, 162, 161, 160, 159,
      158, 157, 156, 155, 154, 153, 152, 151, 150, 149,
      148, 147, 146, 145, 144, 143, 142, 141, 140, 139,
      138, 137, 136, 135, 134, 133, 132, 131, 130, 129,
      128, 127, 126, 125, 124, 123, 122, 121, 120, 119,
      118, 117, 116, 115, 114, 113, 112, 111, 110, 109,
      108, 107, 106, 105, 104, 103, 102, 101, 100, 99,
      98, 97, 96, 95, 94, 93, 92, 91, 90, 89,
      88, 87, 86, 85, 84, 83, 82, 81, 80, 79,
      78, 77, 76, 75, 74, 73, 72, 71, 70, 69,
      68, 67, 66, 65, 64, 63, 62, 61, 60, 59,
      58, 57, 56, 55, 54, 53, 52, 51, 50, 49,
      48, 47, 46, 45, 44, 43, 42, 41, 40, 39,
      38, 37, 36, 35, 34, 33, 32, 31, 30, 29,
      28, 27, 26, 25, 24, 23, 22, 21, 20, 19,
      18, 17, 16, 15, 14, 13, 12, 11, 10, 9,
      8, 7, 6, 5, 4, 3, 2, 1 };


/******** BERT functions *********/
/** @brief reset */
int wp_bert_reset(wp_bert_t *bert);

/** @brief change the type sequence used by this bert */
int wp_bert_set_sequence_type(wp_bert_t *bert, wp_bert_sequence_type_t sequence_type);

/** @bried Get the number of times when the BERT entered in the
	synchronized state */
u32 wp_bert_get_synchonized_count(wp_bert_t *bert);

/** @brief Get the number errors */
u32 wp_bert_get_errors(wp_bert_t *bert);

/** @brief Returns 1 when the BERT is synchronized */
u8 wp_bert_is_synchronized(wp_bert_t *bert);

/** @brief Return the next value to stream toward the remote BERT
	entity */
u8 wp_bert_pop_value(wp_bert_t *bert, u8 *value);

/** @brief Push value in the BERT.  This method return 1 if the
	BERT is synchronized and 0 otherwise.  The expected value is
	also set when the BERT is not synchronized.  */
u8 wp_bert_push_value(wp_bert_t *bert, u8 current_value, u8 *expected_value);

/**  @brief print current state of BERT */
void wp_bert_print_state(wp_bert_t *bert);

/****** end of BERT functions *******/


#endif /* WAN_KERNEL */


#endif /* _AFT_CORE_BERT_H */

