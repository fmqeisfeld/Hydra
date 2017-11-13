/*
 *  Copyright 2008-2013 NVIDIA Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#pragma once

#include <hydra/detail/external/thrust/detail/config.h>
#include <hydra/detail/external/thrust/system/cuda/detail/execution_policy.h>
#include <hydra/detail/external/thrust/detail/raw_pointer_cast.h>
#include <hydra/detail/external/thrust/system/cuda/detail/guarded_cuda_runtime_api.h>
#include <hydra/detail/external/thrust/system/system_error.h>
#include <hydra/detail/external/thrust/system/cuda/error.h>
#include <hydra/detail/external/thrust/system/detail/bad_alloc.h>
#include <hydra/detail/external/thrust/system/cuda/detail/throw_on_error.h>
#include <hydra/detail/external/thrust/detail/malloc_and_free.h>
#include <hydra/detail/external/thrust/detail/seq.h>


HYDRA_EXTERNAL_NAMESPACE_BEGIN  namespace thrust
{
namespace system
{
namespace cuda
{
namespace detail
{


// note that malloc returns a raw pointer to avoid
// depending on the heavyweight hydra/detail/external/thrust/system/cuda/memory.h header
template<typename DerivedPolicy>
__host__ __device__
void *malloc(execution_policy<DerivedPolicy> &, std::size_t n)
{
  void *result = 0;

#ifndef __CUDA_ARCH__
  // XXX use cudaMalloc in __device__ code when it becomes available
  cudaError_t error = cudaMalloc(reinterpret_cast<void**>(&result), n);

  if(error)
  {
    throw thrust::system::detail::bad_alloc(thrust::cuda_category().message(error).c_str());
  } // end if
#else
  result = thrust::raw_pointer_cast(thrust::malloc(thrust::seq, n));
#endif

  return result;
} // end malloc()


template<typename DerivedPolicy, typename Pointer>
__host__ __device__
void free(execution_policy<DerivedPolicy> &, Pointer ptr)
{
#ifndef __CUDA_ARCH__
  // XXX use cudaFree in __device__ code when it becomes available
  throw_on_error(cudaFree(thrust::raw_pointer_cast(ptr)), "cudaFree in free");
#else
  thrust::free(thrust::seq, ptr);
#endif
} // end free()


} // end detail
} // end cuda
} // end system
} // end thrust

HYDRA_EXTERNAL_NAMESPACE_END