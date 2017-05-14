/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <arrow/ipc/api.h>

#include <arrow-glib/array.hpp>
#include <arrow-glib/error.hpp>
#include <arrow-glib/record-batch.hpp>
#include <arrow-glib/schema.hpp>

#include <arrow-glib/output-stream.hpp>

#include <arrow-glib/writer.hpp>

G_BEGIN_DECLS

/**
 * SECTION: writer
 * @section_id: writer-classes
 * @title: Writer classes
 * @include: arrow-glib/arrow-glib.h
 *
 * #GArrowStreamWriter is a class for writing data in stream mode.
 *
 * #GArrowFileWriter is a class for writing data in random access
 * mode.
 */

typedef struct GArrowStreamWriterPrivate_ {
  std::shared_ptr<arrow::ipc::RecordBatchStreamWriter> stream_writer;
} GArrowStreamWriterPrivate;

enum {
  PROP_0,
  PROP_STREAM_WRITER
};

G_DEFINE_TYPE_WITH_PRIVATE(GArrowStreamWriter,
                           garrow_stream_writer,
                           G_TYPE_OBJECT);

#define GARROW_STREAM_WRITER_GET_PRIVATE(obj)                   \
  (G_TYPE_INSTANCE_GET_PRIVATE((obj),                           \
                               GARROW_TYPE_STREAM_WRITER,       \
                               GArrowStreamWriterPrivate))

static void
garrow_stream_writer_finalize(GObject *object)
{
  GArrowStreamWriterPrivate *priv;

  priv = GARROW_STREAM_WRITER_GET_PRIVATE(object);

  priv->stream_writer = nullptr;

  G_OBJECT_CLASS(garrow_stream_writer_parent_class)->finalize(object);
}

static void
garrow_stream_writer_set_property(GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  GArrowStreamWriterPrivate *priv;

  priv = GARROW_STREAM_WRITER_GET_PRIVATE(object);

  switch (prop_id) {
  case PROP_STREAM_WRITER:
    priv->stream_writer =
      *static_cast<std::shared_ptr<arrow::ipc::RecordBatchStreamWriter> *>(g_value_get_pointer(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
garrow_stream_writer_get_property(GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
  switch (prop_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
garrow_stream_writer_init(GArrowStreamWriter *object)
{
}

static void
garrow_stream_writer_class_init(GArrowStreamWriterClass *klass)
{
  GObjectClass *gobject_class;
  GParamSpec *spec;

  gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize     = garrow_stream_writer_finalize;
  gobject_class->set_property = garrow_stream_writer_set_property;
  gobject_class->get_property = garrow_stream_writer_get_property;

  spec = g_param_spec_pointer("stream-writer",
                              "ipc::RecordBatchStreamWriter",
                              "The raw std::shared<arrow::ipc::RecordBatchStreamWriter> *",
                              static_cast<GParamFlags>(G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(gobject_class, PROP_STREAM_WRITER, spec);
}

/**
 * garrow_stream_writer_new:
 * @sink: The output of the writer.
 * @schema: The schema of the writer.
 * @error: (nullable): Return locatipcn for a #GError or %NULL.
 *
 * Returns: (nullable): A newly created #GArrowStreamWriter or %NULL on
 *   error.
 */
GArrowStreamWriter *
garrow_stream_writer_new(GArrowOutputStream *sink,
                         GArrowSchema *schema,
                         GError **error)
{
  std::shared_ptr<arrow::ipc::RecordBatchStreamWriter> arrow_stream_writer;
  auto status =
    arrow::ipc::RecordBatchStreamWriter::Open(garrow_output_stream_get_raw(sink).get(),
                                              garrow_schema_get_raw(schema),
                                              &arrow_stream_writer);
  if (garrow_error_check(error, status, "[ipc][stream-writer][open]")) {
    return garrow_stream_writer_new_raw(&arrow_stream_writer);
  } else {
    return NULL;
  }
}

/**
 * garrow_stream_writer_write_record_batch:
 * @stream_writer: A #GArrowStreamWriter.
 * @record_batch: The record batch to be written.
 * @error: (nullable): Return locatipcn for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_stream_writer_write_record_batch(GArrowStreamWriter *stream_writer,
                                        GArrowRecordBatch *record_batch,
                                        GError **error)
{
  auto arrow_stream_writer = garrow_stream_writer_get_raw(stream_writer);
  auto arrow_record_batch = garrow_record_batch_get_raw(record_batch);
  auto arrow_record_batch_raw = arrow_record_batch.get();

  auto status = arrow_stream_writer->WriteRecordBatch(*arrow_record_batch_raw);
  return garrow_error_check(error,
                            status,
                            "[ipc][stream-writer][write-record-batch]");
}

/**
 * garrow_stream_writer_close:
 * @stream_writer: A #GArrowStreamWriter.
 * @error: (nullable): Return locatipcn for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_stream_writer_close(GArrowStreamWriter *stream_writer,
                           GError **error)
{
  auto arrow_stream_writer = garrow_stream_writer_get_raw(stream_writer);

  auto status = arrow_stream_writer->Close();
  return garrow_error_check(error, status, "[ipc][stream-writer][close]");
}


G_DEFINE_TYPE(GArrowFileWriter,
              garrow_file_writer,
              GARROW_TYPE_STREAM_WRITER);

static void
garrow_file_writer_init(GArrowFileWriter *object)
{
}

static void
garrow_file_writer_class_init(GArrowFileWriterClass *klass)
{
}

/**
 * garrow_file_writer_new:
 * @sink: The output of the writer.
 * @schema: The schema of the writer.
 * @error: (nullable): Return locatipcn for a #GError or %NULL.
 *
 * Returns: (nullable): A newly created #GArrowFileWriter or %NULL on
 *   error.
 */
GArrowFileWriter *
garrow_file_writer_new(GArrowOutputStream *sink,
                       GArrowSchema *schema,
                       GError **error)
{
  std::shared_ptr<arrow::ipc::FileWriter> arrow_file_writer;
  auto status =
    arrow::ipc::FileWriter::Open(garrow_output_stream_get_raw(sink).get(),
                                 garrow_schema_get_raw(schema),
                                 &arrow_file_writer);
  if (garrow_error_check(error, status, "[ipc][file-writer][open]")) {
    return garrow_file_writer_new_raw(&arrow_file_writer);
  } else {
    return NULL;
  }
}

G_END_DECLS

GArrowStreamWriter *
garrow_stream_writer_new_raw(std::shared_ptr<arrow::ipc::StreamWriter> *arrow_stream_writer)
{
  auto stream_writer =
    GARROW_STREAM_WRITER(g_object_new(GARROW_TYPE_STREAM_WRITER,
                                        "stream-writer", arrow_stream_writer,
                                        NULL));
  return stream_writer;
}

std::shared_ptr<arrow::ipc::StreamWriter>
garrow_stream_writer_get_raw(GArrowStreamWriter *stream_writer)
{
  GArrowStreamWriterPrivate *priv;

  priv = GARROW_STREAM_WRITER_GET_PRIVATE(stream_writer);
  return priv->stream_writer;
}

GArrowFileWriter *
garrow_file_writer_new_raw(std::shared_ptr<arrow::ipc::FileWriter> *arrow_file_writer)
{
  auto file_writer =
    GARROW_FILE_WRITER(g_object_new(GARROW_TYPE_FILE_WRITER,
                                    "stream-writer", arrow_file_writer,
                                    NULL));
  return file_writer;
}

arrow::ipc::FileWriter *
garrow_file_writer_get_raw(GArrowFileWriter *file_writer)
{
  auto arrow_stream_writer =
    garrow_stream_writer_get_raw(GARROW_STREAM_WRITER(file_writer));
  auto arrow_file_writer_raw =
    dynamic_cast<arrow::ipc::FileWriter *>(arrow_stream_writer.get());
  return arrow_file_writer_raw;
}
