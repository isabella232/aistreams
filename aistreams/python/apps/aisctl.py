# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""AI Streams CLI."""

import argparse
import logging
import os
import pathlib
import shlex
import sys

import aistreams.gstreamer as gst

_MANAGEMENT_APP_NAME = "manager_app"
_INGESTION_APP_NAME = "ingester_app"
_LOGGING_FORMAT = "%(levelname)s: %(message)s"


def _set_environment_variables():
  gst_plugin_path = os.path.join(
      pathlib.Path(gst.__file__).parents[0], "gst-plugins")
  logging.debug("Setting GST_PLUGIN_PATH to \"%s\"", gst_plugin_path)
  os.environ["GST_PLUGIN_PATH"] = gst_plugin_path
  os.environ["GLOG_alsologtostderr"] = "1"


def _to_cpp_bool_string(b):
  return "true" if b else "false"


def _exec_manager_app(op_id, args):
  """Executes the manager app with the given op id and commandline args."""
  app_path = os.path.join(
      pathlib.Path(__file__).parents[0], _MANAGEMENT_APP_NAME)
  manager_app_config = {
      "app_path":
          app_path,
      "op_id":
          op_id,
      "ssl_root_cert_path":
          args.ssl_root_cert_path,
      "stream_name":
          args.stream_name,
      "target_address":
          args.target_address,
      "use_google_managed_service":
          _to_cpp_bool_string(args.use_google_managed_service),
      "use_insecure_channel":
          _to_cpp_bool_string(args.use_insecure_channel),
  }
  manager_app_cmd = (
      "{app_path} "
      "--op_id={op_id} "
      "--ssl_root_cert_path={ssl_root_cert_path} "
      "--stream_name={stream_name} "
      "--target_address={target_address} "
      "--use_google_managed_service={use_google_managed_service} "
      "--use_insecure_channel={use_insecure_channel} ".format(
          **manager_app_config))
  logging.debug("Executing command %s.", manager_app_cmd)

  manager_app_tokens = shlex.split(manager_app_cmd)
  os.execlp(manager_app_tokens[0], *manager_app_tokens)


def create_stream(args):
  """Create a stream on the server."""
  return _exec_manager_app(0, args)


def list_streams(args):
  """List all the streams on the server."""
  return _exec_manager_app(1, args)


def delete_stream(args):
  """Delete a stream on the server."""
  return _exec_manager_app(2, args)


def ingest(args):
  """Ingest an input source uri."""
  app_path = os.path.join(
      pathlib.Path(__file__).parents[0], _INGESTION_APP_NAME)
  ingester_app_config = {
      "app_path": app_path,
      "target_address": args.target_address,
      "stream_name": args.stream_name,
      "loop_playback": args.loop_playback,
      "source_uri": args.source_uri,
  }
  ingester_app_cmd = ("{app_path} "
                      "--target_address={target_address} "
                      "--stream_name={stream_name} "
                      "--loop_playback={loop_playback} "
                      "--source_uri={source_uri} ".format(
                          **ingester_app_config))
  logging.debug("Executing command %s.", ingester_app_cmd)

  ingester_app_tokens = shlex.split(ingester_app_cmd)
  os.execlp(ingester_app_tokens[0], *ingester_app_tokens)


def main():
  parser = argparse.ArgumentParser(description="AI Streams CLI.")
  subparsers = parser.add_subparsers()

  # Top level options.
  parser.add_argument(
      "-v", "--verbose", action="store_true", help="Verbose output.")

  parser.add_argument(
      "-t",
      "--target-address",
      type=str,
      required=True,
      help="Address (ip:port) to the service endpoint.")

  parser.add_argument(
      "--use-google-managed-service",
      action="store_true",
      help="Use the Google managed service.")

  parser.add_argument(
      "--use-insecure-channel",
      action="store_true",
      help="Use an insecure connection.")

  parser.add_argument(
      "--ssl-domain-name",
      type=str,
      default="aistreams.io",
      help="The expected ssl domain name of the server.")

  parser.add_argument(
      "--ssl-root-cert-path",
      type=str,
      default=None,
      help="The path to the SSL root certificate.")

  # Options for "create" .
  subparser_create = subparsers.add_parser(
      "create", help="Create a stream on the server.")
  subparser_create.add_argument(
      "-s",
      "--stream-name",
      type=str,
      required=True,
      help="The name of the stream to create.")
  subparser_create.set_defaults(func=create_stream)

  # Options for "delete" .
  subparser_delete = subparsers.add_parser(
      "delete", help="Delete a stream on the server.")
  subparser_delete.add_argument(
      "-s",
      "--stream-name",
      type=str,
      required=True,
      help="The name of the stream to delete.")
  subparser_delete.set_defaults(func=delete_stream)

  # Options for "list_streams" .
  subparser_list_streams = subparsers.add_parser(
      "list_streams", help="List all created streams.")
  subparser_list_streams.set_defaults(func=list_streams)

  # Options for "ingest" .
  subparser_ingest = subparsers.add_parser(
      "ingest", help="Send jpeg packets to a stream.")
  subparser_ingest.add_argument(
      "-s",
      "--stream-name",
      type=str,
      required=True,
      help="The name of the stream to ingest packets to.")
  subparser_ingest.add_argument(
      "-u",
      "--source-uri",
      type=str,
      required=True,
      help="The input source's uri; e.g. something.mp4, rtsp://some.thing")
  subparser_ingest.add_argument(
      "-l",
      "--loop-playback",
      action="store_true",
      help="Replay the source if it ends.")
  subparser_ingest.set_defaults(func=ingest)
  parsed_args = parser.parse_args()

  # Set verbosity.
  if parsed_args.verbose:
    logging.basicConfig(format=_LOGGING_FORMAT, level=logging.DEBUG)
  else:
    logging.basicConfig(format=_LOGGING_FORMAT, level=logging.INFO)

  # Set environment variables for this and subprocesses.
  _set_environment_variables()

  parsed_args.func(parsed_args)


if __name__ == "__main__":
  sys.exit(main())
