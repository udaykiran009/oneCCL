#ifndef _REQUEST_H_
#define _REQUEST_H_

#define REQUEST_SET(request, val) (*request = (MPI_Request)val)

typedef MPI_Request request_t;

#endif /* _REQUEST_H_ */
