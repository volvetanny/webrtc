/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/base/basictypes.h"

#include "webrtc/base/gunit.h"

namespace rtc {

static_assert(sizeof(int8) == 1, "Unexpected size");
static_assert(sizeof(uint8) == 1, "Unexpected size");
static_assert(sizeof(int16) == 2, "Unexpected size");
static_assert(sizeof(uint16) == 2, "Unexpected size");
static_assert(sizeof(int32) == 4, "Unexpected size");
static_assert(sizeof(uint32) == 4, "Unexpected size");
static_assert(sizeof(int64) == 8, "Unexpected size");
static_assert(sizeof(uint64) == 8, "Unexpected size");

TEST(BasicTypesTest, Endian) {
  uint16 v16 = 0x1234u;
  uint8 first_byte = *reinterpret_cast<uint8*>(&v16);
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(0x34u, first_byte);
#elif defined(ARCH_CPU_BIG_ENDIAN)
  EXPECT_EQ(0x12u, first_byte);
#endif
}

TEST(BasicTypesTest, SizeOfTypes) {
  int8 i8 = -1;
  uint8 u8 = 1u;
  int16 i16 = -1;
  uint16 u16 = 1u;
  int32 i32 = -1;
  uint32 u32 = 1u;
  int64 i64 = -1;
  uint64 u64 = 1u;
  EXPECT_EQ(1u, sizeof(i8));
  EXPECT_EQ(1u, sizeof(u8));
  EXPECT_EQ(2u, sizeof(i16));
  EXPECT_EQ(2u, sizeof(u16));
  EXPECT_EQ(4u, sizeof(i32));
  EXPECT_EQ(4u, sizeof(u32));
  EXPECT_EQ(8u, sizeof(i64));
  EXPECT_EQ(8u, sizeof(u64));
  EXPECT_GT(0, i8);
  EXPECT_LT(0u, u8);
  EXPECT_GT(0, i16);
  EXPECT_LT(0u, u16);
  EXPECT_GT(0, i32);
  EXPECT_LT(0u, u32);
  EXPECT_GT(0, i64);
  EXPECT_LT(0u, u64);
}

TEST(BasicTypesTest, SizeOfConstants) {
  EXPECT_EQ(8u, sizeof(INT64_C(0)));
  EXPECT_EQ(8u, sizeof(UINT64_C(0)));
  EXPECT_EQ(8u, sizeof(INT64_C(0x1234567887654321)));
  EXPECT_EQ(8u, sizeof(UINT64_C(0x8765432112345678)));
}

// Test CPU_ macros
#if !defined(CPU_ARM) && (defined(__arm__) || defined(_M_ARM))
#error expected CPU_ARM to be defined.
#endif
#if !defined(_M_ARM) && !defined(CPU_X86) && (defined(WEBRTC_WIN) \
    || defined(WEBRTC_MAC) && !defined(WEBRTC_IOS))
#error expected CPU_X86 to be defined.
#endif
#if !defined(ARCH_CPU_LITTLE_ENDIAN) && \
    (defined(WEBRTC_WIN) || defined(WEBRTC_MAC) && \
    !defined(WEBRTC_IOS) || defined(CPU_X86))
#error expected ARCH_CPU_LITTLE_ENDIAN to be defined.
#endif

// TODO(fbarchard): Test all macros in basictypes.h

}  // namespace rtc
