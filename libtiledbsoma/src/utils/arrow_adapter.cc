/**
 * @file   arrow_adapter.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2022 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file defines the ArrowAdapter class.
 */

#include "arrow_adapter.h"
#include "../soma/column_buffer.h"
#include "../utils/logger.h"

namespace tiledbsoma {

using namespace tiledb;

void ArrowAdapter::release_schema(struct ArrowSchema* schema) {
    schema->release = nullptr;

    for (int i = 0; i < schema->n_children; ++i) {
        struct ArrowSchema* child = schema->children[i];
        if (schema->name != nullptr) {
            free((void*)schema->name);
            schema->name = nullptr;
        }
        if (child->release != NULL) {
            child->release(child);
        }
        free(child);
    }
    free(schema->children);

    struct ArrowSchema* dict = schema->dictionary;
    if (dict != nullptr) {
        if (dict->format != nullptr) {
            free((void*)dict->format);
            dict->format = nullptr;
        }
        if (dict->release != nullptr) {
            delete dict;
            dict = nullptr;
        }
    }

    LOG_TRACE("[ArrowAdapter] release_schema");
}

void ArrowAdapter::release_array(struct ArrowArray* array) {
    auto arrow_buffer = static_cast<ArrowBuffer*>(array->private_data);

    LOG_TRACE(fmt::format(
        "[ArrowAdapter] release_array {} use_count={}",
        arrow_buffer->buffer_->name(),
        arrow_buffer->buffer_.use_count()));

    // Delete the ArrowBuffer, which was allocated with new.
    // If the ArrowBuffer.buffer_ shared_ptr is the last reference to the
    // underlying ColumnBuffer, the ColumnBuffer will be deleted.
    delete arrow_buffer;

    if (array->buffers != nullptr) {
        free(array->buffers);
    }

    struct ArrowArray* dict = array->dictionary;
    if (dict != nullptr) {
        if (dict->buffers != nullptr) {
            free(dict->buffers);
            dict->buffers = nullptr;
        }
        if (dict->release != nullptr) {
            delete dict;
            dict = nullptr;
        }
    }

    array->release = nullptr;
}

std::pair<const void*, std::size_t> ArrowAdapter::_get_data_and_length(
    Enumeration& enmr, const void* dst) {
    switch (enmr.type()) {
        case TILEDB_BOOL: {
            // We must handle this specially because vector<bool> does
            // not store elements contiguously in memory
            auto data = enmr.as_vector<bool>();

            // Represent the Boolean vector with, at most, the last two
            // bits. In Arrow, Boolean values are LSB packed
            uint8_t src = 0;
            for (size_t i = 0; i < data.size(); ++i)
                src |= (data[i] << i);

            // Allocate a single byte to copy the bits into
            size_t sz = 1;
            dst = (const void*)malloc(sz);
            std::memcpy((void*)dst, &src, sz);

            return std::pair(dst, data.size());
        }
        case TILEDB_INT8: {
            auto data = enmr.as_vector<int8_t>();
            return std::pair(_fill_data_buffer(data, dst), data.size());
        }
        case TILEDB_UINT8: {
            auto data = enmr.as_vector<uint8_t>();
            return std::pair(
                ArrowAdapter::_fill_data_buffer(data, dst), data.size());
        }
        case TILEDB_INT16: {
            auto data = enmr.as_vector<int16_t>();
            return std::pair(
                ArrowAdapter::_fill_data_buffer(data, dst), data.size());
        }
        case TILEDB_UINT16: {
            auto data = enmr.as_vector<uint16_t>();
            return std::pair(
                ArrowAdapter::_fill_data_buffer(data, dst), data.size());
        }
        case TILEDB_INT32: {
            auto data = enmr.as_vector<int32_t>();
            return std::pair(
                ArrowAdapter::_fill_data_buffer(data, dst), data.size());
        }
        case TILEDB_UINT32: {
            auto data = enmr.as_vector<uint32_t>();
            return std::pair(
                ArrowAdapter::_fill_data_buffer(data, dst), data.size());
        }
        case TILEDB_INT64: {
            auto data = enmr.as_vector<int64_t>();
            return std::pair(
                ArrowAdapter::_fill_data_buffer(data, dst), data.size());
        }
        case TILEDB_UINT64: {
            auto data = enmr.as_vector<uint64_t>();
            return std::pair(
                ArrowAdapter::_fill_data_buffer(data, dst), data.size());
        }
        case TILEDB_FLOAT32: {
            auto data = enmr.as_vector<float>();
            return std::pair(
                ArrowAdapter::_fill_data_buffer(data, dst), data.size());
        }
        case TILEDB_FLOAT64: {
            auto data = enmr.as_vector<double>();
            return std::pair(
                ArrowAdapter::_fill_data_buffer(data, dst), data.size());
        }
        default:
            throw TileDBSOMAError(fmt::format(
                "ArrowAdapter: Unsupported TileDB dict datatype: {} ",
                tiledb::impl::type_to_str(enmr.type())));
    }
}

std::unique_ptr<ArrowSchema> tiledb_schema_to_arrow_schema(
    std::shared_ptr<ArraySchema> tiledb_schema) {
    auto ndim = tiledb_schema->domain().ndim();
    auto nattr = tiledb_schema->attribute_num();

    std::unique_ptr<ArrowSchema> arrow_schema = std::make_unique<ArrowSchema>();
    arrow_schema->format = "+s";
    arrow_schema->n_children = ndim + nattr;
    arrow_schema->release = &release_schema;
    arrow_schema->children = (ArrowSchema**)malloc(
        sizeof(ArrowSchema*) * arrow_schema->n_children);

    ArrowSchema* child;

    for (uint32_t i = 0; i < ndim; ++i) {
        auto dim = tiledb_schema->domain().dimension(i);
        child = arrow_schema->children[i] = (ArrowSchema*)malloc(
            sizeof(ArrowSchema));
        child->format = to_arrow_format(dim.type()).data();
        child->name = strdup(dim.name().c_str());
        child->metadata = nullptr;
        child->flags = 0;
        child->n_children = 0;
        child->dictionary = nullptr;
        child->children = nullptr;
        child->release = &release_schema;
    }

    for (uint32_t i = 0; i < nattr; ++i) {
        auto attr = tiledb_schema->attribute(i);
        child = arrow_schema->children[ndim + i] = (ArrowSchema*)malloc(
            sizeof(ArrowSchema));
        child->format = to_arrow_format(attr.type()).data();
        child->name = strdup(attr.name().c_str());
        child->metadata = nullptr;
        child->flags = attr.nullable() ? ARROW_FLAG_NULLABLE : 0;
        child->n_children = 0;
        child->dictionary = nullptr;
        child->children = nullptr;
        child->release = &release_schema;
    }

    return arrow_schema;
}

std::string_view ArrowAdapter::to_arrow_format(
    tiledb_datatype_t datatype, bool use_large) {
    switch (datatype) {
        case TILEDB_STRING_ASCII:
        case TILEDB_STRING_UTF8:
            return use_large ? "U" :
                               "u";  // large because TileDB uses 64bit offsets
        case TILEDB_CHAR:
        case TILEDB_BLOB:
            return use_large ? "Z" :
                               "z";  // large because TileDB uses 64bit offsets
        case TILEDB_BOOL:
            return "b";
        case TILEDB_INT32:
            return "i";
        case TILEDB_INT64:
            return "l";
        case TILEDB_FLOAT32:
            return "f";
        case TILEDB_FLOAT64:
            return "g";
        case TILEDB_INT8:
            return "c";
        case TILEDB_UINT8:
            return "C";
        case TILEDB_INT16:
            return "s";
        case TILEDB_UINT16:
            return "S";
        case TILEDB_UINT32:
            return "I";
        case TILEDB_UINT64:
            return "L";
        case TILEDB_TIME_SEC:
            return "tts";
        case TILEDB_TIME_MS:
            return "ttm";
        case TILEDB_TIME_US:
            return "ttu";
        case TILEDB_TIME_NS:
            return "ttn";
        case TILEDB_DATETIME_SEC:
            return "tss:";
        case TILEDB_DATETIME_MS:
            return "tsm:";
        case TILEDB_DATETIME_US:
            return "tsu:";
        case TILEDB_DATETIME_NS:
            return "tsn:";
        default:
            break;
    }
    throw TileDBSOMAError(fmt::format(
        "ArrowAdapter: Unsupported TileDB datatype: {} ",
        tiledb::impl::type_to_str(datatype)));
}

}  // namespace tiledbsoma