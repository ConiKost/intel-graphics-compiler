/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CM_CL_DETAIL_BUILTINS_H
#define CM_CL_DETAIL_BUILTINS_H

#include "vector_impl.h"

#include <opencl_def.h>

//=========================== builtin declarations ===========================//

extern "C" void __cm_cl_select(void *res, void *cond, void *true_val,
                               void *false_val);
extern "C" void __cm_cl_rdregion_int(void *res, void *src, int vstride,
                                     int width, int stride,
                                     cm::detail::vector_offset_type offset);
extern "C" void __cm_cl_rdregion_float(void *res, void *src, int vstride,
                                       int width, int stride,
                                       cm::detail::vector_offset_type offset);
extern "C" void __cm_cl_wrregion_int(void *res, void *src, int vstride,
                                     int width, int stride,
                                     cm::detail::vector_offset_type offset);
extern "C" void __cm_cl_wrregion_float(void *res, void *src, int vstride,
                                       int width, int stride,
                                       cm::detail::vector_offset_type offset);
// FIXME: for legacy issues 64-bit pointer is always returned.
extern "C" uint64_t __cm_cl_printf_buffer();
extern "C" int __cm_cl_printf_format_index(__constant char *str);
extern "C" int __cm_cl_printf_format_index_legacy(__private char *str);
extern "C" void __cm_cl_svm_scatter(int num_blocks, void *address, void *src);
extern "C" void __cm_cl_svm_atomic_add(void *dst, void *address, void *src);

namespace cm {
namespace detail {

//========================= soft implementation part =========================//
//
// This implementations are enabled via CM_CL_SOFT_BUILTINS option.
// The idea is to implement all builtins using only vanilla clang means, without
// requirment of cm-cl builtins support.

template <typename T, int width>
vector_impl<T, width> select_impl(vector_impl<char, width> cond,
                                  vector_impl<T, width> true_val,
                                  vector_impl<T, width> false_val) {
  vector_impl<T, width> res;
  for (int i = 0; i != width; ++i)
    res[i] = cond[i] ? true_val[i] : false_val[i];
  return res;
}

//============================= helper functions =============================//

constexpr bool is_legal_region(int vwidth, int vstride, int width, int stride) {
  return vwidth > 0 && width > 0 && vstride >= 0 && stride >= 0;
}

constexpr int encode_num_blocks(int num_blocks) {
  switch (num_blocks) {
  case 1:
    return 0;
  case 2:
    return 1;
  case 4:
    return 2;
  case 8:
    return 3;
  default:
    return -1;
  }
}

//============================= builtin wrappers =============================//

template <typename T, int width>
vector_impl<T, width> select(vector_impl<char, width> cond,
                             vector_impl<T, width> true_val,
                             vector_impl<T, width> false_val) {
#ifdef CM_CL_SOFT_BUILTINS
  return select_impl(cond, true_val, false_val);
#else  // CM_CL_SOFT_BUILTINS
  vector_impl<T, width> res;
  __cm_cl_select(&res, &cond, &true_val, &false_val);
  return res;
#endif // CM_CL_SOFT_BUILTINS
}

// Unlike __cm_cl_rdregion \p offset here is in T elements, not bytes.
template <int vwidth, int vstride, int width, int stride, typename T,
          int src_width>
vector_impl<T, vwidth * width> read_region(vector_impl<T, src_width> src,
                                           vector_offset_type offset) {
  static_assert(is_legal_region(vwidth, vstride, width, stride),
                "provided region is illegal");
  if constexpr (width == 1 && vwidth == 1)
    return src[offset];
  else {
    vector_impl<T, vwidth * width> res;
    if constexpr (cl::is_floating_point<T>::value)
      __cm_cl_rdregion_float(&res, &src, vstride, width, stride,
                             offset * sizeof(T));
    else
      __cm_cl_rdregion_int(&res, &src, vstride, width, stride,
                           offset * sizeof(T));
    return res;
  }
}

// Unlike __cm_cl_wrregion \p offset here is in T elements, not bytes.
template <int vstride, int width, int stride, typename T, int dst_width,
          int src_width>
void write_region(vector_impl<T, dst_width> &dst, vector_impl<T, src_width> src,
                  vector_offset_type offset) {
  static_assert(is_legal_region((src_width / width), vstride, width, stride) &&
                    src_width % width == 0,
                "provided region is illegal");
  if constexpr (src_width == 1 && width == 1)
    dst[offset] = src[0];
  else {
    if constexpr (cl::is_floating_point<T>::value)
      __cm_cl_wrregion_float(&dst, &src, vstride, width, stride,
                             offset * sizeof(T));
    else
      __cm_cl_wrregion_int(&dst, &src, vstride, width, stride,
                           offset * sizeof(T));
  }
}

inline __global void *printf_buffer() {
  // FIXME: for legacy issues 64-bit pointer is always returned.
  auto ptr = static_cast<uintptr_t>(__cm_cl_printf_buffer());
  return reinterpret_cast<__global void *>(ptr);
}

inline int printf_format_index(__constant char *str) {
  return __cm_cl_printf_format_index(str);
}

inline int printf_format_index(__private char *str) {
  return __cm_cl_printf_format_index_legacy(str);
}

template <int num_blocks, typename T, int width>
void svm_scatter(vector_impl<uintptr_t, width> address,
                 vector_impl<T, num_blocks * width> src) {
  static_assert(sizeof(T) == 1 || sizeof(T) == 4 || sizeof(T) == 8,
                "invalid type");
  constexpr auto lowered_num_blocks = encode_num_blocks(num_blocks);
  static_assert(lowered_num_blocks >= 0, "invalid number of blocks");
  __cm_cl_svm_scatter(lowered_num_blocks, &address, &src);
}

template <typename T, int width>
vector_impl<T, width> svm_atomic_add(vector_impl<uintptr_t, width> address,
                                     vector_impl<T, width> src) {
  vector_impl<T, width> res;
  __cm_cl_svm_atomic_add(&res, &address, &src);
  return res;
}

} // namespace detail
} // namespace cm

#endif // CM_CL_DETAIL_BUILTINS_H
