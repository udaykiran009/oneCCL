#include "base.hpp"
#include <functional>
#include <vector>
#include <chrono>


template <typename T> class BcastTest:public BaseTest <T> {
public:
	int Check(TypedTestParam <T> &param) {
		for (size_t j = 0; j < param.bufferCount; j++) {
			for (size_t i = 0; i < param.elemCount; i++) {
				if (param.sendBuf[j][i] != static_cast<T>(i)) {
					sprintf(this->errMessage, "[%zu] got sendBuf[%zu][%zu] = %f, but expected = %f\n",
						param.processIdx, j, i, (double)param.sendBuf[j][i], (double)i);
					return TEST_FAILURE;
				}
			}
		}
		return TEST_SUCCESS;
	}

	int Run(TypedTestParam <T> &param) {
		for (size_t j = 0; j < param.bufferCount; j++) 
			for (size_t i = 0; i < param.elemCount; i++) {
				if (param.processIdx == ROOT_PROCESS_IDX)
					param.sendBuf[j][i] = i;
				else
					param.sendBuf[j][i] = static_cast<T>(SOME_VALUE);
			}
			size_t idx = 0;
			for (idx = 0; idx < param.bufferCount; idx++) {	
				param.coll_attr.priority = (int)param.PriorityRequest();
				param.coll_attr.to_cache = (int)param.GetCacheType();
				param.coll_attr.synchronous = (int)param.GetSyncType();		
				param.req[idx] = param.global_comm.bcast(param.sendBuf[idx].data(), param.elemCount,
					(mlsl::data_type) param.GetDataType(),
					ROOT_PROCESS_IDX, &param.coll_attr);						
			}
			for (idx = 0; idx < param.bufferCount; idx++) {
				param.CompleteRequest(param.req[idx]);
			}
			int result = Check(param);
			return result;
	}
};

RUN_METHOD_DEFINITION(BcastTest);
TEST_CASES_DEFINITION(BcastTest);
MAIN_FUNCTION();
