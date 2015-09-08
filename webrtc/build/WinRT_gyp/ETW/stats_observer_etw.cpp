
// Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#include <string>

#include "webrtc/build/WinRT_gyp/ETW/stats_observer_etw.h"
#include "webrtc/base/thread.h"

// generated by message compiler
#include "webrtc/build/WinRT_gyp/ETW/etw_providers.h"

namespace webrtc {

// The time interval of requesting statistics
const int StatsObserverETW::kInterval = 1000;
enum {
  MSG_POLL_STATS,
};

StatsObserverETW::StatsObserverETW() : pci_(NULL) {
  EventRegisterWebRTCInternals();
}

StatsObserverETW::~StatsObserverETW() {
  EventUnregisterWebRTCInternals();
}

void StatsObserverETW::OnComplete(const StatsReports& reports) {
  for (auto report : reports) {
    std::string sgn = report->id()->ToString();
    auto stat_group_name = sgn.c_str();
    auto timestamp = report->timestamp();

    for (auto value : report->values()) {
      auto stat_name = value.second->display_name();

      switch (value.second->type()) {
        case StatsReport::Value::kInt:
        EventWriteStatsReportInt32(stat_group_name, timestamp, stat_name,
          value.second->int_val());
        break;
      case StatsReport::Value::kInt64:
        EventWriteStatsReportInt64(stat_group_name, timestamp, stat_name,
          value.second->int64_val());
        break;
      case StatsReport::Value::kFloat:
        EventWriteStatsReportFloat(stat_group_name, timestamp, stat_name,
          value.second->float_val());
        break;
      case StatsReport::Value::kBool:
        EventWriteStatsReportBool(stat_group_name, timestamp, stat_name,
          value.second->bool_val());
        break;
      case StatsReport::Value::kStaticString:
        EventWriteStatsReportString(stat_group_name, timestamp, stat_name,
          value.second->static_string_val());
        break;
      case StatsReport::Value::kString:
        EventWriteStatsReportString(stat_group_name, timestamp, stat_name,
          value.second->string_val().c_str());
        break;
      default:
        break;
      }
    }
  }
}

void StatsObserverETW::OnMessage(rtc::Message* msg) {
  switch (msg->message_id) {
  case MSG_POLL_STATS:
    PollStats(pci_);
    break;
  default:
    break;
  }
}

void StatsObserverETW::PollStats(
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> pci) {
  if (!pci.get()) {
    return;
  }

  pci_ = pci;

  auto lss = pci_->local_streams();
  GetStreamCollectionStats(lss);

  rtc::Thread::Current()->PostDelayed(kInterval, this, MSG_POLL_STATS);
}

void StatsObserverETW::GetStreamCollectionStats(
  rtc::scoped_refptr<StreamCollectionInterface> streams) {
  auto ss_count = streams->count();

  for (size_t i = 0; i < ss_count; ++i) {
    auto audio_tracks = streams->at(i)->GetAudioTracks();
    auto video_tracks = streams->at(i)->GetVideoTracks();

    for (auto audio_track : audio_tracks) {
      pci_->GetStats(this, audio_track,
        PeerConnectionInterface::kStatsOutputLevelDebug);
    }

    for (auto video_track : video_tracks) {
      pci_->GetStats(this, video_track,
        PeerConnectionInterface::kStatsOutputLevelDebug);
    }
  }
}

}  //  namespace webrtc
