/*
 * Copyright 2011-2015 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __KERNEL_SPLIT_H__
#define __KERNEL_SPLIT_H__

// clang-format off
#ifndef __KERNEL_METAL__
#include "kernel/kernel_math.h"
#include "kernel/kernel_types.h"

#include "kernel/split/kernel_split_data.h"

#include "kernel/kernel_globals.h"
#include "kernel/kernel_color.h"
#endif // metal

#ifdef __OSL__
#  include "kernel/osl/osl_shader.h"
#endif

#ifdef __KERNEL_OPENCL__
#  include "kernel/kernels/opencl/kernel_opencl_image.h"
#endif
#ifdef __KERNEL_CUDA__
#  include "kernel/kernels/cuda/kernel_cuda_image.h"
#endif
#ifdef __KERNEL_CPU__
#  include "kernel/kernels/cpu/kernel_cpu_image.h"
#endif

#include "util/util_atomic.h"

#include "kernel/kernel_path.h"
#ifdef __BRANCHED_PATH__
#  include "kernel/kernel_path_branched.h"
#endif

#include "kernel/kernel_queues.h"
#include "kernel/kernel_work_stealing.h"

#ifdef __BRANCHED_PATH__
#  include "kernel/split/kernel_branched.h"
#endif
// clang-format on

CCL_NAMESPACE_BEGIN

ccl_device_inline void kernel_split_path_end(__device_space KernelGlobals *kg, int ray_index)
{
  ccl_global __device_space char *ray_state = kernel_split_state.ray_state;

#ifdef __BRANCHED_PATH__
#  ifdef __SUBSURFACE__
  ccl_addr_space __device_space SubsurfaceIndirectRays *ss_indirect = &kernel_split_state.ss_rays[ray_index];

  if (ss_indirect->num_rays) {
    ASSIGN_RAY_STATE(ray_state, ray_index, RAY_UPDATE_BUFFER);
  }
  else
#  endif /* __SUBSURFACE__ */
      if (IS_FLAG(ray_state, ray_index, RAY_BRANCHED_INDIRECT_SHARED)) {
    int orig_ray = kernel_split_state.branched_state[ray_index].original_ray;

    __device_space PathRadiance *L = &kernel_split_state.path_radiance[ray_index];
    __device_space PathRadiance *orig_ray_L = &kernel_split_state.path_radiance[orig_ray];

    path_radiance_sum_indirect(L);
    path_radiance_accum_sample(orig_ray_L, L);

    atomic_fetch_and_dec_uint32(
        (ccl_global __device_space uint *)&kernel_split_state.branched_state[orig_ray].shared_sample_count);

    ASSIGN_RAY_STATE(ray_state, ray_index, RAY_INACTIVE);
  }
  else if (IS_FLAG(ray_state, ray_index, RAY_BRANCHED_LIGHT_INDIRECT)) {
    ASSIGN_RAY_STATE(ray_state, ray_index, RAY_LIGHT_INDIRECT_NEXT_ITER);
  }
  else if (IS_FLAG(ray_state, ray_index, RAY_BRANCHED_VOLUME_INDIRECT)) {
    ASSIGN_RAY_STATE(ray_state, ray_index, RAY_VOLUME_INDIRECT_NEXT_ITER);
  }
  else if (IS_FLAG(ray_state, ray_index, RAY_BRANCHED_SUBSURFACE_INDIRECT)) {
    ASSIGN_RAY_STATE(ray_state, ray_index, RAY_SUBSURFACE_INDIRECT_NEXT_ITER);
  }
  else {
    ASSIGN_RAY_STATE(ray_state, ray_index, RAY_UPDATE_BUFFER);
  }
#else
  ASSIGN_RAY_STATE(ray_state, ray_index, RAY_UPDATE_BUFFER);
#endif
}

CCL_NAMESPACE_END

#endif /* __KERNEL_SPLIT_H__ */
