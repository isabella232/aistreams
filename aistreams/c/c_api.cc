// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "aistreams/c/c_api.h"

#include "aistreams/base/wrappers/senders.h"
#include "aistreams/c/ais_packet_internal.h"
#include "aistreams/c/ais_status_internal.h"
#include "aistreams/c/c_api_internal.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

using aistreams::ConnectionOptions;
using aistreams::OkStatus;
using aistreams::Packet;
using aistreams::PacketReceiver;
using aistreams::PacketSender;
using aistreams::SenderOptions;
using aistreams::Status;
using aistreams::StatusOr;

extern "C" {

// --------------------------------------------------------------------------

AIS_ConnectionOptions* AIS_NewConnectionOptions(void) {
  return new AIS_ConnectionOptions;
}

void AIS_DeleteConnectionOptions(AIS_ConnectionOptions* ais_options) {
  delete ais_options;
}

// Set the target address (ip:port) to the server.
void AIS_SetTargetAddress(const char* target_address,
                          AIS_ConnectionOptions* ais_options) {
  ais_options->connection_options.target_address = target_address;
}

// Use an insecure connection to the server.
void AIS_SetUseInsecureChannel(unsigned char use_insecure_channel,
                               AIS_ConnectionOptions* ais_options) {
  ais_options->connection_options.ssl_options.use_insecure_channel =
      use_insecure_channel;
}

// Set the expected SSL domain name of the server.
//
// You are required to supply this if you do not use an insecure channel.
void AIS_SetSslDomainName(const char* ssl_domain_name,
                          AIS_ConnectionOptions* ais_options) {
  ais_options->connection_options.ssl_options.ssl_domain_name = ssl_domain_name;
}

// Set the path to the root CA certificate.
//
// You are required to supply this if you do not use an insecure channel.
void AIS_SetSslRootCertPath(const char* ssl_root_cert_path,
                            AIS_ConnectionOptions* ais_options) {
  ais_options->connection_options.ssl_options.ssl_root_cert_path =
      ssl_root_cert_path;
}

// --------------------------------------------------------------------------

AIS_Sender* AIS_NewSender(const AIS_ConnectionOptions* options,
                          const char* stream_name, AIS_Status* ais_status) {
  SenderOptions sender_options;
  sender_options.connection_options = options->connection_options;
  sender_options.stream_name = stream_name;

  std::unique_ptr<PacketSender> sender;
  auto status = MakePacketSender(sender_options, &sender);
  if (!status.ok()) {
    ais_status->status = status;
    return nullptr;
  }

  auto ais_sender = std::make_unique<AIS_Sender>();
  ais_sender->packet_sender = std::move(sender);
  ais_status->status = OkStatus();
  return ais_sender.release();
}

void AIS_DeleteSender(AIS_Sender* ais_sender) { delete ais_sender; }

void AIS_SendPacket(AIS_Sender* ais_sender, AIS_Packet* ais_packet,
                    AIS_Status* ais_status) {
  ais_status->status = ais_sender->packet_sender->Send(ais_packet->packet);
  return;
}

// --------------------------------------------------------------------------

AIS_Receiver* AIS_NewReceiver(const AIS_ConnectionOptions* options,
                              const char* stream_name, AIS_Status* ais_status) {
  PacketReceiver::Options packet_receiver_options;
  packet_receiver_options.connection_options = options->connection_options;
  packet_receiver_options.stream_name = stream_name;
  auto packet_receiver_statusor =
      PacketReceiver::Create(packet_receiver_options);
  if (!packet_receiver_statusor.ok()) {
    ais_status->status = packet_receiver_statusor.status();
    return nullptr;
  }

  auto ais_receiver = std::make_unique<AIS_Receiver>();
  ais_receiver->packet_receiver =
      std::move(packet_receiver_statusor).ValueOrDie();
  ais_status->status = OkStatus();
  return ais_receiver.release();
}

void AIS_DeleteReceiver(AIS_Receiver* ais_receiver) { delete ais_receiver; }

void AIS_ReceivePacket(AIS_Receiver* ais_receiver, AIS_Packet* ais_packet,
                       AIS_Status* ais_status) {
  ais_status->status =
      ais_receiver->packet_receiver->Receive(&ais_packet->packet);
  return;
}

}  // end extern "C"
