/*
 * @Author: DI JUNKUN
 * @Date: 2025-01-08
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _ENC_MARK_H_
#define _ENC_MARK_H_

// L4S Explicit Congestion Notification (ECN) .
// https://www.rfc-editor.org/rfc/rfc9331.html ECT stands for ECN-Capable
// Transport and CE stands for Congestion Experienced.

// RFC-3168, Section 5
// +-----+-----+
// | ECN FIELD |
// +-----+-----+
//   ECT   CE         [Obsolete] RFC 2481 names for the ECN bits.
//    0     0         Not-ECT
//    0     1         ECT(1)
//    1     0         ECT(0)
//    1     1         CE

enum class EcnMarking {
  kNotEct = 0,  // Not ECN-Capable Transport
  kEct1 = 1,    // ECN-Capable Transport
  kEct0 = 2,    // Not used by L4s (or webrtc.)
  kCe = 3,      // Congestion experienced
};

#endif