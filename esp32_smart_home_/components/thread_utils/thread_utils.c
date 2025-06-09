#include "thread_utils.h"
#include "esp_log.h"
#include "esp_openthread.h"
#include <openthread/instance.h>
#include <openthread/udp.h>
#include <openthread/tasklet.h>
#include <openthread/platform/platform.h>

static const char *TAG = "thread_utils";
static otInstance    *s_ot_instance;
static otUdpSocket    s_udp_socket;
static thread_rx_cb_t s_rx_cb = NULL;

// Thread port for control/sensor messages
#define THREAD_UDP_PORT 9000

/**
 * @brief Internal UDP receive callback.
 */
static void udp_receive_cb(void *          aContext,
                           otMessage *     aMessage,
                           const otMessageInfo *aInfo)
{
    // Calculate payload length
    uint16_t offset = otMessageGetOffset(aMessage);
    uint16_t length = otMessageGetLength(aMessage) - offset;
    if (length < 1) {
        ESP_LOGW(TAG, "Zero-length Thread packet");
        otMessageFree(aMessage);
        return;
    }

    // Read payload into buffer (CRC + JSON)
    uint8_t buf[256];
    if (length > sizeof(buf)) length = sizeof(buf);
    otMessageRead(aMessage, offset, buf, length);

    uint16_t src_rloc = aInfo->mPeerAddr.mFields.m16[7] & OT_RLOC16_MASK;
    ESP_LOGI(TAG, "Thread RX %u bytes from RLOC0x%04x", length, src_rloc);

    // Invoke user callback
    if (s_rx_cb) {
        s_rx_cb(buf, length, src_rloc);
    }

    otMessageFree(aMessage);
}

void thread_init(void)
{
    ESP_LOGI(TAG, "Initializing OpenThread stack and UDP socket");

    // Initialize OpenThread platform
    esp_openthread_platform_init_conf_t conf = ESP_OPENTHREAD_INIT_CONFIG_DEFAULT();
    esp_openthread_init(&conf);

    // Get otInstance handle
    s_ot_instance = esp_openthread_get_instance();
    if (!s_ot_instance) {
        ESP_LOGE(TAG, "Failed to get otInstance");
        return;
    }

    // Open and bind a UDP socket on THREAD_UDP_PORT
    otError err;
    otSockAddr addr = { .mPort = THREAD_UDP_PORT };

    otUdpOpen(s_ot_instance, &s_udp_socket, udp_receive_cb, NULL);
    err = otUdpBind(s_ot_instance, &s_udp_socket, &addr);
    if (err != OT_ERROR_NONE) {
        ESP_LOGE(TAG, "Failed to bind UDP socket (%d)", err);
    } else {
        ESP_LOGI(TAG, "UDP socket bound to port %u", THREAD_UDP_PORT);
    }

    ESP_LOGI(TAG, "Thread stack initialized");
}

void thread_register_receive_cb(thread_rx_cb_t cb)
{
    s_rx_cb = cb;
    ESP_LOGI(TAG, "Registered Thread receive callback");
}

void thread_send(const uint8_t *data, size_t length, uint16_t dest_id)
{
    if (!s_ot_instance) {
        ESP_LOGW(TAG, "Thread not initialized");
        return;
    }

    // Create IPv6 UDP message
    otMessage *msg = otUdpNewMessage(s_ot_instance, NULL);
    if (!msg) {
        ESP_LOGW(TAG, "Failed to allocate message");
        return;
    }

    // Append payload
    otMessageAppend(msg, data, length);

    // Set destination address (RLOC16 -> IPv6 address)
    otMessageInfo info = {};
    info.mPeerAddr = otThreadGetMeshLocalEid(s_ot_instance);
    info.mPeerAddr.mFields.m16[7] = (info.mPeerAddr.mFields.m16[7] & ~OT_RLOC16_MASK)
                                    | (dest_id & OT_RLOC16_MASK);
    info.mPeerPort      = THREAD_UDP_PORT;
    info.mSocketAddress = info.mPeerAddr;
    info.mSocketPort    = THREAD_UDP_PORT;

    // Send
    otError err = otUdpSend(s_ot_instance, &s_udp_socket, msg, &info);
    if (err != OT_ERROR_NONE) {
        ESP_LOGW(TAG, "Thread send error: %d", err);
        otMessageFree(msg);
    } else {
        ESP_LOGI(TAG, "Thread TX %u bytes to RLOC0x%04x", length, dest_id);
    }
}

void thread_process(void)
{
    // Process radio events and tasklets
    esp_openthread_radio_process(s_ot_instance);
    esp_openthread_tasklets_process(s_ot_instance);
}
