/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * SECTION: element-aissrc
 * @title: aissrc
 *
 * The aissrc receives packets from a stream server.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v aissrc target-address=localhost:50053 ! decodebin !
 * autovideosink
 * ]|
 *
 * Receives packets from a stream server.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aissrc.h"

GST_DEBUG_CATEGORY_STATIC(ais_src_debug_category);
#define GST_CAT_DEFAULT ais_src_debug_category

/* prototypes*/

static void ais_src_set_property(GObject *object, guint property_id,
                                 const GValue *value, GParamSpec *pspec);
static void ais_src_get_property(GObject *object, guint property_id,
                                 GValue *value, GParamSpec *pspec);
static void ais_src_dispose(GObject *object);
static gboolean ais_src_start(GstBaseSrc *basesrc);
static gboolean ais_src_stop(GstBaseSrc *basesrc);
static GstFlowReturn ais_src_create(GstPushSrc *psrc, GstBuffer **outbuf);

/* Codes labelling the properties of the plugin.
 * The first code is a conventional zero sentinel.
 */
enum {
  PROP_0,
  PROP_TARGET_ADDRESS,
  PROP_STREAM_NAME,
  PROP_CONSUMER_NAME,
  PROP_USE_INSECURE_CHANNEL,
  PROP_SSL_DOMAIN_NAME,
  PROP_SSL_ROOT_CERT_PATH,
};

/* pad templates */

static GstStaticPadTemplate ais_src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(
    AisSrc, ais_src, GST_TYPE_PUSH_SRC,
    GST_DEBUG_CATEGORY_INIT(ais_src_debug_category, "aissrc", 0,
                            "debug category for the aissrc element"));

static void ais_src_class_init(AisSrcClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);
  GstBaseSrcClass *gstbasesrc_class = GST_BASE_SRC_CLASS(klass);
  GstPushSrcClass *gstpushsrc_class = GST_PUSH_SRC_CLASS(klass);

  gobject_class->set_property = ais_src_set_property;
  gobject_class->get_property = ais_src_get_property;

  g_object_class_install_property(
      gobject_class, PROP_TARGET_ADDRESS,
      g_param_spec_string("target-address",
                          "Address (ip:port) to the stream server",
                          "Address to the stream server", NULL,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(
      gobject_class, PROP_STREAM_NAME,
      g_param_spec_string("stream-name", "Stream name",
                          "Name of the destination stream on the stream server",
                          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(
      gobject_class, PROP_CONSUMER_NAME,
      g_param_spec_string("consumer-name", "Stream server consumer name",
                          "Consume name used to read from stream server", NULL,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(
      gobject_class, PROP_USE_INSECURE_CHANNEL,
      g_param_spec_boolean("use-insecure-channel", "Use insecure channel",
                           "Use an insecure channel", FALSE,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(
      gobject_class, PROP_SSL_DOMAIN_NAME,
      g_param_spec_string("ssl-domain-name", "SSL domain name",
                          "The expected ssl domain name of the server", NULL,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(
      gobject_class, PROP_SSL_ROOT_CERT_PATH,
      g_param_spec_string("ssl-root-cert-path", "SSL root certificate path",
                          "The file path to the root CA certificate", NULL,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata(
      gstelement_class, "AI Streamer source", "Generic",
      "Receives packets from an AI Streamer stream server", "Google Inc");

  gst_element_class_add_static_pad_template(gstelement_class,
                                            &ais_src_template);

  gobject_class->dispose = ais_src_dispose;
  gstbasesrc_class->start = GST_DEBUG_FUNCPTR(ais_src_start);
  gstbasesrc_class->stop = GST_DEBUG_FUNCPTR(ais_src_stop);
  gstpushsrc_class->create = GST_DEBUG_FUNCPTR(ais_src_create);
}

/* object initialization */

static void ais_src_init(AisSrc *src) {
  src->target_address = g_strdup("");
  src->stream_name = g_strdup("");
  src->consumer_name = g_strdup("");
  src->use_insecure_channel = FALSE;
  src->ssl_domain_name = g_strdup("");
  src->ssl_root_cert_path = g_strdup("");

  /* we operate in time */
  gst_base_src_set_format(GST_BASE_SRC(src), GST_FORMAT_TIME);
}

/**
 * ais_src_set_target_address
 * @src: (not nullable): ais src object.
 * @address: target address.
 * Returns: TRUE on success, FALSE otherwise.
 */
static gboolean ais_src_set_target_address(AisSrc *src, const gchar *address,
                                           GError **error) {
  if (src->ais_receiver != NULL) {
    goto stream_already_open;
  }

  if (address == NULL) {
    goto null_address;
  }

  g_free(src->target_address);
  src->target_address = g_strdup(address);

  return TRUE;

  /* Errors */
stream_already_open : {
  g_warning(
      "Changing the `target_address' property when the client "
      "is already connected is not supported");
  g_set_error(error, GST_URI_ERROR, GST_URI_ERROR_BAD_STATE,
              "Changing the 'target_address' property on when the client "
              "is already connected is not supported");
  return FALSE;
}

null_address : {
  GST_ELEMENT_ERROR(src, RESOURCE, NOT_FOUND,
                    ("A NULL target address was specified."), (NULL));
  return FALSE;
}
}

/**
 * ais_src_set_consumer_name
 * @src: (not nullable): ais src object.
 * @consumer_name: stream server consumer name.
 * Returns: TRUE on success, FALSE otherwise.
 */
static gboolean ais_src_set_consumer_name(AisSrc *src,
                                          const gchar *consumer_name,
                                          GError **error) {
  if (src->ais_receiver != NULL) {
    goto stream_already_open;
  }

  if (consumer_name == NULL) {
    goto empty_consumer_name;
  }

  g_free(src->consumer_name);
  src->consumer_name = g_strdup(consumer_name);
  return TRUE;

/* Errors */
stream_already_open : {
  g_warning(
      "Changing the `consumer_name' property when the stream server "
      "is already connected is not supported");
  g_set_error(error, GST_URI_ERROR, GST_URI_ERROR_BAD_STATE,
              "Changing the `consumer_name' property when the stream "
              "server is already connected is not supported");
  return FALSE;
}

empty_consumer_name : {
  GST_ELEMENT_ERROR(src, RESOURCE, NOT_FOUND, ("No consume name was specified"),
                    (NULL));
  return FALSE;
}
}

/**
 * ais_src_set_stream_name
 * @src: (not nullable): ais src object.
 * @stream_name: stream name.
 * Returns: TRUE on success, FALSE otherwise.
 */
static gboolean ais_src_set_stream_name(AisSrc *src, const gchar *stream_name,
                                        GError **error) {
  if (src->ais_receiver != NULL) {
    goto stream_already_open;
  }

  g_free(src->stream_name);
  if (stream_name != NULL) {
    src->stream_name = g_strdup(stream_name);
  } else {
    src->stream_name = g_strdup("");
  }

  return TRUE;

  /* Errors */
stream_already_open : {
  g_warning(
      "Changing the `stream_name' property on when the client "
      "is already connected is not supported");
  g_set_error(error, GST_URI_ERROR, GST_URI_ERROR_BAD_STATE,
              "Changing the 'stream_name' property on when the client "
              "already connected is not supported");
  return FALSE;
}
}

/**
 * ais_src_set_ssl_domain_name
 * @src: (not nullable): ais src object.
 * @ssl_domain_name: ssl domain name.
 * Returns: TRUE on success, FALSE otherwise.
 */
static gboolean ais_src_set_ssl_domain_name(AisSrc *src,
                                            const gchar *ssl_domain_name,
                                            GError **error) {
  if (src->ais_receiver != NULL) {
    goto stream_already_open;
  }

  g_free(src->ssl_domain_name);
  if (ssl_domain_name != NULL) {
    src->ssl_domain_name = g_strdup(ssl_domain_name);
  } else {
    src->ssl_domain_name = g_strdup("");
  }

  return TRUE;

  /* Errors */
stream_already_open : {
  g_warning(
      "Changing the `ssl_domain_name' property on when the client "
      "is already connected is not supported");
  g_set_error(error, GST_URI_ERROR, GST_URI_ERROR_BAD_STATE,
              "Changing the 'ssl_domain_name' property on when the client "
              "already connected is not supported");
  return FALSE;
}
}

/**
 * ais_src_set_ssl_root_cert_path
 * @src: (not nullable): ais src object.
 * @ssl_root_cert_path: ssl domain name.
 * Returns: TRUE on success, FALSE otherwise.
 */
static gboolean ais_src_set_ssl_root_cert_path(AisSrc *src,
                                               const gchar *ssl_root_cert_path,
                                               GError **error) {
  if (src->ais_receiver != NULL) {
    goto stream_already_open;
  }

  g_free(src->ssl_root_cert_path);
  if (ssl_root_cert_path != NULL) {
    src->ssl_root_cert_path = g_strdup(ssl_root_cert_path);
  } else {
    src->ssl_root_cert_path = g_strdup("");
  }

  return TRUE;

  /* Errors */
stream_already_open : {
  g_warning(
      "Changing the `ssl_root_cert_path' property when the client "
      "is already connected is not supported");
  g_set_error(error, GST_URI_ERROR, GST_URI_ERROR_BAD_STATE,
              "Changing the 'ssl_root_cert_path' property when the client "
              "already connected is not supported");
  return FALSE;
}
}

static void ais_src_set_property(GObject *object, guint property_id,
                                 const GValue *value, GParamSpec *pspec) {
  AisSrc *src = AIS_SRC(object);

  switch (property_id) {
    case PROP_TARGET_ADDRESS:
      ais_src_set_target_address(src, g_value_get_string(value), NULL);
      break;
    case PROP_STREAM_NAME:
      ais_src_set_stream_name(src, g_value_get_string(value), NULL);
      break;
    case PROP_CONSUMER_NAME:
      ais_src_set_consumer_name(src, g_value_get_string(value), NULL);
      break;
    case PROP_USE_INSECURE_CHANNEL:
      src->use_insecure_channel = g_value_get_boolean(value);
      break;
    case PROP_SSL_DOMAIN_NAME:
      ais_src_set_ssl_domain_name(src, g_value_get_string(value), NULL);
      break;
    case PROP_SSL_ROOT_CERT_PATH:
      ais_src_set_ssl_root_cert_path(src, g_value_get_string(value), NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
  }
}

static void ais_src_get_property(GObject *object, guint property_id,
                                 GValue *value, GParamSpec *pspec) {
  AisSrc *src = AIS_SRC(object);

  switch (property_id) {
    case PROP_TARGET_ADDRESS:
      g_value_set_string(value, src->target_address);
      break;
    case PROP_STREAM_NAME:
      g_value_set_string(value, src->stream_name);
      break;
    case PROP_CONSUMER_NAME:
      g_value_set_string(value, src->consumer_name);
      break;
    case PROP_USE_INSECURE_CHANNEL:
      g_value_set_boolean(value, src->use_insecure_channel);
      break;
    case PROP_SSL_DOMAIN_NAME:
      g_value_set_string(value, src->ssl_domain_name);
      break;
    case PROP_SSL_ROOT_CERT_PATH:
      g_value_set_string(value, src->ssl_root_cert_path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

void ais_src_dispose(GObject *object) {
  AisSrc *src = AIS_SRC(object);

  G_OBJECT_CLASS(ais_src_parent_class)->dispose(object);

  g_free(src->target_address);
  g_free(src->stream_name);
  g_free(src->consumer_name);
  g_free(src->ssl_domain_name);
  g_free(src->ssl_root_cert_path);
}

static GstFlowReturn ais_src_create(GstPushSrc *psrc, GstBuffer **outbuf) {
  AisSrc *src = AIS_SRC(psrc);

  AIS_Packet *ais_packet = NULL;
  AIS_PacketAs *ais_packet_as = NULL;

  ais_packet = AIS_NewPacket(src->ais_status);
  AIS_ReceivePacket(src->ais_receiver, ais_packet, src->ais_status);
  if (AIS_GetCode(src->ais_status) != AIS_OK) {
    goto failed_receive_packet;
  }

  ais_packet_as = AIS_NewGstreamerBufferPacketAs(ais_packet, src->ais_status);
  if (ais_packet_as == NULL) {
    goto failed_new_packet_as;
  }
  const AIS_GstreamerBuffer *ais_gstreamer_buffer =
      (const AIS_GstreamerBuffer *)AIS_PacketAsValue(ais_packet_as);

  // Change the caps to those of the incoming packet if it is different.
  const char *caps_string =
      AIS_GstreamerBufferGetCapsString(ais_gstreamer_buffer);
  GstCaps *new_caps = gst_caps_from_string(caps_string);
  GstCaps *caps = gst_pad_get_current_caps(GST_BASE_SRC(src)->srcpad);
  if (caps == NULL || gst_caps_is_equal(caps, new_caps) == FALSE) {
    g_print("Setting caps to %s", caps_string);
    gst_base_src_set_caps(GST_BASE_SRC(src), new_caps);
  }
  if (caps != NULL) {
    gst_caps_unref(caps);
  }
  gst_caps_unref(new_caps);

  // Allocate a GstBuffer and copy the packet payload into it.
  size_t buf_size = AIS_GstreamerBufferSize(ais_gstreamer_buffer);
  *outbuf = gst_buffer_new_and_alloc(buf_size);

  GstMapInfo map;
  if (gst_buffer_map(*outbuf, &map, GST_MAP_READWRITE)) {
    AIS_GstreamerBufferCopyTo(ais_gstreamer_buffer, (char *)map.data);
    gst_buffer_unmap(*outbuf, &map);
  } else {
    goto failed_buffer_map;
  }

finalize:
  AIS_DeleteGstreamerBufferPacketAs(ais_packet_as);
  AIS_DeletePacket(ais_packet);
  return GST_FLOW_OK;

failed_receive_packet : {
  GST_ELEMENT_ERROR(src, LIBRARY, FAILED, ("%s", AIS_Message(src->ais_status)),
                    ("%s", AIS_Message(src->ais_status)));
  goto finalize;
}

failed_new_packet_as : {
  GST_ELEMENT_ERROR(src, LIBRARY, FAILED, ("%s", AIS_Message(src->ais_status)),
                    ("%s", AIS_Message(src->ais_status)));
  goto finalize;
}

failed_buffer_map : {
  GST_ELEMENT_WARNING(src, STREAM, FAILED,
                      ("Failed to gst_buffer_map the given GstBuffer"), (NULL));
  goto finalize;
}
}

static gboolean ais_src_start(GstBaseSrc *bsrc) {
  AisSrc *src = AIS_SRC(bsrc);

  src->ais_status = AIS_NewStatus();

  src->ais_connection_options = AIS_NewConnectionOptions();
  AIS_SetTargetAddress(src->target_address, src->ais_connection_options);
  AIS_SetUseInsecureChannel(src->use_insecure_channel,
                            src->ais_connection_options);
  AIS_SetSslDomainName(src->ssl_domain_name, src->ais_connection_options);
  AIS_SetSslRootCertPath(src->ssl_root_cert_path, src->ais_connection_options);

  // TODO(dschao): Add the option to configure the consumer name.
  src->ais_receiver = AIS_NewReceiver(src->ais_connection_options,
                                      src->stream_name, src->ais_status);
  if (src->ais_receiver == NULL) {
    goto failed_new_receiver;
  }

  return TRUE;

failed_new_receiver : {
  GST_ELEMENT_ERROR(src, RESOURCE, NOT_FOUND,
                    ("%s", AIS_Message(src->ais_status)),
                    ("%s", AIS_Message(src->ais_status)));
  return FALSE;
}
}

static gboolean ais_src_stop(GstBaseSrc *bsrc) {
  AisSrc *src = AIS_SRC(bsrc);
  AIS_DeleteReceiver(src->ais_receiver);
  AIS_DeleteConnectionOptions(src->ais_connection_options);
  AIS_DeleteStatus(src->ais_status);
  return TRUE;
}

static gboolean plugin_init(GstPlugin *plugin) {
  return gst_element_register(plugin, "aissrc", GST_RANK_NONE, AIS_TYPE_SRC);
}

#ifndef VERSION
#define VERSION "0.0.1"
#endif
#ifndef PACKAGE
#define PACKAGE "ais_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "ais_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://nothing.org/"
#endif

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, aissrc,
                  "Stream server source", plugin_init, VERSION, "Proprietary",
                  PACKAGE_NAME, GST_PACKAGE_ORIGIN)
