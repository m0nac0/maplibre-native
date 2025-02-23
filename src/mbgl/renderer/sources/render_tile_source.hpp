#pragma once

#include <mbgl/renderer/render_source.hpp>
#include <mbgl/renderer/source_state.hpp>
#include <mbgl/renderer/tile_pyramid.hpp>
#include <mbgl/style/sources/vector_source_impl.hpp>

namespace mbgl {

/**
 * @brief Base class for render sources that provide render tiles.
 */
class RenderTileSource : public RenderSource {
public:
    ~RenderTileSource() override;

    bool isLoaded() const override;

    std::unique_ptr<RenderItem> createRenderItem() override;
    void prepare(const SourcePrepareParameters&) override;
    void updateFadingTiles() override;
    bool hasFadingTiles() const override;

    RenderTiles getRenderTiles() const override;
    RenderTiles getRenderTilesSortedByYPosition() const override;
    const Tile* getRenderedTile(const UnwrappedTileID&) const override;

    std::unordered_map<std::string, std::vector<Feature>> queryRenderedFeatures(
        const ScreenLineString& geometry,
        const TransformState& transformState,
        const std::unordered_map<std::string, const RenderLayer*>& layers,
        const RenderedQueryOptions& options,
        const mat4& projMatrix) const override;

    std::vector<Feature> querySourceFeatures(const SourceQueryOptions&) const override;

    void setFeatureState(const std::optional<std::string>&, const std::string&, const FeatureState&) override;

    void getFeatureState(FeatureState& state, const std::optional<std::string>&, const std::string&) const override;

    void removeFeatureState(const std::optional<std::string>&,
                            const std::optional<std::string>&,
                            const std::optional<std::string>&) override;

    void reduceMemoryUse() override;
    void dumpDebugLogs() const override;

protected:
    RenderTileSource(Immutable<style::Source::Impl>);
    TilePyramid tilePyramid;
    Immutable<std::vector<RenderTile>> renderTiles;
    mutable RenderTiles filteredRenderTiles;
    mutable RenderTiles renderTilesSortedByY;

private:
    float bearing = 0.0F;
    SourceFeatureState featureState;
};

/**
 * @brief Base class for render sources that use tile sets.
 */
class RenderTileSetSource : public RenderTileSource {
protected:
    RenderTileSetSource(Immutable<style::Source::Impl>);
    ~RenderTileSetSource() override;

    virtual void updateInternal(const Tileset&,
                                const std::vector<Immutable<style::LayerProperties>>&,
                                bool needsRendering,
                                bool needsRelayout,
                                const TileParameters&) = 0;
    // Returns tileset from the current impl.
    virtual const std::optional<Tileset>& getTileset() const = 0;

private:
    uint8_t getMaxZoom() const final;
    void update(Immutable<style::Source::Impl>,
                const std::vector<Immutable<style::LayerProperties>>&,
                bool needsRendering,
                bool needsRelayout,
                const TileParameters&) final;

    std::optional<Tileset> cachedTileset;
};

} // namespace mbgl
