oneCCL Concepts
===============

oneAPI Collective Communication Library introduces the following list of concepts:

oneCCL Environment
******************

oneCCL Enviroment is a singleton object which is used as an entry point into oneCCL library and which is defined only for C++ version of API. 
oneCCL Environment exposes a number of helper methods to manage other CCL objects, such as streams, communicators, etc.

oneCCL Stream
*************

**C version of oneCCL API:**

::

    typedef void* ccl_stream_t;

**C++ version of oneCCL API:**

::

    class stream;
    using stream_t = std::unique_ptr<ccl::stream>;

CCL Stream encapsulates execution context for communication primitives declared by oneCCL specification. It is opaque handle managed using a dedicated API:

**C version of oneCCL API:**

::

    ccl_status_t ccl_stream_create(ccl_stream_type_t type,
                                        void* native_stream,
                                        ccl_stream_t* ccl_stream);
    ccl_status_t ccl_stream_free(ccl_stream_t stream);

**C++ version of oneCCL API:**

::

    class environment
    {
    public:
    ...
        /**
        * Creates a new ccl stream of @c type with @c native stream
        * @param type the @c ccl::stream_type and may be @c cpu or @c sycl (if configured)
        * @param native_stream the existing handle of stream
        */
        stream_t create_stream(ccl::stream_type type = ccl::stream_type::cpu, void* native_stream = nullptr) const;
    }

When creating oneCCL stream object using the API described above one needs to specify stream type and pass pointer to the underlying command queue object. 
For example for oneAPI device, ccl::stream_type::sycl and cl::sycl::queue object need to be passed respectively.

oneCCL Communicator
*******************

**C version of oneCCL API:**

::

    typedef void* ccl_comm_t;

**C++ version of oneCCL API:**

::

    class communicator;
    using communicator_t = std::unique_ptr<ccl::communicator>;

oneCCL Communicator defines participants of collective communication operations. It is opaque handle managed using a dedicated API:

**C version of oneCCL API:**

::

    ccl_status_t ccl_comm_create(ccl_comm_t* comm,
                                        const ccl_comm_attr_t* attr);
    ccl_status_t ccl_comm_free(ccl_comm_t comm);

**C++ version of oneCCL API:**

::

    class environment
    {
    public:
    ...
        /**
        * Creates a new communicator according to @c attr parameters
        * or creates a copy of global communicator, if @c attr is @c nullptr(default)
        * @param attr
        */
        communicator_t create_communicator(const ccl::comm_attr* attr = nullptr) const;
    }

When creating oneCCL Communicator one can optionally specify attributes, which control the runtime behavior of oneCCL implementation.

oneCCL Communicator Attributes
******************************

::

    typedef struct
    {
        /**
        * Used to split global communicator into parts. Ranks with identical color
        * will form a new communicator.
        */
        int color;
    } ccl_comm_attr_t;

ccl_comm_attr_t (ccl::comm_attr in C++ version of API) is extendable structure which serves as modificator of communicator behaviour. 

oneCCL Collective Call Attributes
*********************************

::

    /* Extendable list of collective attributes */
    typedef struct
    {
        /** 
        * Callbacks into application code
        * for pre-/post-processing data
        * and custom reduction operation
        */
        ccl_prologue_fn_t prologue_fn;
        ccl_epilogue_fn_t epilogue_fn;
        ccl_reduction_fn_t reduction_fn;
        /* Priority for collective operation */
        size_t priority;
        /* Blocking/non-blocking */
        int synchronous;
        /* Persistent/non-persistent */
        int to_cache;
        /**
        * Id of the operation. If specified, new communicator will be created and collective
        * operations with the same @b match_id will be executed in the same order.
        */
        const char* match_id;
    } ccl_coll_attr_t;

ccl_coll_attr_t (ccl::coll_attr in C++ version of API) is extendable structure which serves as modificator of communication primitive behaviour. 
It can be optionally passed into any collective operation exposed by oneCCL.

oneCCL Request
**************

Each collective communication operation of oneCCL returns a request which can be used to query completion of this operation or block the execution till operation is in progress.
CCL request is an opaque handle which makes sense only within corresponding APIs.

**C version of oneCCL API:**

::

    typedef void* ccl_request_t;

**C++ version of oneCCL API:**

::

    /**
    * A request interface that allows the user to track collective operation progress
    */
    class request
    {
    public:
        /**
        * Blocking wait for collective operation completion
        */
        virtual void wait() = 0;

        /**
        * Non-blocking check for collective operation completion
        * @retval true if the operations has been completed
        * @retval false if the operations has not been completed
        */
        virtual bool test() = 0;

        virtual ~request() = default;
    };
