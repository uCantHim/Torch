#pragma once

#include <expected>
#include <iosfwd>
#include <string>

#include <trc_util/TypeUtils.h>

#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetSource.h"
#include "trc/assets/AssetType.h"
#include "trc/serial/asset.pb.h"

namespace trc
{
    /**
     * @brief May be specialized for an asset type to make it serializable.
     */
    template<AssetBaseType T>
    struct AssetSerializerTraits;

    struct AssetParseError
    {
        enum class Code
        {
            eSyntaxError,
            eSemanticError,
            eSystemError,
            eOther,
        };

        Code errorCode;
        std::string message;
    };

    template<AssetBaseType Asset>
    using AssetParseResult = std::expected<AssetData<Asset>, AssetParseError>;

    template<typename Asset>
    concept SerializableAsset =
        AssetBaseType<Asset>
        && util::CompleteType<AssetSerializerTraits<Asset>>
        && util::CompleteType<AssetData<Asset>>
        && requires (AssetData<Asset> data, std::istream& is, std::ostream& os) {
            { AssetSerializerTraits<Asset>{}.deserialize(is) } -> std::same_as<AssetParseResult<Asset>>;
            { AssetSerializerTraits<Asset>{}.serialize(data, os) };
        };

    template<AssetBaseType T>
    auto parseAsset(std::istream& is) -> AssetParseResult<T>
    {
        serial::AssetFile file;
        if (!file.ParseFromIstream(&is)) {
            return std::unexpected(AssetParseError{
                AssetParseError::Code::eSyntaxError,
                "Unable to parse asset data"
            });
        }

        if (file.metadata().type().name() != AssetType::make<T>().getName())
        {
            return std::unexpected(AssetParseError{
                AssetParseError::Code::eSemanticError,
                "Asset data is not of the requested type " + AssetType::make<T>().getName()
                + " (Actual type: " + file.metadata().type().name() + ")"
            });
        }

        std::stringstream ss{ file.asset_data() };
        return AssetSerializerTraits<T>::deserialize(ss);
    }

    template<SerializableAsset T>
    bool serializeAsset(const AssetData<T>& data,
                        std::ostream& os,
                        std::optional<AssetMetadata> meta = {})
    {
        serial::AssetFile file;

        // Write metadata
        if (meta) {
            *file.mutable_metadata() = meta->serialize();
        }
        else {
            *file.mutable_metadata() = AssetMetadata{
                .name="<no name specified in serializeAsset()>",
                .type=AssetType::make<T>(),
            }.serialize();
        }

        // Write asset data
        std::stringstream ss;
        AssetSerializerTraits<T>::serialize(data, ss);
        file.set_asset_data(ss.str());

        return file.SerializeToOstream(&os);
    }

    /**
     * A default implementation as a solution while I migrate the old interfaces
     * to the new AssetSerializerTraits<>.
     *
     * TODO: Remove this.
     */
    template<AssetBaseType T>
        requires std::default_initializable<AssetData<T>>
              && requires (AssetData<T> data, std::istream& is, std::ostream& os) {
                  { data.deserialize(is) };
                  { data.serialize(os) };
              }
    struct AssetSerializerTraits<T>
    {
        static auto deserialize(std::istream& is) -> AssetParseResult<T>
        {
            AssetData<T> data{};
            try {
                data.deserialize(is);
                return data;
            }
            catch (const std::exception& err) {
                return std::unexpected(AssetParseError{ AssetParseError::Code::eOther, err.what() });
            }
            catch (...) {
                return std::unexpected(AssetParseError{ AssetParseError::Code::eOther, "Unknown error" });
            }
        }

        static void serialize(const AssetData<T>& data, std::ostream& os)
        {
            data.serialize(os);
        }
    };
} // namespace trc
