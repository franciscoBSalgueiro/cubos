/// @dir
/// @brief @ref entity-inspector-tool-plugin plugin directory.

/// @file
/// @brief Plugin entry point.
/// @ingroup entity-inspector-tool-plugin

#pragma once

#include <cubos/engine/cubos.hpp>

/// @brief Namespace for @ref tool-plugins "tool plugins".
namespace cubos::engine::tools
{
    /// @defgroup entity-inspector-tool-plugin Entity inspector
    /// @ingroup tool-plugins
    /// @brief Allows inspecting and modifying the components of the selected entity through a
    /// ImGui window.
    ///
    /// @note The selected entity is identified by the @ref EntitySelector resource.
    ///
    /// ## Dependencies
    /// - @ref imgui-plugin
    /// - @ref entity-selector-tool-plugin

    /// @brief Plugin entry function.
    /// @param cubos @b CUBOS. main class
    /// @ingroup entity-inspector-tool-plugin
    void entityInspectorPlugin(Cubos& cubos);
} // namespace cubos::engine::tools
