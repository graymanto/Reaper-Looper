#pragma once

namespace reaperlooper
{

/**
 * A fixed-size window onto the (dynamically sized) Session grid, for
 * hardware controllers with a fixed physical surface - e.g. the APC mini
 * mk2's 8x8 pad grid.
 *
 * Unlike `sequenced`, where the hardware grid *is* the whole session grid
 * 1:1, this project's logical grid can grow past the physical surface (see
 * the plan's "grid scalability" decision), so the viewport is what
 * scrolls/pages across it - the same relationship Ableton's hardware
 * controllers have to Session View.
 *
 * Phase 3 only wires up a single, fixed {0,0,8,8} viewport (no UI exists
 * yet to scroll it) - clampTo() exists now so the concept and its edge
 * cases are already correct when Phase 4 adds paging controls.
 */
struct GridViewport
{
    int originTrack { 0 }; ///< Logical track index shown at physical column 0.
    int originRow { 0 };   ///< Logical row index shown at physical row 0.
    int width { 8 };
    int height { 8 };

    int logicalTrack (int physicalCol) const noexcept { return originTrack + physicalCol; }
    int logicalRow (int physicalRow) const noexcept { return originRow + physicalRow; }

    /**
     * Keeps the viewport within [0, totalTracks - width] / [0, totalRows - height]
     * (clamped to 0 if the grid is currently smaller than the viewport, so a
     * viewport never shows negative logical coordinates).
     */
    void clampTo (int totalTracks, int totalRows) noexcept
    {
        const int maxOriginTrack = totalTracks > width ? totalTracks - width : 0;
        const int maxOriginRow = totalRows > height ? totalRows - height : 0;

        originTrack = originTrack < 0 ? 0 : (originTrack > maxOriginTrack ? maxOriginTrack : originTrack);
        originRow = originRow < 0 ? 0 : (originRow > maxOriginRow ? maxOriginRow : originRow);
    }
};

} // namespace reaperlooper
