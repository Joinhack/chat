#ifndef NETWORK_H
#define NETWORK_H

int tcp_accept_event_proc(cevents *cevts, int fd, void *priv, int mask);

int cio_install_read_event(cevents *cevts, cio *io);

int read_event_proc(cevents *cevts, int fd, void *priv, int mask);

void *process_event(void *priv);

#endif /*end define network*/
