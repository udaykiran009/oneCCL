Sparse Collective Operations
============================

Language models typically feature huge embedding tables within their topology, which makes straight forward gradient computation
followed by allreduce for the whole set of weights not feasible in practice due to both performance and memory footprint reasons.
Thus, gradients for such layers are usually computed for a smaller sub-tensor each iteration and communication pattern required
to average the gradients across processes doesn't map well to allreduce API. To address these scenarios, frameworks usually utilize
allgather primitive, which may be suboptimal if there's a lot of intersection between sub-tensors from different processes.

Latest research paves the way to handling such communication in more optimal manner but each of these approaches has it's own area
of applicability. Our ultimate goal is to provide a common API for sparse collective operations which would allow implementing different
approaches under the hood and thus simplify framework design.

CCL can work with sparse tensors represented by two tensors: one for indices and one for values.

``sparse_allreduce`` function has following parameters:
-	``send_ind_buf`` - buffer of indices with ``send_ind_count`` elements of ``index_dtype``;
-	``send_int_count`` - number of ``send_ind_buf`` elements of type ``index_type``;
-	``send_val_buf`` - buffer of values with ``send_val_count`` elements of ``value_dtype``;
-	``send_val_count`` - number of ``send_val_buf`` elements of type ``value_type``;
-	``recv_ind_buf`` *[out]* - the buffer to store reduced indices, must have the same dimension as ``send_ind_buf``;
-	``recv_ind_count`` *[out]* - the amount of reduced indices;
-	``recv_val_buf``` *[out]* - the buffer to store reduced values, must have the same dimension as ``send_val_buf``;
-	``recv_val_count`` *[out]* - the amount of reduced values;
-	``index_dtype`` - index type of elements in the buffer ``send_ind_buf`` and ``recv_ind_buf``;
-	``value_dtype`` - data type of elements in the buffer ``send_val_buf`` and ``recv_val_buf``;
-	``reduction`` - type of reduction operation to be applied;
-	``attributes`` - optional attributes that customize operation;
-	return ``iccl::request`` object that can be used to track the progress of the operation.