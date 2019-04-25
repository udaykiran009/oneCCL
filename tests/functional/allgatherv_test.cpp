#include "base.hpp"
#include <functional>
#include <vector>
#include <chrono>

template < typename T > class AllGathervTest:public BaseTest < T > {
public:
	std::vector<size_t> recvCounts;
	std::vector<size_t> offset;
	int Check(TypedTestParam < T > &param) {
		for (size_t j = 0; j < param.bufferCount; j++) {
			for (size_t i = 0; i < param.processCount; i++) {
				for (size_t k = 0; k < recvCounts[i]; k++) {
					size_t idx = offset[i] + k;
					T expected = static_cast<T>(i + k);
					if (param.recvBuf[j][idx] != expected) {
						sprintf(this->errMessage, "[%zu] got recvBuf[%zu][%zu]  = %f, but expected = %f\n",
							param.processIdx, j, idx, (double) param.recvBuf[j][idx], (double) expected);
						return TEST_FAILURE;
					}

					if ((param.GetPlaceType() == PT_IN) && (param.sendBuf[j][idx] != expected)) {
						sprintf(this->errMessage, "[%zu] got sendBuf[%zu][%zu]  = %f, but expected = %f\n",
							param.processIdx, j, idx, (double) param.sendBuf[j][idx], (double) expected);
						return TEST_FAILURE;
					}
				}
			}

			if (param.GetPlaceType() == PT_OOP) {
				for (size_t i = 0; i < param.processCount; i++) {
					for (size_t k = 0; k < recvCounts[i]; k++) {
						T expected = (param.processIdx + i + k);
						if (param.sendBuf[j][offset[i] + k] != expected) {
							sprintf(this->errMessage,
								"[%zu] got sendBuf[%zu][%zu] = %f, but expected = %f\n",
								param.processIdx, j, i, (double) param.sendBuf[j][i], (double) expected);
							return TEST_FAILURE;
						}
					}
				}
			}
		}
		return TEST_SUCCESS;
	}

	int Run(TypedTestParam < T > &param) {
		recvCounts.resize(param.processCount);
		offset.resize(param.processCount);
		offset[0] = 0;
		recvCounts[0] = param.elemCount;

		for (size_t i = 1; i < param.processCount; i++) {
			if (param.elemCount > i)
				recvCounts[i] = param.elemCount - i;
			else
				recvCounts[i] = param.elemCount;

			offset[i] = recvCounts[i - 1] + offset[i - 1];
		}
		for (size_t j = 0; j < param.bufferCount; j++) {
			for (size_t i = 0; i < param.processCount; i++) {
				for (size_t k = 0; k < recvCounts[i]; k++) {
					param.sendBuf[j][offset[i] + k] = param.processIdx + i + k;
					if (param.GetPlaceType() != PT_IN)
						param.recvBuf[j][offset[i] + k] = static_cast<T>SOME_VALUE;
				}
			}
			/* in case of in-place i-th process already has result in i-th block of send buffer */
			if (param.GetPlaceType() == PT_IN) {
				for (size_t i = 0; i < recvCounts[param.processIdx]; i++) {
					param.sendBuf[j][offset[param.processIdx] + i] = param.processIdx + i;
				}
			}
		}	
		size_t idx = 0;
		for (idx = 0; idx < param.bufferCount; idx++) {	
			BaseTest<T>::Init (param);
			param.req[idx] =
				param.global_comm.allgatherv(param.sendBuf[idx].data(), recvCounts[param.processIdx], param.recvBuf[idx].data(),
				recvCounts.data(), (mlsl::data_type) param.GetDataType(), &param.coll_attr);
		}
		for (idx = 0; idx < param.bufferCount; idx++)
		{

			param.CompleteRequest(param.req[idx]);
		}
		int result = Check(param);
		return result;
	}
};

RUN_METHOD_DEFINITION(AllGathervTest);
TEST_CASES_DEFINITION(AllGathervTest);
MAIN_FUNCTION();
