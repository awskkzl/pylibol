/*********************************************************************************
*     File Name           :     compress.h
*     Created By          :     yuewu
*     Creation Date       :     [2015-11-13 21:14]
*     Last Modified       :     [2015-11-14 17:39]
*     Description         :     compressor for binary data format
**********************************************************************************/

#ifndef SOL_PARIO_COMPRESS_H__
#define SOL_PARIO_COMPRESS_H__

#if defined(_MSC_VER) && defined(_DEBUG)
#include <cassert>
#endif

#include <sol/math/vector.h>
#include <sol/util/types.h>

namespace sol {
namespace pario {

// encode  an unsigned int with run length encoding
// if encode signed int, first map it to unsigned with ZigZag Encoding
inline void run_len_encode(math::Vector<char>& codes, uint64_t i) {
  // store an int 7 bits at a time.
  while (i >= 128) {
    codes.push_back((i & 127) | 128);
    i = i >> 7;
  }
  codes.push_back((i & 127));
}

inline const char* run_len_decode(
    const char* p, uint64_t& i) {  // read an int 7 bits at a time.
  size_t count = 0;
  while (*p & 128) i = i | ((*(p++) & 127) << 7 * count++);
  i = i | (*(p++) << 7 * count);
  return p;
}

/**
 * comp : compress the index list, note that the indexes must be sorted from
 * small to big
 *  Note: the function will not erase codes by iteself
 *
 * @Param indexes: indexes to be encoded
 * @Param codes: ouput codes
 */
template <typename T, typename index_type_traits<T>::type* = nullptr>
inline void comp_index(const math::Vector<T>& indexes,
                       math::Vector<char>& codes) {
  T last = 0;
  size_t feat_num = indexes.size();
  for (size_t i = 0; i < feat_num; i++) {
    run_len_encode(codes, indexes[i] - last);
    last = indexes[i];
  }
}

/**
 * decomp_index : de-compress the codes to indexes
 *
 * @Param codes: input codes
 * @Param indexes: output indexes
 */
template <typename T, typename index_type_traits<T>::type* = nullptr>
inline void decomp_index(const math::Vector<char>& codes,
                         math::Vector<T>& indexes) {
  // size_t sz = indexes.size();
  indexes.clear();
  uint64_t last = 0;
  uint64_t index = 0;

  const char* p = codes.begin();
  while (p < codes.end()) {
    index = 0;
    p = run_len_decode(p, index);
    index += last;
    last = index;
    indexes.push_back(T(index));
  }
#if defined(_MSC_VER) && defined(_DEBUG)
  assert(p == codes.end());
#endif
}

}  // namespace pario
}  // namespace sol
#endif
