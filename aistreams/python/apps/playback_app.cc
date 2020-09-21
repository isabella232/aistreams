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

#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "absl/time/time.h"
#include "aistreams/base/connection_options.h"
#include "aistreams/gstreamer/gst-plugins/cli_builders/aissrc_cli_builder.h"
#include "aistreams/gstreamer/gstreamer_utils.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

ABSL_FLAG(std::string, target_address, "localhost:50052",
          "Address (ip:port) to the AI Streams instance.");
ABSL_FLAG(std::string, stream_name, "", "Name of the stream to play from.");
ABSL_FLAG(bool, use_insecure_channel, true, "Use an insecure channel.");
ABSL_FLAG(std::string, ssl_domain_name, "aistreams.googleapis.com",
          "The expected ssl domain name of the service.");
ABSL_FLAG(std::string, ssl_root_cert_path, "",
          "The path to the ssl root certificate.");

namespace aistreams {

Status RunPlayback() {
  std::vector<std::string> gst_pipeline;

  // Configure aissrc.
  std::string target_address = absl::GetFlag(FLAGS_target_address);
  std::string stream_name = absl::GetFlag(FLAGS_stream_name);
  SslOptions ssl_options;
  ssl_options.use_insecure_channel = absl::GetFlag(FLAGS_use_insecure_channel);
  ssl_options.ssl_domain_name = absl::GetFlag(FLAGS_ssl_domain_name);
  ssl_options.ssl_root_cert_path = absl::GetFlag(FLAGS_ssl_root_cert_path);
  AissrcCliBuilder aissrc_cli_builder;
  auto aissrc_plugin_statusor =
      aissrc_cli_builder.SetTargetAddress(target_address)
          .SetStreamName(stream_name)
          .SetSslOptions(ssl_options)
          .Finalize();
  if (!aissrc_plugin_statusor.ok()) {
    LOG(ERROR) << aissrc_plugin_statusor.status();
    return InvalidArgumentError(
        "Could not get a valid configuration for aissrc");
  }
  gst_pipeline.push_back(aissrc_plugin_statusor.ValueOrDie());

  // Configure the visualization pipeline.
  gst_pipeline.push_back("decodebin");
  gst_pipeline.push_back("videoconvert");
  gst_pipeline.push_back("autovideosink");

  // Run the gstreamer pipeline.
  auto gstlaunch_command = absl::StrJoin(gst_pipeline, " ! ");
  LOG(INFO) << absl::StrFormat("Running the gstreamer pipeline %s",
                               gstlaunch_command);
  auto status = GstLaunchPipeline(gstlaunch_command);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("Gstreamer launch did not complete successfully");
  }
  return OkStatus();
}

}  // namespace aistreams

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  auto status = aistreams::RunPlayback();
  if (!status.ok()) {
    LOG(ERROR) << status;
    return EXIT_FAILURE;
  }
  return 0;
}
