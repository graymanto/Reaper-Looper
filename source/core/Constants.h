#pragma once

/**
 * Placeholder for Phase 1 (core engine). Deliberately does NOT define fixed
 * grid dimensions the way `sequenced`'s Constants.h does (kNumTracks = 8,
 * kNumRows = 8) - per the plan, this project's logical grid is dynamically
 * sized, with a hardware controller's fixed physical surface (e.g. the APC
 * mini mk2's 8x8 pads) acting as a scrollable viewport onto it rather than
 * constraining the model itself.
 */
