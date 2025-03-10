/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http.h"

#include <zephyr/kernel.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/http/status.h>
#include <zephyr/net/socket.h>

#if !defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP)
#include <zephyr/net/tls_credentials.h>
#endif

#include "log.h"

ASTARTE_LOG_MODULE_REGISTER(astarte_http, CONFIG_ASTARTE_DEVICE_SDK_HTTP_LOG_LEVEL);

/************************************************
 *       Checks over configuration values       *
 ***********************************************/

#if defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP)
#warning "TLS has been disabled (unsafe)!"
#endif /* defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP) */

BUILD_ASSERT(sizeof(CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME) != 1, "Missing hostname in configuration");

/************************************************
 *       Callbacks declaration/definition       *
 ***********************************************/

static void http_response_cb(
    struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
    bool *request_ok = (bool *) user_data;
    if (final_data == HTTP_DATA_MORE) {
        ASTARTE_LOG_ERR("Partial data received (%zd bytes)", rsp->data_len);
        ASTARTE_LOG_ERR("HTTP reply is too long for rx buffer.");
        *request_ok = false;
    } else if (final_data == HTTP_DATA_FINAL) {
        ASTARTE_LOG_DBG("All the data received (%zd bytes)", rsp->data_len);
        if ((rsp->http_status_code != HTTP_200_OK) && (rsp->http_status_code != HTTP_201_CREATED)) {
            ASTARTE_LOG_ERR("HTTP request failed, response code: %s %d", rsp->http_status,
                rsp->http_status_code);
            *request_ok = false;
        }
    }
}

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Create a new TCP socket and connect it to a server.
 *
 * @note The returned socket should be closed once its use has terminated.
 *
 * @return -1 upon failure, a file descriptor for the new socket otherwise.
 */
static int create_and_connect_socket(void);

#if defined(CONFIG_ASTARTE_DEVICE_SDK_HTTP_LOG_LEVEL_DBG)
/**
 * @brief Print content of addrinfo struct.
 *
 * @param[in] input_addinfo addrinfo struct to print.
 */
static void dump_addrinfo(const struct zsock_addrinfo *input_addinfo);
#endif

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_result_t astarte_http_post(int32_t timeout_ms, const char *url, const char **header_fields,
    const char *payload, uint8_t *resp_buf, size_t resp_buf_size)
{
    // Create and connect the socket to use
    int sock = create_and_connect_socket();
    if (sock < 0) {
        return ASTARTE_RESULT_SOCKET_ERROR;
    }

    struct http_request req = { 0 };
    uint8_t recv_buf[CONFIG_ASTARTE_DEVICE_SDK_ADVANCED_HTTP_RCV_BUFFER_SIZE];
    memset(&recv_buf, 0, sizeof(recv_buf));

    req.method = HTTP_POST;
    req.host = CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME;
#if defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP)
    req.port = "80";
#else
    req.port = "443";
#endif
    req.url = url;
    req.content_type_value = "application/json";
    req.header_fields = header_fields;
    req.protocol = "HTTP/1.1";
    req.response = http_response_cb;
    req.payload = payload;
    req.payload_len = strlen(payload);
    req.recv_buf = recv_buf;
    req.recv_buf_len = sizeof(recv_buf);

    bool post_ok = true;

    int http_rc = http_client_req(sock, &req, timeout_ms, &post_ok);
    if ((http_rc < 0) || !post_ok) {
        ASTARTE_LOG_ERR("HTTP post request failed: %d", http_rc);
        ASTARTE_LOG_ERR("Receive buffer content:\n%s", recv_buf);
        zsock_close(sock);
        return ASTARTE_RESULT_HTTP_REQUEST_ERROR;
    }

    // Close the used socket
    zsock_close(sock);

    // Find the two consecutive CRLF (string "\r\n\r\n") indicating the end of the headers section
    uint8_t *http_recv_body = NULL;
    const char two_crlf[] = "\r\n\r\n";
    for (size_t i = 0; i < CONFIG_ASTARTE_DEVICE_SDK_ADVANCED_HTTP_RCV_BUFFER_SIZE - 4; i++) {
        if (memcmp(recv_buf + i, two_crlf, 4) == 0) {
            http_recv_body = recv_buf + i + 4;
            break;
        }
    }

    // Check that sufficient space is present in the response buffer
    if (resp_buf_size <= strlen(http_recv_body)) {
        ASTARTE_LOG_ERR("Insufficient output buffer for HTTP post function.");
        ASTARTE_LOG_ERR("Requires %d bytes.", strlen(http_recv_body) + 1);
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    // Copy the received data to the response buffer
    memcpy(resp_buf, http_recv_body, strlen(http_recv_body) + 1);

    return ASTARTE_RESULT_OK;
}

astarte_result_t astarte_http_get(int32_t timeout_ms, const char *url, const char **header_fields,
    uint8_t *resp_buf, size_t resp_buf_size)
{
    // Create and connect the socket to use
    int sock = create_and_connect_socket();
    if (sock < 0) {
        return ASTARTE_RESULT_SOCKET_ERROR;
    }

    struct http_request req = { 0 };
    uint8_t recv_buf[CONFIG_ASTARTE_DEVICE_SDK_ADVANCED_HTTP_RCV_BUFFER_SIZE];
    memset(&recv_buf, 0, sizeof(recv_buf));

    req.method = HTTP_GET;
    req.host = CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME;
#if defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP)
    req.port = "80";
#else
    req.port = "443";
#endif
    req.url = url;
    req.content_type_value = "application/json";
    req.header_fields = header_fields;
    req.protocol = "HTTP/1.1";
    req.response = http_response_cb;
    req.recv_buf = recv_buf;
    req.recv_buf_len = sizeof(recv_buf);

    bool get_ok = true;

    int http_rc = http_client_req(sock, &req, timeout_ms, &get_ok);
    if ((http_rc < 0) || !get_ok) {
        ASTARTE_LOG_ERR("HTTP request failed: %d", http_rc);
        ASTARTE_LOG_ERR("Receive buffer content:\n%s", recv_buf);
        zsock_close(sock);
        return ASTARTE_RESULT_HTTP_REQUEST_ERROR;
    }

    // Close the socket
    zsock_close(sock);

    // Find the two consecutive CRLF (string "\r\n\r\n") indicating the end of the headers section
    uint8_t *http_recv_body = NULL;
    const char two_crlf[] = "\r\n\r\n";
    for (size_t i = 0; i < CONFIG_ASTARTE_DEVICE_SDK_ADVANCED_HTTP_RCV_BUFFER_SIZE - 4; i++) {
        if (memcmp(recv_buf + i, two_crlf, 4) == 0) {
            http_recv_body = recv_buf + i + 4;
            break;
        }
    }

    // Check that sufficient space is present in the response buffer
    if (resp_buf_size <= strlen(http_recv_body)) {
        ASTARTE_LOG_ERR("Insufficient output buffer for HTTP post function.");
        ASTARTE_LOG_ERR("Requires %d bytes.", strlen(http_recv_body) + 1);
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    // Copy the received data to the response buffer
    memcpy(resp_buf, http_recv_body, strlen(http_recv_body) + 1);

    return ASTARTE_RESULT_OK;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static int create_and_connect_socket(void)
{
    char hostname[] = CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME;
#if defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP)
    char port[] = "80";
#else
    char port[] = "443";
#endif
    struct zsock_addrinfo hints = { 0 };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct zsock_addrinfo *broker_addrinfo = NULL;
    int getaddrinfo_rc = zsock_getaddrinfo(hostname, port, &hints, &broker_addrinfo);
    if (getaddrinfo_rc != 0) {
        ASTARTE_LOG_ERR("Unable to resolve address (%d) %s", getaddrinfo_rc,
            zsock_gai_strerror(getaddrinfo_rc));
        if (getaddrinfo_rc == DNS_EAI_SYSTEM) {
            ASTARTE_LOG_ERR("Errno: %s", strerror(errno));
        }
        return -1;
    }

#if defined(CONFIG_ASTARTE_DEVICE_SDK_HTTP_LOG_LEVEL_DBG)
    dump_addrinfo(broker_addrinfo);
#endif

#if defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP)
    int proto = IPPROTO_TCP;
#else
    int proto = IPPROTO_TLS_1_2;
#endif
    int sock = zsock_socket(broker_addrinfo->ai_family, broker_addrinfo->ai_socktype, proto);
    if (sock == -1) {
        ASTARTE_LOG_ERR("Socket creation error: %d", sock);
        zsock_freeaddrinfo(broker_addrinfo);
        return -1;
    }

#if !defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP)
    sec_tag_t sec_tag_opt[] = {
        CONFIG_ASTARTE_DEVICE_SDK_HTTPS_CA_CERT_TAG,
    };
    int sockopt_rc
        = zsock_setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_opt, sizeof(sec_tag_opt));
    if (sockopt_rc == -1) {
        ASTARTE_LOG_ERR("Socket options error: %d", sockopt_rc);
        zsock_close(sock);
        zsock_freeaddrinfo(broker_addrinfo);
        return -1;
    }

    sockopt_rc = zsock_setsockopt(sock, SOL_TLS, TLS_HOSTNAME, hostname, sizeof(hostname));
    if (sockopt_rc == -1) {
        ASTARTE_LOG_ERR("Socket options error: %d", sockopt_rc);
        zsock_close(sock);
        zsock_freeaddrinfo(broker_addrinfo);
        return -1;
    }
#endif

    int connect_rc = zsock_connect(sock, broker_addrinfo->ai_addr, broker_addrinfo->ai_addrlen);
    if (connect_rc == -1) {
        ASTARTE_LOG_ERR("Connection error: %d", connect_rc);
        ASTARTE_LOG_ERR("Errno: (%d) %s", errno, strerror(errno));
        zsock_close(sock);
        zsock_freeaddrinfo(broker_addrinfo);
        return -1;
    }

    zsock_freeaddrinfo(broker_addrinfo);

    return sock;
}

#if defined(CONFIG_ASTARTE_DEVICE_SDK_HTTP_LOG_LEVEL_DBG)
#define ADDRINFO_IP_ADDR_SIZE 16U
static void dump_addrinfo(const struct zsock_addrinfo *input_addinfo)
{
    char ip_addr[ADDRINFO_IP_ADDR_SIZE] = { 0 };
    zsock_inet_ntop(AF_INET, &((struct sockaddr_in *) input_addinfo->ai_addr)->sin_addr, ip_addr,
        sizeof(ip_addr));
    ASTARTE_LOG_DBG("addrinfo @%p: ai_family=%d, ai_socktype=%d, ai_protocol=%d, "
                    "sa_family=%d, sin_port=%x, ip_addr=%s ai_addrlen=%zu",
        input_addinfo, input_addinfo->ai_family, input_addinfo->ai_socktype,
        input_addinfo->ai_protocol, input_addinfo->ai_addr->sa_family,
        ((struct sockaddr_in *) input_addinfo->ai_addr)->sin_port, ip_addr,
        input_addinfo->ai_addrlen);
}
#endif
