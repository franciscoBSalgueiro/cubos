#include <cubos/engine/cubos.hpp>
#include <cubos/engine/renderer/directional_light.hpp>
#include <cubos/engine/renderer/environment.hpp>
#include <cubos/engine/renderer/plugin.hpp>
#include <cubos/engine/renderer/point_light.hpp>
#include <cubos/engine/settings/settings.hpp>
#include <cubos/engine/transform/plugin.hpp>
#include <cubos/engine/voxels/plugin.hpp>

using cubos::core::ecs::Commands;
using cubos::core::ecs::Read;
using cubos::core::ecs::Write;

using namespace cubos::engine;

static const Asset<VoxelGrid> CastleAsset = AnyAsset("d5dc1160-9a91-4592-91e9-cff68e3d5f65");
static const Asset<VoxelPalette> PaletteAsset = AnyAsset("ff4539ba-93c0-4498-8ab6-147e93650c4a");

static void settingsSystem(Write<Settings> settings)
{
    settings->setString("assets.io.path", SAMPLE_ASSETS_FOLDER);
}

static void setPaletteSystem(Read<Assets> assets, Write<Renderer> renderer)
{
    // Read the palette's data and pass it to the renderer.
    auto palette = assets->read(PaletteAsset);
    (*renderer)->setPalette(*palette);
}

static void spawnLightSystem(Commands cmds)
{
    // Spawn the sun.
    cmds.create()
        .add(DirectionalLight{glm::vec3(1.0F), 1.0F})
        .add(Rotation{glm::quat(glm::vec3(glm::radians(45.0F), glm::radians(45.0F), 0))});
}

static void setEnvironmentSystem(Write<RendererEnvironment> env)
{
    env->ambient = {0.2F, 0.2F, 0.2F};
    env->skyGradient[0] = {0.1F, 0.2F, 0.4F};
    env->skyGradient[1] = {0.6F, 0.6F, 0.8F};
}

static void spawnCamerasSystem(Commands commands, Write<ActiveCameras> camera)
{
    // Spawn the a camera entity for the first viewport.
    camera->entities[0] =
        commands.create()
            .add(Camera{.fovY = 60.0F, .zNear = 0.1F, .zFar = 100.0F})
            .add(Position{{0.0, 15.0F, 40.0F}})
            .entity();
}

static void spawnCastleSystem(Commands cmds, Read<Assets> assets)
{
    auto castle = assets->read(CastleAsset);
    glm::vec3 offset = glm::vec3(castle->size().x, 0.0F, castle->size().z) / -2.0F;

    cmds.create().add(RenderableGrid{CastleAsset, offset}).add(LocalToWorld{});
}

int main()
{
    Cubos cubos{};
    cubos.addPlugin(rendererPlugin);
    cubos.addPlugin(voxelsPlugin);

    cubos.startupSystem(settingsSystem).tagged("cubos.settings");
    cubos.startupSystem(spawnLightSystem);
    cubos.startupSystem(setEnvironmentSystem);
    cubos.startupSystem(spawnCamerasSystem);

    cubos.startupSystem(setPaletteSystem).after("cubos.renderer.init");
    cubos.system(spawnCastleSystem);

    cubos.run();
}