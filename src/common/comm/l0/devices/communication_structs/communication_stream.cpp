#include "common/comm/l0/devices/communication_structs/communication_strea.hpp"


namespace native {
communication_stream::communication_stream(ccl_device& device, std::shared_ptr<ccl_context> ctx)
{
    device_queue = std::make_shared<ccl_device::device_queue>(device->create_cmd_queue(ctx));
    referenced_communication_device_count = 0;
}

}
