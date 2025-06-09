#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Initialize the OpenThread stack and UDP socket.
 */
void thread_init(void);

/**
 * @brief Register a callback for incoming Thread messages.
 *
 * @param cb  Function to be called on receive: (data, length, src_rloc16)
 */
typedef void (*thread_rx_cb_t)(const uint8_t *data, size_t length, uint16_t src_id);
void thread_register_receive_cb(thread_rx_cb_t cb);

/**
 * @brief Send a raw buffer over Thread.
 *
 * @param data     Pointer to the payload bytes
 * @param length   Number of bytes to send
 * @param dest_id  RLOC16 of destination node
 */
void thread_send(const uint8_t *data, size_t length, uint16_t dest_id);

/**
 * @brief Process any pending Thread tasklets and radio events.
 *        Must be called periodically from main loop.
 */
void thread_process(void);

#endif // THREAD_UTILS_H