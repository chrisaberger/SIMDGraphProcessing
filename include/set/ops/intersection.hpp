#ifndef _INTERSECTION_H_
#define _INTERSECTION_H_

#define GALLOP_SIZE 16
#define VEC_T __m128i
#define COMPILER_LIKELY(x)     __builtin_expect((x),1)
#define COMPILER_RARELY(x)     __builtin_expect((x),0)
/**
 * The following macros (VEC_OR, VEC_ADD_PTEST,VEC_CMP_EQUAL,VEC_SET_ALL_TO_INT,VEC_LOAD_OFFSET,
 * ASM_LEA_ADD_BYTES are only used in the v1 procedure below.
 */
#define VEC_OR(dest, other)                                             \
    __asm volatile("por %1, %0" : "+x" (dest) : "x" (other) )

// // decltype is C++ and typeof is C
#define VEC_ADD_PTEST(var, add, xmm)      {                             \
        decltype(var) _new = var + add;                                   \
        __asm volatile("ptest %2, %2\n\t"                           \
                           "cmovnz %1, %0\n\t"                          \
                           : /* writes */ "+r" (var)                    \
                           : /* reads */  "r" (_new), "x" (xmm)         \
                           : /* clobbers */ "cc");                      \
    }


#define VEC_CMP_EQUAL(dest, other)                                      \
    __asm volatile("pcmpeqd %1, %0" : "+x" (dest) : "x" (other))

#define VEC_SET_ALL_TO_INT(reg, int32)                                  \
    __asm volatile("movd %1, %0; pshufd $0, %0, %0"                 \
                       : "=x" (reg) : "g" (int32) )

#define VEC_LOAD_OFFSET(xmm, ptr, bytes)                    \
    __asm volatile("movdqu %c2(%1), %0" : "=x" (xmm) :  \
                   "r" (ptr), "i" (bytes))

#define ASM_LEA_ADD_BYTES(ptr, bytes)                            \
    __asm volatile("lea %c1(%0), %0\n\t" :                       \
                   /* reads/writes %0 */  "+r" (ptr) :           \
                   /* reads */ "i" (bytes));

namespace ops{
  /**
   * Fast scalar scheme designed by N. Kurz.
   */
  inline size_t scalar(const uint32_t *A, const size_t lenA,
                const uint32_t *B, const size_t lenB, uint32_t *out) {
      const uint32_t *const initout(out);
      if (lenA == 0 || lenB == 0)
          return 0;

      const uint32_t *endA = A + lenA;
      const uint32_t *endB = B + lenB;

      while (1) {
          while (*A < *B) {
  SKIP_FIRST_COMPARE:
              if (++A == endA)
                  return (out - initout);
          }
          while (*A > *B) {
              if (++B == endB)
                  return (out - initout);
          }
          if (*A == *B) {
              #if WRITE_VECTOR == 1
              *out++ = *A;
              #else
              out++;
              #endif
              if (++A == endA || ++B == endB)
                  return (out - initout);
          } else {
              goto SKIP_FIRST_COMPARE;
          }
      }

      return (out - initout); // NOTREACHED
  }
  inline size_t match_scalar(const uint32_t *A, const size_t lenA,
                      const uint32_t *B, const size_t lenB,
                      uint32_t *out) {

      const uint32_t *initout = out;
      if (lenA == 0 || lenB == 0) return 0;

      const uint32_t *endA = A + lenA;
      const uint32_t *endB = B + lenB;

      while (1) {
          while (*A < *B) {
  SKIP_FIRST_COMPARE:
              if (++A == endA) goto FINISH;
          }
          while (*A > *B) {
              if (++B == endB) goto FINISH;
          }
          if (*A == *B) {
              *out++ = *A;
              if (++A == endA || ++B == endB) goto FINISH;
          } else {
              goto SKIP_FIRST_COMPARE;
          }
      }

  FINISH:
      return (out - initout);
  }
  /**
   * Intersections scheme designed by N. Kurz that works very
   * well when intersecting an array with another where the density
   * differential is small (between 2 to 10).
   *
   * It assumes that lenRare <= lenFreq.
   *
   * Note that this is not symmetric: flipping the rare and freq pointers
   * as well as lenRare and lenFreq could lead to significant performance
   * differences.
   *
   * The matchOut pointer can safely be equal to the rare pointer.
   *
   * This function  use inline assembly.
   */
  inline Set<uinteger>* set_intersect_v1(Set<uinteger> *C_in, const Set<uinteger> *A_in, const Set<uinteger> *B_in){
      const uint32_t *rare = (uint32_t*)A_in->data;
      size_t lenRare = A_in->cardinality;
      const uint32_t *freq = (uint32_t*)B_in->data;
      size_t lenFreq = B_in->cardinality;
      uint32_t *matchOut = (uint32_t*)C_in->data;

      assert(lenRare <= lenFreq);
      const uint32_t *matchOrig = matchOut;
      if (lenFreq == 0 || lenRare == 0){
        const size_t density = 0.0;
        C_in->cardinality = 0;
        C_in->number_of_bytes = (0)*sizeof(uint32_t);
        C_in->density = density;
        C_in->type= common::UINTEGER;
        return C_in;
      }

      const uint64_t kFreqSpace = 2 * 4 * (0 + 1) - 1;
      const uint64_t kRareSpace = 0;

      const uint32_t *stopFreq = &freq[lenFreq] - kFreqSpace;
      const uint32_t *stopRare = &rare[lenRare] - kRareSpace;

      VEC_T Rare;

      VEC_T F0, F1;

      if (COMPILER_RARELY((rare >= stopRare) || (freq >= stopFreq))) goto FINISH_SCALAR;

      uint64_t valRare;
      valRare = rare[0];
      VEC_SET_ALL_TO_INT(Rare, valRare);

      uint64_t maxFreq;
      maxFreq = freq[2 * 4 - 1];
      VEC_LOAD_OFFSET(F0, freq, 0 * sizeof(VEC_T)) ;
      VEC_LOAD_OFFSET(F1, freq, 1 * sizeof(VEC_T));

      if (COMPILER_RARELY(maxFreq < valRare)) goto ADVANCE_FREQ;

  ADVANCE_RARE:
      do {
          *matchOut = static_cast<uint32_t>(valRare);
          valRare = rare[1]; // for next iteration
          ASM_LEA_ADD_BYTES(rare, sizeof(*rare)); // rare += 1;

          if (COMPILER_RARELY(rare >= stopRare)) {
              rare -= 1;
              goto FINISH_SCALAR;
          }

          VEC_CMP_EQUAL(F0, Rare) ;
          VEC_CMP_EQUAL(F1, Rare);

          VEC_SET_ALL_TO_INT(Rare, valRare);

          VEC_OR(F0, F1);
  #ifdef __SSE4_1__
          VEC_ADD_PTEST(matchOut, 1, F0);
  #else
          matchOut += static_cast<uint32_t>(_mm_movemask_epi8(F0) != 0);
  #endif

          VEC_LOAD_OFFSET(F0, freq, 0 * sizeof(VEC_T)) ;
          VEC_LOAD_OFFSET(F1, freq, 1 * sizeof(VEC_T));

      } while (maxFreq >= valRare);

      uint64_t maxProbe;

  ADVANCE_FREQ:
      do {
          const uint64_t kProbe = (0 + 1) * 2 * 4;
          const uint32_t *probeFreq = freq + kProbe;
          maxProbe = freq[(0 + 2) * 2 * 4 - 1];

          if (COMPILER_RARELY(probeFreq >= stopFreq)) {
              goto FINISH_SCALAR;
          }

          freq = probeFreq;

      } while (maxProbe < valRare);

      maxFreq = maxProbe;

      VEC_LOAD_OFFSET(F0, freq, 0 * sizeof(VEC_T)) ;
      VEC_LOAD_OFFSET(F1, freq, 1 * sizeof(VEC_T));

      goto ADVANCE_RARE;

      size_t count;
  FINISH_SCALAR:
      count = matchOut - matchOrig;

      lenFreq = stopFreq + kFreqSpace - freq;
      lenRare = stopRare + kRareSpace - rare;

      size_t tail = match_scalar(freq, lenFreq, rare, lenRare, matchOut);

    const size_t density = 0.0;
    C_in->cardinality = count + tail;
    C_in->number_of_bytes = (count+tail)*sizeof(uint32_t);
    C_in->density = density;
    C_in->type= common::UINTEGER;

    return C_in;  
  }


  /**
   * This intersection function is similar to v1, but is faster when
   * the difference between lenRare and lenFreq is large, but not too large.
   * It assumes that lenRare <= lenFreq.
   *
   * Note that this is not symmetric: flipping the rare and freq pointers
   * as well as lenRare and lenFreq could lead to significant performance
   * differences.
   *
   * The matchOut pointer can safely be equal to the rare pointer.
   *
   * This function DOES NOT use inline assembly instructions. Just intrinsics.
   */
  inline Set<uinteger>* set_intersect_v3(Set<uinteger> *C_in, const Set<uinteger> *A_in, const Set<uinteger> *B_in){
      const uint32_t *rare = (uint32_t*)A_in->data;
      const size_t lenRare = A_in->cardinality;
      const uint32_t *freq = (uint32_t*)B_in->data;
      const size_t lenFreq = B_in->cardinality;
      uint32_t *out = (uint32_t*)C_in->data;

      if (lenFreq == 0 || lenRare == 0){
        const size_t density = 0.0;
        C_in->cardinality = 0;
        C_in->number_of_bytes = (0)*sizeof(uint32_t);
        C_in->density = density;
        C_in->type= common::UINTEGER;
        return C_in;
      }
      assert(lenRare <= lenFreq);
      const uint32_t *const initout(out);
      typedef __m128i vec;
      const uint32_t veclen = sizeof(vec) / sizeof(uint32_t);
      const size_t vecmax = veclen - 1;
      const size_t freqspace = 32 * veclen;
      const size_t rarespace = 1;

      const uint32_t *stopFreq = freq + lenFreq - freqspace;
      const uint32_t *stopRare = rare + lenRare - rarespace;
      if (freq > stopFreq) {
        const size_t final_count = scalar(freq, lenFreq, rare, lenRare, out);
        const size_t density = 0.0;
        C_in->cardinality = final_count;
        C_in->number_of_bytes = (final_count)*sizeof(uint32_t);
        C_in->density = density;
        C_in->type= common::UINTEGER;
        return C_in;
      }
      while (freq[veclen * 31 + vecmax] < *rare) {
          freq += veclen * 32;
          if (freq > stopFreq)
              goto FINISH_SCALAR;
      }
      for (; rare < stopRare; ++rare) {
          const uint32_t matchRare = *rare;//nextRare;
          const vec Match = _mm_set1_epi32(matchRare);
          while (freq[veclen * 31 + vecmax] < matchRare) { // if no match possible
              freq += veclen * 32; // advance 32 vectors
              if (freq > stopFreq)
                  goto FINISH_SCALAR;
          }
          vec Q0, Q1, Q2, Q3;
          if (freq[veclen * 15 + vecmax] >= matchRare) {
              if (freq[veclen * 7 + vecmax] < matchRare) {
                  Q0 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 8), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 9), Match));
                  Q1 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 10), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 11), Match));

                  Q2 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 12), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 13), Match));
                  Q3 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 14), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 15), Match));
              } else {
                  Q0 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 4), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 5), Match));
                  Q1 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 6), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 7), Match));
                  Q2 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 0), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 1), Match));
                  Q3 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 2), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 3), Match));
              }
          } else {
              if (freq[veclen * 23 + vecmax] < matchRare) {
                  Q0 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 8 + 16), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 9 + 16), Match));
                  Q1 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 10 + 16), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 11 + 16), Match));

                  Q2 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 12 + 16), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 13 + 16), Match));
                  Q3 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 14 + 16), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 15 + 16), Match));
              } else {
                  Q0 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 4 + 16), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 5 + 16), Match));
                  Q1 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 6 + 16), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 7 + 16), Match));
                  Q2 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 0 + 16), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 1 + 16), Match));
                  Q3 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 2 + 16), Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 3 + 16), Match));
              }

          }
          const vec F0 = _mm_or_si128(_mm_or_si128(Q0, Q1), _mm_or_si128(Q2, Q3));
  #ifdef __SSE4_1__
          if (_mm_testz_si128(F0, F0)) {
  #else 
          if (!_mm_movemask_epi8(F0)) {
  #endif 
          } else {
              *out++ = matchRare;
          }
      }

  FINISH_SCALAR: 
    const size_t final_count = (out - initout) + scalar(freq,stopFreq + freqspace - freq, rare, stopRare + rarespace - rare, out);
    const size_t density = 0.0;
    C_in->cardinality = final_count;
    C_in->number_of_bytes = (final_count)*sizeof(uint32_t);
    C_in->density = density;
    C_in->type= common::UINTEGER;
    return C_in;
  }


  /**
   * This is the SIMD galloping function. This intersection function works well
   * when lenRare and lenFreq have vastly different values.
   *
   * It assumes that lenRare <= lenFreq.
   *
   * Note that this is not symmetric: flipping the rare and freq pointers
   * as well as lenRare and lenFreq could lead to significant performance
   * differences.
   *
   * The matchOut pointer can safely be equal to the rare pointer.
   *
   * This function DOES NOT use assembly. It only relies on intrinsics.
   */

  inline Set<uinteger>* set_intersect_galloping(Set<uinteger> *C_in, const Set<uinteger> *A_in, const Set<uinteger> *B_in){
      const uint32_t *rare = (uint32_t*)A_in->data;
      const size_t lenRare = A_in->cardinality;
      const uint32_t *freq = (uint32_t*)B_in->data;
      const size_t lenFreq = B_in->cardinality;
      uint32_t *out = (uint32_t*)C_in->data;

      if (lenFreq == 0 || lenRare == 0){
        const size_t density = 0.0;
        C_in->cardinality = 0;
        C_in->number_of_bytes = (0)*sizeof(uint32_t);
        C_in->density = density;
        C_in->type= common::UINTEGER;
        return C_in;
      }
      assert(lenRare <= lenFreq);
      const uint32_t *const initout(out);
      typedef __m128i vec;
      const uint32_t veclen = sizeof(vec) / sizeof(uint32_t);
      const size_t vecmax = veclen - 1;
      const size_t freqspace = 32 * veclen;
      const size_t rarespace = 1;

      const uint32_t *stopFreq = freq + lenFreq - freqspace;
      const uint32_t *stopRare = rare + lenRare - rarespace;
      if (freq > stopFreq) {
        const size_t final_count = scalar(freq, lenFreq, rare, lenRare, out);
        const size_t density = 0.0;
        C_in->cardinality = final_count;
        C_in->number_of_bytes = (final_count)*sizeof(uint32_t);
        C_in->density = density;
        C_in->type= common::UINTEGER;
        return C_in;
      }
      for (; rare < stopRare; ++rare) {
          const uint32_t matchRare = *rare;//nextRare;
          const vec Match = _mm_set1_epi32(matchRare);

          if (freq[veclen * 31 + vecmax] < matchRare) { // if no match possible
              uint32_t offset = 1;
              if (freq + veclen  * 32 > stopFreq) {
                  freq += veclen * 32;
                  goto FINISH_SCALAR;
              }
              while (freq[veclen * offset * 32 + veclen * 31 + vecmax]
                     < matchRare) { // if no match possible
                  if (freq + veclen * (2 * offset) * 32 <= stopFreq) {
                      offset *= 2;
                  } else if (freq + veclen * (offset + 1) * 32 <= stopFreq) {
                      offset = static_cast<uint32_t>((stopFreq - freq) / (veclen * 32));
                      //offset += 1;
                      if (freq[veclen * offset * 32 + veclen * 31 + vecmax]
                          < matchRare) {
                          freq += veclen * offset * 32;
                          goto FINISH_SCALAR;
                      } else {
                          break;
                      }
                  } else {
                      freq += veclen * offset * 32;
                      goto FINISH_SCALAR;
                  }
              }
              uint32_t lower = offset / 2;
              while (lower + 1 != offset) {
                  const uint32_t mid = (lower + offset) / 2;
                  if (freq[veclen * mid * 32 + veclen * 31 + vecmax]
                      < matchRare)
                      lower = mid;
                  else
                      offset = mid;
              }
              freq += veclen * offset * 32;
          }
          vec Q0, Q1, Q2, Q3;
          if (freq[veclen * 15 + vecmax] >= matchRare) {
              if (freq[veclen * 7 + vecmax] < matchRare) {
                  Q0
                      = _mm_or_si128(
                            _mm_cmpeq_epi32(
                                _mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 8), Match),
                            _mm_cmpeq_epi32(
                                _mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 9), Match));
                  Q1 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 10),
                                           Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 11),
                                           Match));

                  Q2 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 12),
                                           Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 13),
                                           Match));
                  Q3 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 14),
                                           Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 15),
                                           Match));
              } else {
                  Q0
                      = _mm_or_si128(
                            _mm_cmpeq_epi32(
                                _mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 4), Match),
                            _mm_cmpeq_epi32(
                                _mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 5), Match));
                  Q1
                      = _mm_or_si128(
                            _mm_cmpeq_epi32(
                                _mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 6), Match),
                            _mm_cmpeq_epi32(
                                _mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 7), Match));
                  Q2
                      = _mm_or_si128(
                            _mm_cmpeq_epi32(
                                _mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 0), Match),
                            _mm_cmpeq_epi32(
                                _mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 1), Match));
                  Q3
                      = _mm_or_si128(
                            _mm_cmpeq_epi32(
                                _mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 2), Match),
                            _mm_cmpeq_epi32(
                                _mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 3), Match));
              }
          } else {
              if (freq[veclen * 23 + vecmax] < matchRare) {
                  Q0 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 8 + 16),
                                           Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 9 + 16),
                                           Match));
                  Q1 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 10 + 16),
                                           Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 11 + 16),
                                           Match));

                  Q2 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 12 + 16),
                                           Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 13 + 16),
                                           Match));
                  Q3 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 14 + 16),
                                           Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 15 + 16),
                                           Match));
              } else {
                  Q0 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 4 + 16),
                                           Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 5 + 16),
                                           Match));
                  Q1 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 6 + 16),
                                           Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 7 + 16),
                                           Match));
                  Q2 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 0 + 16),
                                           Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 1 + 16),
                                           Match));
                  Q3 = _mm_or_si128(
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 2 + 16),
                                           Match),
                           _mm_cmpeq_epi32(_mm_loadu_si128(reinterpret_cast<const vec *>(freq) + 3 + 16),
                                           Match));
              }

          }
          const vec F0 = _mm_or_si128(_mm_or_si128(Q0, Q1), _mm_or_si128(Q2, Q3));
  #ifdef __SSE4_1__
          if (_mm_testz_si128(F0, F0)) {
  #else 
          if (!_mm_movemask_epi8(F0)) {
  #endif 
          } else {
              *out++ = matchRare;
          }
      }

  FINISH_SCALAR: 
    const size_t final_count = (out - initout) + scalar(freq,stopFreq + freqspace - freq, rare, stopRare + rarespace - rare, out);
    const size_t density = 0.0;
    C_in->cardinality = final_count;
    C_in->number_of_bytes = (final_count)*sizeof(uint32_t);
    C_in->density = density;
    C_in->type= common::UINTEGER;
    return C_in;
  }

  inline Set<uinteger>* set_intersect_ibm(Set<uinteger> *C_in, const Set<uinteger> *A_in, const Set<uinteger> *B_in){
    uint32_t * const C = (uint32_t*) C_in->data; 
    const uint32_t * const A = (uint32_t*) A_in->data;
    const uint32_t * const B = (uint32_t*) B_in->data;
    const size_t s_a = A_in->cardinality;
    const size_t s_b = B_in->cardinality;

    const size_t st_a = (s_a / SHORTS_PER_REG) * SHORTS_PER_REG;
    const size_t st_b = (s_b / SHORTS_PER_REG) * SHORTS_PER_REG;

    size_t count = 0;
    size_t i_a = 0, i_b = 0;

    while(i_a < st_a && i_b < st_b){
      //Pull in 4 uint 32's
      const __m128i v_a_1_32 = _mm_loadu_si128((__m128i*)&A[i_a]);
      const __m128i v_a_2_32 = _mm_loadu_si128((__m128i*)&A[i_a+(SHORTS_PER_REG/2)]);

      /*cout << endl;
      cout << "ORIGINAL DATA" << endl;
      common::_mm128i_print(v_a_1_32);
      common::_mm128i_print(v_a_2_32);*/


      //shuffle to get lower 16 bits only in one register
      const __m128i v_a_l1 = _mm_shuffle_epi8(v_a_1_32,_mm_set_epi8(uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0)));
      const __m128i v_a_l2 = _mm_shuffle_epi8(v_a_2_32,_mm_set_epi8(uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80)));
      const __m128i v_a_l = _mm_or_si128(v_a_l1,v_a_l2);

      const __m128i v_b_1_32 = _mm_loadu_si128((__m128i*)&B[i_b]);
      const __m128i v_b_2_32 = _mm_loadu_si128((__m128i*)&B[i_b+(SHORTS_PER_REG/2)]);

      //common::_mm128i_print(v_b_1_32);
      //common::_mm128i_print(v_b_2_32);

      const __m128i v_b_l1 = _mm_shuffle_epi8(v_b_1_32,_mm_set_epi8(uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0)));
      const __m128i v_b_l2 = _mm_shuffle_epi8(v_b_2_32,_mm_set_epi8(uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80)));
      const __m128i v_b_l = _mm_or_si128(v_b_l1,v_b_l2);
      
     // __m128i res_v = _mm_cmpistrm(v_b, v_a,
     //         _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
      const __m128i res_vl = _mm_cmpestrm(v_b_l, SHORTS_PER_REG, v_a_l, SHORTS_PER_REG,
              _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
      const uint32_t result_l = _mm_extract_epi32(res_vl, 0);

     // cout << "LOWER " << hex  << result_l << dec << endl;

      if(result_l != 0){
        //shuffle to get upper 16 bits only in one register
        const __m128i v_a_u1 = _mm_shuffle_epi8(v_a_1_32,_mm_set_epi8(uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x0F),uint8_t(0x0E),uint8_t(0x0B),uint8_t(0x0A),uint8_t(0x07),uint8_t(0x06),uint8_t(0x03),uint8_t(0x2)));
        const __m128i v_a_u2 = _mm_shuffle_epi8(v_a_2_32,_mm_set_epi8(uint8_t(0x0F),uint8_t(0x0E),uint8_t(0x0B),uint8_t(0x0A),uint8_t(0x07),uint8_t(0x06),uint8_t(0x03),uint8_t(0x2),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80)));
        const __m128i v_a_u = _mm_or_si128(v_a_u1,v_a_u2);

        const __m128i v_b_1_32 = _mm_loadu_si128((__m128i*)&B[i_b]);
        const __m128i v_b_2_32 = _mm_loadu_si128((__m128i*)&B[i_b+(SHORTS_PER_REG/2)]);

        const __m128i v_b_u1 = _mm_shuffle_epi8(v_b_1_32,_mm_set_epi8(uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x0F),uint8_t(0x0E),uint8_t(0x0B),uint8_t(0x0A),uint8_t(0x07),uint8_t(0x06),uint8_t(0x03),uint8_t(0x2)));
        const __m128i v_b_u2 = _mm_shuffle_epi8(v_b_2_32,_mm_set_epi8(uint8_t(0x0F),uint8_t(0x0E),uint8_t(0x0B),uint8_t(0x0A),uint8_t(0x07),uint8_t(0x06),uint8_t(0x03),uint8_t(0x2),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80)));
        const __m128i v_b_u = _mm_or_si128(v_b_u1,v_b_u2);

        const __m128i res_vu = _mm_cmpestrm(v_b_u, SHORTS_PER_REG, v_a_u, SHORTS_PER_REG,
                _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
        const uint32_t result_u = _mm_extract_epi32(res_vu, 0);

        //cout << "UPPER " << hex  << result_l << dec << endl;

        const uint32_t w_bitmask = result_u & result_l;
        //cout << count << " BITMASK: " << hex  << w_bitmask << dec << endl;
        if(w_bitmask != 0){
          const size_t start_index = _mm_popcnt_u32((~w_bitmask)&(w_bitmask-1));
          const size_t A_pos = start_index+i_a;
          const size_t A_end = 8-start_index;
          const size_t B_pos = i_b;
          const size_t B_end = 8;
          count += scalar(&A[A_pos],A_end,&B[B_pos],B_end,&C[count]);
        }
      } 
      if(A[i_a+7] > B[i_b+7]){
        goto advanceB;
      } else if (A[i_a+7] < B[i_b+7]){
        goto advanceA;
      } else{
        goto advanceAB;
      }
      advanceA:
        i_a += 8;
        continue;
      advanceB:
        i_b += 8;
        continue;
      advanceAB:
        i_a += 8;
        i_b += 8;
        continue;
    }

    // intersect the tail using scalar intersection
    count += scalar(&A[i_a],s_a-i_a,&B[i_b],s_b-i_b,&C[count]);

    //XXX: Fix
    const double density = 0.0;//((count > 0) ? ((double)count/(C[count]-C[0])) : 0.0);

    C_in->cardinality = count;
    C_in->number_of_bytes = count*sizeof(uint32_t);
    C_in->density = density;
    C_in->type= common::UINTEGER;

    return C_in;
  }

  inline Set<uinteger>* set_intersect_standard(Set<uinteger> *C_in, const Set<uinteger> *A_in, const Set<uinteger> *B_in){
    uint32_t * const C = (uint32_t*) C_in->data; 
    const uint32_t * const A = (uint32_t*) A_in->data;
    const uint32_t * const B = (uint32_t*) B_in->data;
    const size_t s_a = A_in->cardinality;
    const size_t s_b = B_in->cardinality;

    size_t count = 0;
    size_t i_a = 0, i_b = 0;

    // trim lengths to be a multiple of 4
    #if VECTORIZE == 1
    size_t st_a = (s_a / 4) * 4;
    size_t st_b = (s_b / 4) * 4;
    while(i_a < st_a && i_b < st_b) {
      //[ load segments of four 32-bit elements
      __m128i v_a = _mm_loadu_si128((__m128i*)&A[i_a]);
      __m128i v_b = _mm_loadu_si128((__m128i*)&B[i_b]);
      //]

      //[ move pointers
      uint32_t a_max = A[i_a+3];
      uint32_t b_max = B[i_b+3];
      i_a += (a_max <= b_max) * 4;
      i_b += (a_max >= b_max) * 4;
      //]

      //[ compute mask of common elements
      uint32_t right_cyclic_shift = _MM_SHUFFLE(0,3,2,1);
      __m128i cmp_mask1 = _mm_cmpeq_epi32(v_a, v_b);    // pairwise comparison
      v_b = _mm_shuffle_epi32(v_b, right_cyclic_shift);       // shuffling
      __m128i cmp_mask2 = _mm_cmpeq_epi32(v_a, v_b);    // again...
      v_b = _mm_shuffle_epi32(v_b, right_cyclic_shift);
      __m128i cmp_mask3 = _mm_cmpeq_epi32(v_a, v_b);    // and again...
      v_b = _mm_shuffle_epi32(v_b, right_cyclic_shift);
      __m128i cmp_mask4 = _mm_cmpeq_epi32(v_a, v_b);    // and again.
      __m128i cmp_mask = _mm_or_si128(
              _mm_or_si128(cmp_mask1, cmp_mask2),
              _mm_or_si128(cmp_mask3, cmp_mask4)
      ); // OR-ing of comparison masks
      // convert the 128-bit mask to the 4-bit mask
      uint32_t mask = _mm_movemask_ps((__m128)cmp_mask);
      //]

      //[ copy out common elements
      #if WRITE_VECTOR == 1
      //cout << "mask: " << mask << endl;
      __m128i p = _mm_shuffle_epi8(v_a, shuffle_mask32[mask]);
      _mm_storeu_si128((__m128i*)&C[count], p);
      //cout << "C[" << count << "]: " << C[count] << endl;

      #endif

      count += _mm_popcnt_u32(mask); // a number of elements is a weight of the mask
      //]
    }
    #endif

    // intersect the tail using scalar intersection
    count += scalar(&A[i_a],s_a-i_a,&B[i_b],s_b-i_b,&C[count]);

    #if WRITE_VECTOR == 0
    (void) C;
    #endif

    //XXX: Fix
    const double density = 0.0;//((count > 0) ? ((double)count/(C[count]-C[0])) : 0.0);
    
    C_in->cardinality = count;
    C_in->number_of_bytes = count*sizeof(uint32_t);
    C_in->density = density;
    C_in->type= common::UINTEGER;

    return C_in;  
  }
  
 inline size_t simd_intersect_vector16(uint16_t *C, const uint16_t *A, const uint16_t *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    (void)C;
    #endif
    
    size_t count = 0;
    size_t i_a = 0, i_b = 0;

    #if VECTORIZE == 1
    size_t st_a = (s_a / SHORTS_PER_REG) * SHORTS_PER_REG;
    size_t st_b = (s_b / SHORTS_PER_REG) * SHORTS_PER_REG;

    while(i_a < st_a && i_b < st_b) {
      __m128i v_a = _mm_loadu_si128((__m128i*)&A[i_a]);
      __m128i v_b = _mm_loadu_si128((__m128i*)&B[i_b]);    

      uint16_t a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
      uint16_t b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
      
     // __m128i res_v = _mm_cmpistrm(v_b, v_a,
     //         _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
      __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
              _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
      uint32_t r = _mm_extract_epi32(res_v, 0);

      #if WRITE_VECTOR == 1
      __m128i p = _mm_shuffle_epi8(v_a, shuffle_mask16[r]);
      _mm_storeu_si128((__m128i*)&C[count], p);
     #endif

      count += _mm_popcnt_u32(r);
      
      i_a += (a_max <= b_max) * SHORTS_PER_REG;
      i_b += (a_max >= b_max) * SHORTS_PER_REG;
    }
    #endif

    // intersect the tail using scalar intersection
    //...
    bool notFinished = i_a < s_a  && i_b < s_b;
    while(notFinished){
      while(notFinished && B[i_b] < A[i_a]){
        ++i_b;
        notFinished = i_b < s_b;
      }
      if(notFinished && A[i_a] == B[i_b]){
        #if WRITE_VECTOR == 1
        C[count] = A[i_a];
        #endif
        ++count;
      }
      ++i_a;
      notFinished = notFinished && i_a < s_a;
    }
    return count;
  }
  inline Set<pshort>* set_intersect(Set<pshort> *C_in, const Set<pshort> *A_in, const Set<pshort> *B_in){
    uint16_t * const C = (uint16_t*)C_in->data;
    const uint16_t * const A = (uint16_t*)A_in->data;
    const uint16_t * const B = (uint16_t*)B_in->data;
    const size_t s_a = A_in->number_of_bytes/sizeof(uint16_t);
    const size_t s_b = B_in->number_of_bytes/sizeof(uint16_t);

    size_t i_a = 0, i_b = 0;
    size_t counter = 0;
    size_t count = 0;
    bool notFinished = i_a < s_a && i_b < s_b;


    //We add 3, 2 for the prefix and the size and 1 because the size is stored -1
    while(notFinished) {
      //size_t limLower = limLowerHolder;
      if(A[i_a] < B[i_b]) {
        i_a += A[i_a + 1] + 3;
        notFinished = i_a < s_a;
      } else if(B[i_b] < A[i_a]) {
        i_b += B[i_b + 1] + 3;
        notFinished = i_b < s_b;
      } else {
        uint16_t partition_size = 0;
        //If we are not in the range of the limit we don't need to worry about it.
        #if WRITE_VECTOR == 1
        C[counter++] = A[i_a]; // write partition prefix
        #endif
        partition_size = simd_intersect_vector16(&C[counter+1],&A[i_a + 2],&B[i_b + 2],((size_t)A[i_a+1])+1,((size_t)B[i_b+1])+1);
        #if WRITE_VECTOR == 1
        C[counter++] = (partition_size-1); // write partition size
        #endif
        i_a += A[i_a+1] + 3;
        i_b += B[i_b+1] + 3;      

        counter += (partition_size > 0) ? (partition_size):(-2);
        count += partition_size;
        notFinished = i_a < s_a && i_b < s_b;
      }
    }

    const double density = 0.0;

    C_in->cardinality = count;
    C_in->number_of_bytes = counter*sizeof(uint16_t);
    C_in->density = density;
    C_in->type= common::PSHORT;

    return C_in;
}

inline Set<bitset>* set_intersect(Set<bitset> *C_in, const Set<bitset> *A_in, const Set<bitset> *B_in){
    long count = 0l;
    C_in->number_of_bytes = 0;

    if(A_in->number_of_bytes > 0 && B_in->number_of_bytes > 0){
      const uint64_t *a_index = (uint64_t*) A_in->data;
      const uint64_t *b_index = (uint64_t*) B_in->data;

      uint64_t * const C = (uint64_t*)(C_in->data+sizeof(uint64_t));
      const uint64_t * const A = (uint64_t*)(A_in->data+sizeof(uint64_t));
      const uint64_t * const B = (uint64_t*)(B_in->data+sizeof(uint64_t));
      const size_t s_a = ((A_in->number_of_bytes-sizeof(uint64_t))/sizeof(uint64_t));
      const size_t s_b = ((B_in->number_of_bytes-sizeof(uint64_t))/sizeof(uint64_t));

      #if WRITE_VECTOR == 0
      (void) C;
      #endif

      const bool a_big = a_index[0] > b_index[0];
      const uint64_t start_index = (a_big) ? a_index[0] : b_index[0];
      const uint64_t a_start_index = (a_big) ? 0:(b_index[0]-a_index[0]);
      const uint64_t b_start_index = (a_big) ? (a_index[0]-b_index[0]):0;

      const uint64_t end_index = ((a_index[0]+s_a) > (b_index[0]+s_b)) ? (b_index[0]+s_b):(a_index[0]+s_a);
      const uint64_t total_size = (start_index > end_index) ? 0:(end_index-start_index);

      //16 uint16_ts
      //8 ints
      //4 longs
      size_t i = 0;

      #if WRITE_VECTOR == 1
      uint64_t *c_index = (uint64_t*) C_in->data;
      c_index[0] = start_index;
      #endif

      #if VECTORIZE == 1
      uint64_t tmp[4];
      while((i+3) < total_size){
        const __m256 a1 = _mm256_loadu_ps((const float*)(A + i + a_start_index));
        const __m256 a2 = _mm256_loadu_ps((const float*)(B + i + b_start_index));
        const __m256 r = _mm256_and_ps(a2, a1);

        #if WRITE_VECTOR == 1
        _mm256_storeu_ps((float*)(C + i), r);
        #endif

        _mm256_storeu_ps((float*)tmp, r);
        count += _mm_popcnt_u64(tmp[0]);
        count += _mm_popcnt_u64(tmp[1]);
        count += _mm_popcnt_u64(tmp[2]);
        count += _mm_popcnt_u64(tmp[3]);

        i += 4;
      }
      #endif

      for(; i < total_size; i++){
        const uint64_t result = A[i+a_start_index] & B[i+b_start_index];

        #if WRITE_VECTOR == 1
        C[i] = result;
        #endif

        count += _mm_popcnt_u64(result);
      }
      C_in->number_of_bytes = total_size*sizeof(uint64_t)+sizeof(uint32_t);
    }
    const double density = 0.0;//(count > 0) ? (double)count/(8*small_length) : 0.0;

    C_in->cardinality = count;
    C_in->density = density;
    C_in->type= common::BITSET;

    return C_in;
  }
  inline Set<pshort>* set_intersect(Set<pshort> *C_in, const Set<pshort> *A_in, const Set<bitset> *B_in){
    uint16_t * const C = (uint16_t*)C_in->data;
    const uint16_t * const A = (uint16_t*)A_in->data;
    const uint64_t start_index = (B_in->number_of_bytes > 0) ? ((uint64_t*)B_in->data)[0]:0;

    const uint64_t * const B = (uint64_t*)(B_in->data+sizeof(uint64_t));
    const size_t s_a = A_in->number_of_bytes / sizeof(uint16_t);
    const size_t s_b = (B_in->number_of_bytes > 0) ? (B_in->number_of_bytes-sizeof(uint64_t))/sizeof(uint64_t):0;

    #if WRITE_VECTOR == 0
    (void) C;
    #endif

    size_t count = 0;
    size_t write_pos = 0;
    for(size_t i = 0; i < s_a;) {
      const uint32_t prefix = (A[i] << 16);
      const uint32_t size = A[i+1];
      i += 2;

      const size_t part_header_pos = write_pos;
      size_t part_count = 0;
      const size_t part_end = i + size;
      const size_t prefix_index = bitset::word_index(prefix);
      //65536/64 = 1024, that is there are 1024 bitset indicies in one partition
      if(prefix_index < (s_b+start_index) && (prefix_index+1024) >= start_index){
        write_pos += 2;
        for(; i <= part_end; i++) {
          const uint32_t cur = prefix | A[i];
          const size_t cur_index = bitset::word_index(cur);

          //Why not do bounds check in is_set?
          if((cur_index < (s_b+start_index)) && (cur_index >= start_index) && bitset::is_set(cur,B,start_index)){
            #if WRITE_VECTOR == 1
            C[write_pos++] = A[i];
            #endif
            part_count++;
            count++;
          }
        }
        if(part_count != 0) {
          write_pos -= 2;
        } else{
          C[part_header_pos] = (prefix >> 16);
          C[part_header_pos + 1] = part_count - 1;
        }
      } else if(prefix_index >= (s_b+start_index)){
        break;
      }
      i = part_end+1;
    }

    // XXX: Correct density computation
    const double density = 0.0;

    C_in->cardinality = count;
    C_in->number_of_bytes = write_pos * sizeof(uint16_t);
    C_in->density = density;
    C_in->type= common::PSHORT;

    return C_in;
  }

  inline Set<pshort>* set_intersect(Set<pshort> *C_in,const Set<bitset> *A_in,const Set<pshort> *B_in){
    return set_intersect(C_in,B_in,A_in);
  }

  inline Set<uinteger>* set_intersect(Set<uinteger> *C_in,const Set<uinteger> *A_in,const Set<bitset> *B_in){
    uint32_t * const C = (uint32_t*)C_in->data;
    const uint32_t * const A = (uint32_t*)A_in->data;
    const size_t s_a = A_in->cardinality;
    const size_t s_b = (B_in->number_of_bytes > 0) ? (B_in->number_of_bytes-sizeof(uint64_t))/sizeof(uint64_t):0;
    const uint64_t start_index = (B_in->number_of_bytes > 0) ? ((uint64_t*)B_in->data)[0]:0;

    const uint64_t * const B = (uint64_t*)(B_in->data+sizeof(uint64_t));

    #if WRITE_VECTOR == 0
    (void) C;
    #endif

    size_t count = 0;
    for(size_t i = 0; i < s_a; i++){
      const uint32_t cur = A[i];
      const size_t cur_index = bitset::word_index(cur);
      if((cur_index < (s_b+start_index)) && (cur_index >= start_index) && bitset::is_set(cur,B,start_index)){
        #if WRITE_VECTOR == 1
        C[count] = cur;
        #endif
        count++;
      } else if(cur_index >= (s_b+start_index)){
        break;
      }
    }
    // XXX: Correct density computation
    const double density = 0.0;//((count > 1) ? ((double)count/(C[count - 1]-C[0])) : 0.0);

    C_in->cardinality = count;
    C_in->number_of_bytes = count*sizeof(uint32_t);
    C_in->density = density;
    C_in->type= common::UINTEGER;

    return C_in;
  }
  inline Set<uinteger>* set_intersect(Set<uinteger> *C_in,const Set<bitset> *A_in,const Set<uinteger> *B_in){
    return set_intersect(C_in,B_in,A_in);
  }
  inline Set<uinteger>* set_intersect(Set<uinteger> *C_in,const Set<uinteger> *A_in,const Set<pshort> *B_in){
    uint32_t * const C = (uint32_t*)C_in->data;
    const uint32_t * const A = (uint32_t*)A_in->data;
    const uint16_t * const B = (uint16_t*)B_in->data;
    const size_t s_a = A_in->cardinality;
    const size_t s_b = B_in->number_of_bytes/sizeof(uint16_t);

    #if WRITE_VECTOR == 0
    (void)C;
    #endif

    size_t a_i = 0;
    size_t b_i = 0;
    size_t count = 0;

    bool not_finished = a_i < s_a && b_i < s_b;
    while(not_finished){
      uint32_t prefix = (B[b_i] << 16);
      uint16_t b_inner_size = B[b_i+1]+1;
      uint32_t cur_match = A[a_i];
      size_t inner_end = b_i+b_inner_size+2;
      //cout << endl;
      //cout << "Bi: " << b_i << " Bsize: " << s_b << " InnerEnd: " << inner_end << endl;

      if(prefix < (cur_match & 0xFFFF0000)){
        //cout << "1" << endl;
        b_i = inner_end;
        not_finished = b_i < s_b;
      } else if(prefix > cur_match){
        //cout << prefix << " " << cur_match << endl;
        //cout << "2" << endl;
        a_i++;
        not_finished = a_i < s_a;
      } else{
        //cout << "3" << endl;
        b_i += 2;
        size_t i_b = 0;

        #if VECTORIZE == 1
        bool a_continue = (a_i+SHORTS_PER_REG) < s_a && (A[a_i+SHORTS_PER_REG-1] & 0xFFFF0000) == prefix;
        size_t st_b = (b_inner_size / SHORTS_PER_REG) * SHORTS_PER_REG;
        while(a_continue && i_b < st_b) {
          __m128i v_a_1_32 = _mm_loadu_si128((__m128i*)&A[a_i]);
          __m128i v_a_2_32 = _mm_loadu_si128((__m128i*)&A[a_i+(SHORTS_PER_REG/2)]);

          __m128i v_a_1 = _mm_shuffle_epi8(v_a_1_32,_mm_set_epi8(uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0)));
          __m128i v_a_2 = _mm_shuffle_epi8(v_a_2_32,_mm_set_epi8(uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80)));
          
          __m128i v_a = _mm_or_si128(v_a_1,v_a_2);
            
          //uint16_t *t = (uint16_t*) &v_a;
          //cout << "Data: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << " " << t[4] << " " << t[5] << " " << t[6] << " " << t[7] << endl;

          __m128i v_b = _mm_loadu_si128((__m128i*)&B[b_i+i_b]);    

          uint16_t a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
          uint16_t b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
          
          __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
                  _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
          uint32_t r = _mm_extract_epi32(res_v, 0);

          #if WRITE_VECTOR == 1
          uint32_t r_lower = r & 0x0F;
          uint32_t r_upper = (r & 0xF0) >> 4;
          __m128i p = _mm_shuffle_epi8(v_a_1_32,shuffle_mask32[r_lower]);
          _mm_storeu_si128((__m128i*)&C[count], p);
          
          //uint32_t *t = (uint32_t*) &p;
          //cout << "Data: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;

          p = _mm_shuffle_epi8(v_a_2_32,shuffle_mask32[r_upper]);
          _mm_storeu_si128((__m128i*)&C[count+_mm_popcnt_u32(r_lower)], p);
          C[count] = A[a_i];
          #endif

          count += _mm_popcnt_u32(r);
          a_i += (a_max <= b_max) * SHORTS_PER_REG;
          a_continue = (a_i+SHORTS_PER_REG) < s_a && (A[a_i+SHORTS_PER_REG-1] & 0xFFFF0000) == prefix;
          i_b += (a_max >= b_max) * SHORTS_PER_REG;
        }
        #endif

        bool notFinished = a_i < s_a  && i_b < b_inner_size && (A[a_i] & 0xFFFF0000) == prefix;
        while(notFinished){
          while(notFinished && (uint32_t)(prefix | B[i_b+b_i]) < A[a_i]){
            ++i_b;
            notFinished = i_b < b_inner_size;
          }
          if(notFinished && A[a_i] == (uint32_t)(prefix | B[i_b+b_i])){
            #if WRITE_VECTOR == 1
            C[count] = A[a_i];
            #endif
            ++count;
          }
          ++a_i;
          notFinished = notFinished && a_i < s_a && (A[a_i] & 0xFFFF0000) == prefix;
        }
        b_i = inner_end;
        not_finished = a_i < s_a && b_i < s_b;
      }
    }
    // XXX: Density computation is broken
    const double density = 0.0;//((count > 0) ? ((double)count/(C[count]-C[0])) : 0.0);

    C_in->cardinality = count;
    C_in->number_of_bytes = count*sizeof(uint32_t);
    C_in->density = density;
    C_in->type= common::UINTEGER;

    return C_in;
  }
    inline size_t intersect_offsets(
    uint32_t *C, 
    size_t *position_data_A, 
    size_t *position_data_B, 
    uint32_t *A, 
    size_t s_a,
    uint32_t *B, 
    size_t s_b){

    size_t count = 0;
    size_t i_a = 0;
    size_t i_b = 0;
    bool notFinished = i_a < s_a  && i_b < s_b;
    while(notFinished){
      while(notFinished && B[i_b] < A[i_a]){
        ++i_b;
        notFinished = i_b < s_b;
      }
      if(notFinished && A[i_a] == B[i_b]){
        C[count] = A[i_a];
        position_data_A[count] = i_a;
        position_data_B[count] = i_b;
        ++count;
      }
      ++i_a;
      notFinished = notFinished && i_a < s_a;
    }
    return count;
  }
  inline Set<uinteger>* set_intersect(Set<uinteger> *C_in,const Set<pshort> *A_in,const Set<uinteger> *B_in){
    return set_intersect(C_in,B_in,A_in);
  }

  inline Set<uinteger>* set_intersect(Set<uinteger> *C_in, const Set<uinteger> *A_in, const Set<uinteger> *B_in) {
    //return set_intersect_standard(C_in, rare, freq);

    const Set<uinteger> *rare = (A_in->cardinality > B_in->cardinality) ? B_in:A_in;
    const Set<uinteger> *freq = (A_in->cardinality > B_in->cardinality) ? A_in:B_in;
    const unsigned long min_size = 1;

    if(std::max(A_in->cardinality,B_in->cardinality) / std::max(min_size, std::min(A_in->cardinality,B_in->cardinality)) > 16)
      return set_intersect_v3(C_in, rare, freq);
    else
      return set_intersect_standard(C_in, rare, freq);
  }

  inline size_t intersect_block(uint64_t *result_data, uint64_t *A, uint64_t *B){
    //BLOCK SIZE HAS TO BE A MULTIPLE OF 256
    size_t count = 0;
    for(size_t i = 0; i < BLOCK_SIZE; i+=256){
      const size_t vector_index = (i/64);
      const __m256 m1 = _mm256_loadu_ps((float*)(A + vector_index));
      const __m256 m2 = _mm256_loadu_ps((float*)(B + vector_index));
      const __m256 r = _mm256_and_ps(m1, m2);
      _mm256_storeu_ps((float*)(result_data+vector_index), r);
      count += _mm_popcnt_u64(result_data[vector_index]);
      count += _mm_popcnt_u64(result_data[vector_index+1]);
      count += _mm_popcnt_u64(result_data[vector_index+2]);
      count += _mm_popcnt_u64(result_data[vector_index+3]);
    }
    return count;
  }
  inline Set<bitset_new>* set_intersect(Set<bitset_new> *C_in,const Set<bitset_new> *A_in,const Set<bitset_new> *B_in){
    if(A_in->number_of_bytes == 0 || B_in->number_of_bytes == 0){
      C_in->cardinality = 0;
      C_in->number_of_bytes = 0;
      C_in->density = 0.0;
      C_in->type= common::UINTEGER;
      return C_in;
    }

    size_t A_num_blocks = A_in->number_of_bytes/(sizeof(uint32_t)+(BLOCK_SIZE/8));
    size_t B_num_blocks = B_in->number_of_bytes/(sizeof(uint32_t)+(BLOCK_SIZE/8));

    size_t A_scratch_space = A_num_blocks*sizeof(size_t);
    size_t B_scratch_space = B_num_blocks*sizeof(size_t);
    size_t scratch_space = A_scratch_space + B_scratch_space;

    //need to move alloc outsize
    size_t *A_offset_positions = (size_t*)C_in->data;
    size_t *B_offset_positions = (size_t*)(C_in->data+A_scratch_space);
    C_in->data += scratch_space;

    uint64_t *A_data = (uint64_t*)(A_in->data+(A_num_blocks*sizeof(uint32_t)));
    uint64_t *B_data = (uint64_t*)(B_in->data+(B_num_blocks*sizeof(uint32_t)));

    uint32_t *A_offset_pointer = (uint32_t*)A_in->data;
    uint32_t *B_offset_pointer = (uint32_t*)B_in->data;

    size_t offset_count = intersect_offsets((uint32_t *)C_in->data,
      A_offset_positions,B_offset_positions,A_offset_pointer,A_num_blocks,B_offset_pointer,B_num_blocks);

    uint64_t *result = (uint64_t*)(C_in->data + sizeof(uint32_t)*offset_count);
    const size_t bytes_per_block = (BLOCK_SIZE/8);
    const size_t words_per_block = bytes_per_block/sizeof(uint64_t);
    size_t count = 0;
    for(size_t i = 0; i < offset_count; i++){
      size_t A_offset = A_offset_positions[i] * words_per_block;
      size_t B_offset = B_offset_positions[i] * words_per_block;
      count += intersect_block(result+(i*words_per_block),A_data+A_offset,B_data+B_offset);
    }

    C_in->cardinality = count;
    C_in->number_of_bytes = offset_count*(sizeof(uint32_t)+bytes_per_block);
    C_in->density = 0.0;
    C_in->type= common::BITSET_NEW;

    return C_in;
  }
  inline size_t hetero_intersect_offsets(
    uint32_t *A_positions, 
    uint32_t *B_position_data, 
    uint32_t *A, 
    size_t s_a,
    uint32_t *B, 
    size_t s_b){

    size_t count = 0;
    size_t i_a = 0;
    size_t i_b = 0;
    bool notFinished = i_a < s_a  && i_b < s_b;
    while(notFinished){
      while(notFinished && B[i_b] < (A[i_a] >> ADDRESS_BITS_PER_BLOCK)){
        ++i_b;
        notFinished = i_b < s_b;
      }
      if(notFinished && (A[i_a] >> ADDRESS_BITS_PER_BLOCK) == B[i_b]){
        A_positions[count] = i_a;
        B_position_data[count] = i_b;
        ++count;
      }
      ++i_a;
      notFinished = notFinished && i_a < s_a;
    }
    return count;
  }
  inline size_t probe_block(
    uint32_t *result,
    uint32_t value,
    uint64_t *block){
    const uint32_t probe_value = value % BLOCK_SIZE;
    const size_t word_to_check = probe_value / BITS_PER_WORD;
    const size_t bit_to_check = probe_value % BITS_PER_WORD;
    if((block[word_to_check] >> bit_to_check) % 2){
      result[0] = value;
      return 1;
    } 
    return 0;
  }

  inline Set<uinteger>* set_intersect(Set<uinteger> *C_in,const Set<uinteger> *A_in,const Set<bitset_new> *B_in){
    if(A_in->number_of_bytes == 0 || B_in->number_of_bytes == 0){
      C_in->cardinality = 0;
      C_in->number_of_bytes = 0;
      C_in->density = 0.0;
      C_in->type= common::UINTEGER;
      return C_in;
    }

    size_t B_num_blocks = B_in->number_of_bytes/(sizeof(uint32_t)+(BLOCK_SIZE/8));
    uint64_t *B_data = (uint64_t*)(B_in->data+(B_num_blocks*sizeof(uint32_t)));
    uint32_t *B_offset_pointer = (uint32_t*)B_in->data;

    uint32_t *A_data = (uint32_t*)A_in->data;

    size_t A_scratch_space = A_in->cardinality*sizeof(uint32_t);
    size_t B_scratch_space = A_in->cardinality*sizeof(uint32_t);
    size_t scratch_space = A_scratch_space + B_scratch_space;

    //need to move alloc outsize
    uint32_t *A_positions = (uint32_t*)C_in->data;
    uint32_t *B_offset_positions = (uint32_t*)(C_in->data+A_scratch_space);
    C_in->data += scratch_space;

    size_t offset_count = hetero_intersect_offsets(
      A_positions,B_offset_positions,A_data,
      A_in->cardinality,B_offset_pointer,B_num_blocks);

    size_t count = 0;
    const size_t bytes_per_block = (BLOCK_SIZE/8);
    const size_t words_per_block = bytes_per_block/sizeof(uint64_t);
    uint32_t *result = (uint32_t*)C_in->data;
    for(size_t i = 0; i < offset_count; i++){
      const size_t B_offset = B_offset_positions[i] * words_per_block;
      count += probe_block(result+count,A_data[A_positions[i]],B_data+B_offset);
    }
  
    C_in->cardinality = count;
    C_in->number_of_bytes = count*sizeof(uint32_t);
    C_in->density = 0.0;
    C_in->type= common::UINTEGER;

    return C_in;
  }
  inline Set<uinteger>* set_intersect(Set<uinteger> *C_in,const Set<bitset_new> *A_in,const Set<uinteger> *B_in){
    return set_intersect(C_in,B_in,A_in);
  }

  inline void distinct_merge_three_way(
    size_t count,
    uint32_t *result, 
    uint32_t *A, size_t lenA, 
    uint32_t *B, size_t lenB,
    uint32_t *C, size_t lenC){

    size_t i_a = 0;
    size_t i_b = 0;
    size_t i_c = 0;
    for(size_t i=0; i < count; i++){
      if(i_a < lenA && i_b < lenB && i_c < lenC){
        if(A[i_a] <= B[i_b] && A[i_a] <= C[i_c]){
          result[i] = A[i_a++];
        } else if(B[i_b] <= A[i_a] && B[i_b] <= C[i_c]){
          result[i] = B[i_b++];
        } else{
          result[i] = C[i_c++];
        }
      } else if(i_b < lenB && i_c < lenC){
        if(B[i_b] <= C[i_c]){
          result[i] = B[i_b++];
        } else{
          result[i] = C[i_c++];
        }
      } else if(i_a < lenA && i_c < lenC){
        if(A[i_a] <= C[i_c]){
          result[i] = A[i_a++];
        } else{
          result[i] = C[i_c++];
        }
      } else if(i_a < lenA && i_b < lenB){
        if(A[i_a] <= B[i_b]){
          result[i] = A[i_a++];
        } else{
          result[i] = B[i_b++];
        }
      } else if(i_a < lenA){
        result[i] = A[i_a++];
      } else if(i_b < lenB){
        result[i] = B[i_b++];
      } else{
        result[i] = C[i_c++];
      }
    }
  }

  inline Set<new_type>* set_intersect(Set<new_type> *C_in,const Set<new_type> *A_in,const Set<new_type> *B_in){
    if(A_in->number_of_bytes == 0 || B_in->number_of_bytes == 0){
      C_in->cardinality = 0;
      C_in->number_of_bytes = 0;
      C_in->density = 0.0;
      C_in->type= common::UINTEGER;
      return C_in;
    }

    const size_t A_num_uint_bytes = ((size_t*)A_in->data)[0];
    uint8_t * A_uinteger_data = A_in->data+sizeof(size_t);
    uint8_t * A_new_bs_data = A_in->data+sizeof(size_t)+A_num_uint_bytes;
    const size_t A_num_bs_bytes = A_in->number_of_bytes-(sizeof(size_t)-A_num_uint_bytes);
    const size_t A_uint_card = A_num_uint_bytes/sizeof(uint32_t);

    const size_t B_num_uint_bytes = ((size_t*)B_in->data)[0];
    uint8_t * B_uinteger_data = B_in->data+sizeof(size_t);
    uint8_t * B_new_bs_data = B_in->data+sizeof(size_t)+B_num_uint_bytes;
    const size_t B_num_bs_bytes = B_in->number_of_bytes-(sizeof(size_t)-B_num_uint_bytes);
    const size_t B_uint_card = B_num_uint_bytes/sizeof(uint32_t);

    //do all three uintegers then merge then intersect the bs
    const size_t scratch1_space = A_num_uint_bytes;
    uint8_t *scratch1 = C_in->data;
    const size_t scratch2_space = A_num_uint_bytes;
    uint8_t *scratch2 = C_in->data+scratch1_space;    
    const size_t scratch3_space = B_num_uint_bytes;
    uint8_t *scratch3 = C_in->data+scratch1_space+scratch2_space; 
    C_in->data += (scratch1_space+scratch2_space+scratch3_space);  

    Set<uinteger>UU(scratch1);
    Set<uinteger>UBS(scratch2);
    Set<uinteger>BSU(scratch3);

    const Set<uinteger>A_I(A_uinteger_data,A_uint_card,A_num_uint_bytes,common::UINTEGER);
    const Set<uinteger>B_I(B_uinteger_data,B_uint_card,B_num_uint_bytes,common::UINTEGER);

    const Set<bitset_new>A_BS(A_new_bs_data,A_in->cardinality-A_uint_card,A_num_bs_bytes,common::BITSET_NEW);
    const Set<bitset_new>B_BS(B_new_bs_data,B_in->cardinality-B_uint_card,B_num_bs_bytes,common::BITSET_NEW);

    size_t count = 0;
    
    UU = ops::set_intersect(&UU,&A_I,&B_I);
    count += UU.cardinality;

    UBS = ops::set_intersect(&UBS,&A_I,&B_BS);
    count += UBS.cardinality;

    BSU = ops::set_intersect(&BSU,&B_I,&A_BS);
    count += BSU.cardinality;

    uint8_t *C_pointer = C_in->data+sizeof(size_t);
    const size_t num_uint = count;
    #if WRITE_VECTOR == 1
    ((size_t*)C_in->data)[0] = (num_uint*sizeof(uint32_t));
    distinct_merge_three_way(
      count,
      (uint32_t*)(C_pointer),
      (uint32_t*)UU.data,UU.cardinality, 
      (uint32_t*)UBS.data,UBS.cardinality,
      (uint32_t*)BSU.data,BSU.cardinality);
    #endif
 
    C_pointer += (num_uint*sizeof(uint32_t));

    Set<bitset_new>BSBS(C_pointer);
    BSBS = ops::set_intersect(&BSBS,&A_BS,&B_BS);
    count += BSBS.cardinality;

    C_in->cardinality = count;
    C_in->number_of_bytes = sizeof(size_t)+(num_uint*sizeof(uint32_t))+BSBS.number_of_bytes;
    C_in->density = 0.0;
    C_in->type= common::NEW_TYPE;

    return C_in;
  }
/*
  inline Set<kunle>* set_intersect(
      Set<kunle> *C_in,
      const Set<kunle> *A_in,
      const Set<kunle> *B_in) {
    size_t count = 0;

    // Words per SIMD register
    const size_t words_per_reg = 4;
    const size_t bits_per_word = 64;

    uint32_t * const C = (uint64_t*)C_in->data;
    const uint64_t * const A = (uint64_t*)A_in->data;
    const uint64_t * const B = (uint64_t*)B_in->data;

    size_t level_pos[num_levels][2];
    size_t level_lens[num_levels][2];
    size_t curr_level = 0;

    uint64_t r_vals[num_levels][words_per_reg];
    uint64_t m1_skip_vals[num_levels][words_per_reg];
    uint64_t m2_skip_vals[num_levels][words_per_reg];

    while(true) {
      const __m256 m1 = _mm256_loadu_ps(A + level_pos[curr_level][0]);
      const __m256 m2 = _mm256_loadu_ps(B + level_pos[curr_level][1]);
      const __m256 r = _mm256_and_ps(m1, m2);

      _mm256_storeu_ps((float*)r_vals[curr_level], r);

      if(curr_level == num_levels - 1) {
        // If we are at the leaf level, count the number of bits in r and
        // move up a level
        count += _mm_popcnt_u64(r_vals[curr_level][0]);
        count += _mm_popcnt_u64(r_vals[curr_level][1]);
        count += _mm_popcnt_u64(r_vals[curr_level][2]);
        count += _mm_popcnt_u64(r_vals[curr_level][3]);
        curr_level--;
      }
      else {
        // Skip bins in m1 that are not in m2
        const __m256 m1_skip = _mm256_andnot_ps(m1, m2);
        // Skip bins in m2 that are not in m1
        const __m256 m2_skip = _mm256_andnot_ps(m2, m1);

        _mm256_storeu_ps((float*)m1_skip_vals[curr_level], m1_skip);
        _mm256_storeu_ps((float*)m2_skip_vals[curr_level], m2_skip);

        for(size_t word = 0; word < words_per_reg; word++) {
          for(size_t i = 0; i < bits_per_word; i++) {
            uint64_t r_bit = r_vals[curr_level] >> i;
            uint64_t m1_skip_bit = m1_skip_vals[curr_level] >> i;
            uint64_t m2_skip_bit = m2_skip_vals[curr_level] >> i;

            if(r_bit) {
              // Potential hit, check the lower levels
              curr_level++;
            }
            else if(m1_skip_bit) {
              // Skip subtree that corresponds to the m1_skip_bit
              level_pos[curr_level + 1][0]++;
            }
            else if(m2_skip_bit) {
              // Skip subtree that corresponds to the m2_skip_bit
              level_pos[curr_level + 1][1]++;
            }
          }
        }
      }
    }

    return count;
  }
  */
  /*
  inline Set<uinteger>* set_intersect(Set<uinteger> *C_in,const Set<uinteger> *A_in,const Set<pshort> *B_in){
    uint32_t * const C = (uint32_t*)C_in->data;
    const uint32_t * const A = (uint32_t*)A_in->data;
    const uint16_t * const B = (uint16_t*)B_in->data;
    const size_t s_a = A_in->cardinality;
    const size_t s_b = B_in->number_of_bytes/sizeof(uint16_t);
    const size_t st_a = (s_a/SHORTS_PER_REG)*SHORTS_PER_REG;

    #if WRITE_VECTOR == 0
    (void)C;
    #endif

    size_t a_i = 0;
    size_t b_i = 0;
    size_t count = 0;

    bool not_finished = a_i < s_a && b_i < s_b;
    while(not_finished){
      uint32_t prefix = (B[b_i] << 16);
      uint16_t b_inner_size = B[b_i+1]+1;
      uint32_t cur_match = A[a_i];
      size_t inner_end = b_i+b_inner_size+2;
      //cout << endl;
      //cout << "Bi: " << b_i << " Bsize: " << s_b << " InnerEnd: " << inner_end << endl;

      if(prefix < (cur_match & 0xFFFF0000)){
        //cout << "1" << endl;
        b_i = inner_end;
        not_finished = b_i < s_b;
      } else if(prefix > cur_match){
        //cout << prefix << " " << cur_match << endl;
        //cout << "2" << endl;
        a_i++;
        not_finished = a_i < s_a;
      } else {
        //cout << "3" << endl;
        b_i += 2;
        size_t i_b = 0;

        bool innerNotFinished = true;
        #if VECTORIZE == 1
        const size_t st_b = (b_inner_size / SHORTS_PER_REG) * SHORTS_PER_REG;
        while((a_i+SHORTS_PER_REG) < st_a && i_b < st_b && (A[a_i] & 0xFFFF0000) <= prefix) {
          __m128i v_a_1_32 = _mm_loadu_si128((__m128i*)&A[a_i]);
          __m128i v_a_2_32 = _mm_loadu_si128((__m128i*)&A[a_i+(SHORTS_PER_REG/2)]);

          __m128i v_a_1 = _mm_shuffle_epi8(v_a_1_32,_mm_set_epi8(uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0)));
          __m128i v_a_2 = _mm_shuffle_epi8(v_a_2_32,_mm_set_epi8(uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80)));
          
          __m128i v_a = _mm_or_si128(v_a_1,v_a_2);
            
          //uint16_t *t = (uint16_t*) &v_a;
          //cout << "Data: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << " " << t[4] << " " << t[5] << " " << t[6] << " " << t[7] << endl;

          __m128i v_b = _mm_loadu_si128((__m128i*)&B[b_i+i_b]);    

          const uint32_t a_max = A[a_i+SHORTS_PER_REG-1];
          const uint32_t b_max = prefix | (uint32_t)B[b_i+i_b+SHORTS_PER_REG-1];
          
          __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
                  _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
          
          const uint32_t r = _mm_extract_epi32(res_v, 0);
          if(_mm_popcnt_u32(r)){
            const size_t a_i_end = a_i+8;
            const size_t i_b_end = i_b+8;
            size_t a_i_tmp = a_i;
            size_t i_b_tmp = i_b;
            innerNotFinished = a_i_tmp < a_i_end  && i_b_tmp < i_b_end;
            while(innerNotFinished){
              while(innerNotFinished && A[a_i_tmp] < (uint32_t)(prefix | B[i_b_tmp+b_i]) ){
                ++a_i_tmp;
                innerNotFinished = a_i_tmp < a_i_end;
                if(innerNotFinished && (A[a_i_tmp] & 0xFFFF0000) > prefix){
                  a_i += (a_max <= b_max) * SHORTS_PER_REG;
                  i_b += (a_max >= b_max) * SHORTS_PER_REG;
                  goto INNER_PARTITION_DONE;
                }
              }
              if(innerNotFinished && A[a_i_tmp] == (uint32_t)(prefix | B[i_b_tmp+b_i])){
                //cout << A[a_i_tmp]<< endl;
                #if WRITE_VECTOR == 1
                C[count] = A[a_i_tmp];
                #endif
                ++count;
              }
              ++i_b_tmp;
              innerNotFinished = innerNotFinished && i_b_tmp < i_b_end;
            }
          }
          a_i += (a_max <= b_max) * SHORTS_PER_REG;
          i_b += (a_max >= b_max) * SHORTS_PER_REG;
        }
        #endif

        //cout << "1a_i: " << a_i << " i_b: " << i_b << " " << inner_end << endl;


        innerNotFinished = a_i < s_a  && i_b < b_inner_size && (A[a_i] & 0xFFFF0000) <= prefix;
        while(innerNotFinished){
          while(innerNotFinished && A[a_i] < (uint32_t)(prefix | B[i_b+b_i])){
            ++a_i;
            innerNotFinished = a_i < s_a && (A[a_i] & 0xFFFF0000) <= prefix;
          }
          if(innerNotFinished && A[a_i] == (uint32_t)(prefix | B[i_b+b_i])){
            //cout << A[a_i]<< endl;
            #if WRITE_VECTOR == 1
            C[count] = A[a_i];
            #endif
            ++count;
          }
          ++i_b;
          innerNotFinished = innerNotFinished && i_b < b_inner_size;
        }

        INNER_PARTITION_DONE:
        b_i = inner_end;

        //cout << "a_i: " << a_i << " b_i: " << b_i << endl;

        not_finished = a_i < s_a && b_i < s_b;
      }
    }

    // XXX: Density computation is broken
    const double density = 0.0;//((count > 0) ? ((double)count/(C[count]-C[0])) : 0.0);

    C_in->cardinality = count;
    C_in->number_of_bytes = count*sizeof(uint32_t);
    C_in->density = density;
    C_in->type= common::UINTEGER;

    return C_in;
  }
  inline Set<uinteger>* set_intersect(Set<uinteger> *C_in,const Set<uinteger> *A_in,const Set<pshort> *B_in){
    uint32_t * const C = (uint32_t*)C_in->data;
    const uint32_t * const A = (uint32_t*)A_in->data;
    const uint16_t * const B = (uint16_t*)B_in->data;
    const size_t s_a = A_in->cardinality;
    const size_t s_b = B_in->number_of_bytes/sizeof(uint16_t);
    const size_t st_a = (s_a/SHORTS_PER_REG)*SHORTS_PER_REG;

    (void) C;
    #if WRITE_VECTOR == 0
    (void)C;
    #endif

    size_t i_a = 0;
    size_t i_b = 0;
    size_t count = 0;

    while(i_a < s_a && i_b < s_b){
      uint32_t prefix = ((uint32_t)B[i_b] << 16);
      uint16_t partition_size = B[i_b+1]+1;

      //First make sure we are looking at similar partitions
      while(A[i_a] < prefix){
        i_a++;
        if(i_a >= s_a) goto INTERSECTION_FINISHED;
      }
      while(prefix < (A[i_a] & 0xFFFF0000) ){
        i_b += 2+partition_size;
        if(i_b >= s_b) goto INTERSECTION_FINISHED;
        prefix = ((uint32_t)B[i_b] << 16);
        partition_size = B[i_b+1]+1;
      }

      i_b += 2;
      size_t inner_b_index = 0;
      const size_t st_b = (partition_size/SHORTS_PER_REG)*SHORTS_PER_REG;
      while((i_a+SHORTS_PER_REG) < st_a && (inner_b_index+SHORTS_PER_REG) < st_b){
        
        uint32_t B_val = prefix | B[i_b + inner_b_index];
        size_t max = partition_size - inner_b_index;
        if((B_val+GALLOP_SIZE) < A[i_a] && GALLOP_SIZE < max){
          size_t min = 0;
          while(max > min){
            const size_t mid = min + (max-min)/2;
            B_val = prefix | B[i_b+inner_b_index+mid];
 
            if(B_val == A[i_a]){
              inner_b_index += mid;
              goto VECTORIZE_PARTITION;
            } else if(B_val < A[i_a]){
              min = mid+1;
            } else{
              max = (mid > 0) ? mid-1:0;
            }
          }
          inner_b_index += min;
        }

        VECTORIZE_PARTITION:
        const size_t a_end = i_a + SHORTS_PER_REG;
        const uint32_t a_max = A[i_a+SHORTS_PER_REG-1];

        __m128i v_a_1_32 = _mm_loadu_si128((__m128i*)&A[i_a]);
        __m128i v_a_2_32 = _mm_loadu_si128((__m128i*)&A[i_a+(SHORTS_PER_REG/2)]);

        __m128i v_a_1 = _mm_shuffle_epi8(v_a_1_32,_mm_set_epi8(uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0)));
        __m128i v_a_2 = _mm_shuffle_epi8(v_a_2_32,_mm_set_epi8(uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80)));
        
        __m128i v_a = _mm_or_si128(v_a_1,v_a_2);
        __m128i v_b = _mm_loadu_si128((__m128i*)&B[i_b+inner_b_index]);  

       __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
                _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
        
        const uint32_t r = _mm_extract_epi32(res_v, 0);
        if(_mm_popcnt_u32(r)){
          for(size_t i = 0; i < SHORTS_PER_REG; i++){
            while((prefix | B[i_b+inner_b_index+i]) > A[i_a]){
              i_a++;
              if(i_a >= s_a) goto INTERSECTION_FINISHED;
              if((A[i_a] & 0xFFFF0000) > prefix) goto FINISH_PARTITION;
            }
            if((prefix | B[i_b+inner_b_index+i]) == A[i_a]){
              #if WRITE_VECTOR == 1
              C[count] = A[i_a];
              #endif
              count++;
              i_a++;
              if(i_a >= s_a) goto INTERSECTION_FINISHED;
              if((A[i_a] & 0xFFFF0000) > prefix) goto FINISH_PARTITION;
            }
          }
        }
        if(a_max > (prefix | B[i_b+inner_b_index+SHORTS_PER_REG-1])){
          inner_b_index += SHORTS_PER_REG;
        } else{
          i_a = std::max(a_end,i_a);
          if((A[i_a] & 0xFFFF0000) > prefix) goto FINISH_PARTITION;
        }
      }
    
      while(inner_b_index < partition_size && i_a < s_a){
        uint32_t B_val = prefix | B[i_b + inner_b_index];
        size_t max = partition_size - inner_b_index;
        if((B_val+GALLOP_SIZE) < A[i_a] && GALLOP_SIZE < max){
          size_t min = 0;
          while(max > min){
            const size_t mid = min + (max-min)/2;
            B_val = prefix | B[i_b+inner_b_index+mid];
 
            if(B_val == A[i_a]){
              inner_b_index += mid;
              goto SCALAR_PARTITION;
            } else if(B_val < A[i_a]){
              min = mid+1;
            } else{
              max = (mid > 0) ? mid-1:0;
            }
          }
          inner_b_index += min;
        }

        SCALAR_PARTITION:
        B_val = prefix | B[i_b+inner_b_index];
        while(A[i_a] < B_val){
          i_a++;
          if(i_a >= s_a) goto INTERSECTION_FINISHED;
        }
        if(B_val == A[i_a]){
          #if WRITE_VECTOR == 1
          C[count] = A[i_a];
          #endif
          count++;
          i_a++;
        }
        inner_b_index++;
      }
      FINISH_PARTITION:
      i_b += partition_size;
    }

    INTERSECTION_FINISHED:

    // XXX: Density computation is broken
    const double density = 0.0;

    C_in->cardinality = count;
    C_in->number_of_bytes = count*sizeof(uint32_t);
    C_in->density = density;
    C_in->type = common::UINTEGER;

    return C_in;
  }
  */

  inline Set<hybrid>* set_intersect(Set<hybrid> *C_in,const Set<hybrid> *A_in,const Set<hybrid> *B_in){
    if(A_in->cardinality == 0 || B_in->cardinality == 0){
      C_in->cardinality = 0;
      C_in->number_of_bytes = 0;
      return C_in;
    }

    switch (A_in->type) {
        case common::UINTEGER:
          switch (B_in->type) {
            case common::UINTEGER:
              #ifdef STATS
              common::num_uint_uint++;
              #endif
              return (Set<hybrid>*)set_intersect((Set<uinteger>*)C_in,(const Set<uinteger>*)A_in,(const Set<uinteger>*)B_in);
              break;
            case common::PSHORT:
              #ifdef STATS
              common::num_uint_pshort++;
              #endif
              return (Set<hybrid>*)set_intersect((Set<uinteger>*)C_in,(const Set<uinteger>*)A_in,(const Set<pshort>*)B_in);
              break;
            case common::BITSET:
              #ifdef STATS
              common::num_pshort_bs++;
              #endif
              return (Set<hybrid>*)set_intersect((Set<uinteger>*)C_in,(const Set<uinteger>*)A_in,(const Set<bitset>*)B_in);
              break;
            default:
              break;
          }
        break;
        case common::PSHORT:
          switch (B_in->type) {
            case common::UINTEGER:
              #ifdef STATS
              common::num_uint_pshort++;
              #endif
              return (Set<hybrid>*)set_intersect((Set<uinteger>*)C_in,(const Set<uinteger>*)B_in,(const Set<pshort>*)A_in);
            break;
            case common::PSHORT:
              #ifdef STATS
              common::num_pshort_pshort++;
              #endif
              return (Set<hybrid>*)set_intersect((Set<pshort>*)C_in,(const Set<pshort>*)A_in,(const Set<pshort>*)B_in);
            break;
            case common::BITSET:
              #ifdef STATS
              common::num_pshort_bs++;
              #endif
              return (Set<hybrid>*)set_intersect((Set<pshort>*)C_in,(const Set<pshort>*)A_in,(const Set<bitset>*)B_in);
            break;
            default:
            break;
          }
        break;
        case common::BITSET:
          switch (B_in->type) {
            case common::UINTEGER:
              #ifdef STATS
              common::num_uint_bs++;
              #endif
              return (Set<hybrid>*)set_intersect((Set<uinteger>*)C_in,(const Set<uinteger>*)B_in,(const Set<bitset>*)A_in);
            break;
            case common::PSHORT:
              #ifdef STATS
              common::num_pshort_bs++;
              #endif
              return (Set<hybrid>*)set_intersect((Set<pshort>*)C_in,(const Set<pshort>*)B_in,(const Set<bitset>*)A_in);
            break;
            case common::BITSET:
              #ifdef STATS
              common::num_bs_bs++;
              #endif
              return (Set<hybrid>*)set_intersect((Set<bitset>*)C_in,(const Set<bitset>*)A_in,(const Set<bitset>*)B_in);
            break;
            default:
            break;
          }
        break;
        default:
        break;
    }

    cout << "ERROR" << endl;
    return C_in;
  }
}

#endif
