#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "mlsl_types.h"

mlsl_status_t mlsl_transport_init();
mlsl_status_t mlsl_transport_finalize();

mlsl_status_t mlsl_transport_send();
mlsl_status_t mlsl_transport_recv();
mlsl_status_t mlsl_transport_wait();

#endif /* TRANSPORT_H */
