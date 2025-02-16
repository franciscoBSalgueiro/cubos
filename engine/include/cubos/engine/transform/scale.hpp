/// @file
/// @brief Component @ref cubos::engine::Scale.
/// @ingroup transform-plugin

#pragma once

#include <glm/glm.hpp>

namespace cubos::engine
{
    /// @brief Component which assigns a uniform scale to an entity.
    /// @sa LocalToWorld Holds the resulting transform matrix.
    /// @ingroup transform-plugin
    struct [[cubos::component("cubos/scale", VecStorage)]] Scale
    {
        float factor; ///< Uniform scale factor of the entity.
    };
} // namespace cubos::engine
