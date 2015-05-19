/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005/2006, Anthony Minessale II <anthmct@yahoo.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthmct@yahoo.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Anthony Minessale II <anthmct@yahoo.com>
 * Nenad Corbic <ncorbic@sangoma.com>
 *
 * switch_buffer.h -- Data Buffering Code
 * 
 */
/** 
 * @file switch_buffer.h
 * @brief Data Buffering Code
 * @see switch_buffer
 */

#ifndef SMG_SWITCH_BUFFER_H
#define SMG_SWITCH_BUFFER_H

/*!
  \brief Test for the existance of a flag on an arbitary object
  \param obj the object to test
  \param flag the or'd list of flags to test
  \return true value if the object has the flags defined
*/
#define switch_test_flag(obj, flag) ((obj)->flags & flag)

/*!
  \brief Set a flag on an arbitrary object
  \param obj the object to set the flags on
  \param flag the or'd list of flags to set
*/
#define switch_set_flag(obj, flag) (obj)->flags |= (flag)



/**
 * @defgroup switch_buffer Buffer Routines
 * @ingroup core1
 * The purpose of this module is to make a plain buffering interface that can be used for read/write buffers
 * throughout the application.  The first implementation was done to provide the functionality and the interface
 * and I think it can be optimized under the hood as we go using bucket brigades and/or ring buffering techniques.
 * @{
 */


 
#define switch_size_t int
#define switch_status_t int
#define SWITCH_DECLARE(_var_)  _var_
#define SWITCH_STATUS_SUCCESS 0
#define SWITCH_STATUS_MEMERR -1
#define switch_byte_t unsigned char
#define switch_size_t int
#define switch_buffer_free switch_buffer_destroy

typedef struct switch_buffer {
	switch_byte_t *data;
	switch_byte_t *head;
	switch_size_t used;
	switch_size_t actually_used;
	switch_size_t datalen;
	switch_size_t max_len;
	switch_size_t blocksize;
	uint32_t flags;
	uint32_t id;
	int32_t loops;
}switch_buffer_t;

//#define assert(_cond_) if ((_cond_)) { printf("%s:%s: Assertion!\n"); return -EINVAL; }

/*! \brief Allocate a new switch_buffer 
 * \param pool Pool to allocate the buffer from
 * \param buffer returned pointer to the new buffer
 * \param max_len length required by the buffer
 * \return status
 */
SWITCH_DECLARE(switch_status_t) switch_buffer_create(switch_buffer_t **buffer, switch_size_t max_len);

/*! \brief Allocate a new dynamic switch_buffer 
 * \param buffer returned pointer to the new buffer
 * \param blocksize length to realloc by as data is added
 * \param start_len ammount of memory to reserve initially
 * \param max_len length the buffer is allowed to grow to
 * \return status
 */
SWITCH_DECLARE(switch_status_t) switch_buffer_create_dynamic(switch_buffer_t **buffer, switch_size_t blocksize, switch_size_t start_len, switch_size_t max_len);

/*! \brief Get the length of a switch_buffer_t 
 * \param buffer any buffer of type switch_buffer_t
 * \return int size of the buffer.
 */
SWITCH_DECLARE(switch_size_t) switch_buffer_len(switch_buffer_t *buffer);

/*! \brief Get the freespace of a switch_buffer_t 
 * \param buffer any buffer of type switch_buffer_t
 * \return int freespace in the buffer.
 */
SWITCH_DECLARE(switch_size_t) switch_buffer_freespace(switch_buffer_t *buffer);

/*! \brief Get the in use amount of a switch_buffer_t 
 * \param buffer any buffer of type switch_buffer_t
 * \return int ammount of buffer curently in use
 */
SWITCH_DECLARE(switch_size_t) switch_buffer_inuse(switch_buffer_t *buffer);

/*! \brief Read data from a switch_buffer_t up to the ammount of datalen if it is available.  Remove read data from buffer. 
 * \param buffer any buffer of type switch_buffer_t
 * \param data pointer to the read data to be returned
 * \param datalen amount of data to be returned
 * \return int ammount of data actually read
 */
SWITCH_DECLARE(switch_size_t) switch_buffer_read(switch_buffer_t *buffer, void *data, switch_size_t datalen);

/*! \brief Read data endlessly from a switch_buffer_t 
 * \param buffer any buffer of type switch_buffer_t
 * \param data pointer to the read data to be returned
 * \param datalen amount of data to be returned
 * \return int ammount of data actually read
 * \note Once you have read all the data from the buffer it will loop around.
 */
SWITCH_DECLARE(switch_size_t) switch_buffer_read_loop(switch_buffer_t *buffer, void *data, switch_size_t datalen);

/*! \brief Assign a number of loops to read
 * \param buffer any buffer of type switch_buffer_t
 * \param loops the number of loops (-1 for infinite)
 */
SWITCH_DECLARE(void) switch_buffer_set_loops(switch_buffer_t *buffer, int32_t loops);

/*! \brief Write data into a switch_buffer_t up to the length of datalen
 * \param buffer any buffer of type switch_buffer_t
 * \param data pointer to the data to be written
 * \param datalen amount of data to be written
 * \return int amount of buffer used after the write, or 0 if no space available
 */
SWITCH_DECLARE(switch_size_t) switch_buffer_write(switch_buffer_t *buffer, const void *data, switch_size_t datalen);

/*! \brief Remove data from the buffer
 * \param buffer any buffer of type switch_buffer_t
 * \param datalen amount of data to be removed
 * \return int size of buffer, or 0 if unable to toss that much data
 */
SWITCH_DECLARE(switch_size_t) switch_buffer_toss(switch_buffer_t *buffer, switch_size_t datalen);

/*! \brief Remove all data from the buffer
 * \param buffer any buffer of type switch_buffer_t
 */
SWITCH_DECLARE(void) switch_buffer_zero(switch_buffer_t *buffer);

/*! \brief Destroy the buffer
 * \param buffer buffer to destroy
 * \note only neccessary on dynamic buffers (noop on pooled ones)
 */
SWITCH_DECLARE(void) switch_buffer_destroy(switch_buffer_t **buffer);

/** @} */


#endif
/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */

