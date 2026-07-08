/*
 * netlog.h — UDP broadcast log for live runtime debugging.
 *
 * The PS5 payload sends each log line as a UDP datagram to the broadcast
 * address on port NETLOG_PORT.  On the dev machine (Termux / PC), run:
 *
 *   nc -ul 9020
 *
 * to receive all output in real-time — even when the elfldr TCP connection
 * has already closed or when USB logging is active.
 */

#ifndef QUAZA_NETLOG_H
#define QUAZA_NETLOG_H

#define NETLOG_PORT 9020

/* Call once before any netlog_send() calls. Non-fatal if socket fails. */
void netlog_init(void);

/* Send a formatted log line. Silently drops if init failed. */
void netlog_printf(const char *fmt, ...);

#endif /* QUAZA_NETLOG_H */
