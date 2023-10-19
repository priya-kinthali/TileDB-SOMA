#ifndef ARROW_ADAPTER_H
#define ARROW_ADAPTER_H

#include <tiledb/tiledb>
#include <tiledb/tiledb_experimental>

// https://arrow.apache.org/docs/format/CDataInterface.html
// https://arrow.apache.org/docs/format/Columnar.html#buffer-listing-for-each-layout
// https://arrow.apache.org/docs/format/CDataInterface.html#exporting-a-simple-int32-array

#ifndef ARROW_SCHEMA_AND_ARRAY_DEFINED
#include "carrow.h"
#endif
namespace tiledbsoma {

using namespace tiledb;

class ColumnBuffer;

/**
 * @brief The ArrowBuffer holds a shared pointer to a ColumnBuffer, which
 * manages the lifetime of a ColumnBuffer used to back an Arrow array.
 *
 * The ArrowArray.release callback will delete the ArrowBuffer, and
 * automatically decrement the use count of the ColumnBuffer's shared pointer.
 *
 */
struct ArrowBuffer {
    ArrowBuffer(std::shared_ptr<ColumnBuffer> buffer)
        : buffer_(buffer){};

    std::shared_ptr<ColumnBuffer> buffer_;
};

class ArrowAdapter {
   public:
    static void release_schema(struct ArrowSchema* schema);
    static void release_array(struct ArrowArray* array);

    /**
     * @brief Convert ColumnBuffer to an Arrow array.
     *
     * @return std::pair<std::unique_ptr<ArrowArray>,
     * std::unique_ptr<ArrowSchema>>
     */
    static std::pair<std::unique_ptr<ArrowArray>, std::unique_ptr<ArrowSchema>>
    to_arrow(std::shared_ptr<ColumnBuffer> column);

    static std::pair<const void*, std::size_t> _get_data_and_length(
        Enumeration& enmr, const void* dst) {
        switch (enmr.type()) {
            case TILEDB_BOOL:
            case TILEDB_INT8: {
                auto data = enmr.as_vector<int8_t>();
                return std::pair(_fill_data_buffer(data, dst), data.size());
            }
            case TILEDB_UINT8: {
                auto data = enmr.as_vector<uint8_t>();
                return std::pair(_fill_data_buffer(data, dst), data.size());
            }
            case TILEDB_INT16: {
                auto data = enmr.as_vector<int16_t>();
                return std::pair(_fill_data_buffer(data, dst), data.size());
            }
            case TILEDB_UINT16: {
                auto data = enmr.as_vector<uint16_t>();
                return std::pair(_fill_data_buffer(data, dst), data.size());
            }
            case TILEDB_INT32: {
                auto data = enmr.as_vector<int32_t>();
                return std::pair(_fill_data_buffer(data, dst), data.size());
            }
            case TILEDB_UINT32: {
                auto data = enmr.as_vector<uint32_t>();
                return std::pair(_fill_data_buffer(data, dst), data.size());
            }
            case TILEDB_INT64: {
                auto data = enmr.as_vector<int64_t>();
                return std::pair(_fill_data_buffer(data, dst), data.size());
            }
            case TILEDB_UINT64: {
                auto data = enmr.as_vector<uint64_t>();
                return std::pair(_fill_data_buffer(data, dst), data.size());
            }
            default:
                throw TileDBSOMAError(fmt::format(
                    "ArrowAdapter: Unsupported TileDB dict datatype: {} ",
                    tiledb::impl::type_to_str(enmr.type())));
        }
    }

    template <typename T>
    static const void* _fill_data_buffer(std::vector<T> src, const void* dst) {
        auto sz = src.size() * sizeof(T);
        dst = (const void*)malloc(sz);
        std::memcpy((void*)dst, src.data(), sz);
        return dst;
    }

    static std::unique_ptr<ArrowSchema> tiledb_schema_to_arrow_schema(
        std::shared_ptr<ArraySchema> tiledb_schema,
        std::map<std::string, Enumeration> attr_to_enmr = {});

    /**
     * @brief Get Arrow format string from TileDB datatype.
     *
     * @param datatype TileDB datatype.
     * @return std::string_view Arrow format string.
     */
    static std::string_view to_arrow_format(
        tiledb_datatype_t datatype, bool use_large = true);

   private:
    static std::pair<const void*, std::size_t> _get_data_and_length(
        Enumeration& enmr, const void* dst);

    template <typename T>
    static const void* _fill_data_buffer(std::vector<T> src, const void* dst) {
        auto sz = src.size() * sizeof(T);
        dst = (const void*)malloc(sz);
        std::memcpy((void*)dst, src.data(), sz);
        return dst;
    }
};
};  // namespace tiledbsoma

#endif