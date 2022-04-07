#pragma once

#include "slang-gfx.h"
#include "source/core/slang-basic.h"
#include "source/core/slang-render-api-util.h"
#include "tools/unit-test/slang-unit-test.h"

using namespace Slang;
using namespace gfx;

namespace gfx_test
{
    struct ValidationTextureFormatBase : RefObject
    {
        virtual void validateBlocksEqual(const void* actual, const void* expected) = 0;

        virtual void initializeTexel(void* texel, int x, int y, int z, int mipLevel, int arrayLayer) = 0;
    };

    template <typename T>
    struct ValidationTextureFormat : ValidationTextureFormatBase
    {
        int componentCount;

        ValidationTextureFormat(int componentCount) : componentCount(componentCount) {};

        virtual void validateBlocksEqual(const void* actual, const void* expected) override
        {
            auto a = (const T*)actual;
            auto e = (const T*)expected;

            for (Int i = 0; i < componentCount; ++i)
            {
                SLANG_CHECK(a[i] == e[i]);
            }
        }

        virtual void initializeTexel(void* texel, int x, int y, int z, int mipLevel, int arrayLayer) override
        {
            auto temp = (T*)texel;

            switch (componentCount)
            {
            case 1:
                temp[0] = T(x + y + z + mipLevel + arrayLayer);
                break;
            case 2:
                temp[0] = T(x + z + arrayLayer);
                temp[1] = T(y + mipLevel);
                break;
            case 3:
                temp[0] = T(x + mipLevel);
                temp[1] = T(y + arrayLayer);
                temp[2] = T(z);
                break;
            case 4:
                temp[0] = T(x + arrayLayer);
                temp[1] = (T)y;
                temp[2] = (T)z;
                temp[3] = (T)mipLevel;
                break;
            default:
                assert(!"component count should be no greater than 4");
                SLANG_CHECK_ABORT(false);
            }
        }
    };

    template <typename T>
    struct PackedValidationTextureFormat : ValidationTextureFormatBase
    {
        int rBits;
        int gBits;
        int bBits;
        int aBits;

        PackedValidationTextureFormat(int rBits, int gBits, int bBits, int aBits)
            : rBits(rBits), gBits(gBits), bBits(bBits), aBits(aBits) {};

        virtual void validateBlocksEqual(const void* actual, const void* expected) override
        {
            T a[4];
            T e[4];
            unpackTexel(*(const T*)actual, a);
            unpackTexel(*(const T*)expected, e);

            for (Int i = 0; i < 4; ++i)
            {
                SLANG_CHECK(a[i] == e[i]);
            }
        }

        virtual void initializeTexel(void* texel, int x, int y, int z, int mipLevel, int arrayLayer) override
        {
            T temp = 0;

            // The only formats which currently use this have either 3 or 4 channels. TODO: BC formats?
            if (aBits == 0)
            {
                temp |= z;
                temp <<= gBits;
                temp |= (y + arrayLayer);
                temp <<= rBits;
                temp |= (x + mipLevel);
            }
            else
            {
                temp |= mipLevel;
                temp <<= bBits;
                temp |= z;
                temp <<= gBits;
                temp |= y;
                temp <<= rBits;
                temp |= (x + arrayLayer);
            }

            *(T*)texel = temp;
        }

        void unpackTexel(T texel, T* outComponents)
        {
            outComponents[0] = texel & ((1 << rBits) - 1);
            texel >>= rBits;

            outComponents[1] = texel & ((1 << gBits) - 1);
            texel >>= gBits;

            outComponents[2] = texel & ((1 << bBits) - 1);
            texel >>= bBits;

            outComponents[3] = texel & ((1 << aBits) - 1);
            texel >>= aBits;
        }
    };

    // Struct containing texture data and information for a specific subresource.
    struct ValidationTextureData : RefObject
    {
        const void* textureData;
        ITextureResource::Size extents;
        ITextureResource::Offset3D strides;

        void* getBlockAt(Int x, Int y, Int z)
        {
            assert(x >= 0 && x < extents.width);
            assert(y >= 0 && y < extents.height);
            assert(z >= 0 && z < extents.depth);

            char* layerData = (char*)textureData + z * strides.z;
            char* rowData = layerData + y * strides.y;
            return rowData + x * strides.x;
        }
    };

    // Struct containing relevant information for a texture, including a list of its subresources
    // and all relevant information for each subresource.
    struct TextureInfo : RefObject
    {
        Format format;
        uint32_t texelSize;
        ITextureResource::Type textureType;

        ITextureResource::Size extents;
        uint32_t mipLevelCount;
        uint32_t arrayLayerCount;

        List<RefPtr<ValidationTextureData>> subresourceObjects;
        List<ITextureResource::SubresourceData> subresourceDatas;
    };

    RefPtr<ValidationTextureFormatBase> getValidationTextureFormat(Format format);
    TextureAspect getTextureAspect(gfx::Format format);
    uint32_t getSubresourceIndex(uint32_t mipLevel, uint32_t mipLevelCount, uint32_t baseArrayLayer);
    void generateTextureData(RefPtr<TextureInfo> texture, ValidationTextureFormatBase* validationFormat);
}
