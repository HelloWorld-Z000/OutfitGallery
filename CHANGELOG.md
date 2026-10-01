# Changelog

## 1.0.9 - Photo loading, display scaling and external translations

- Fix Missing photo when swap-chain device retrieval fails, using the renderer device as a fallback. Existing and newly saved photos were verified by an affected Steam 1.5.97 user.
- Retry failed photo loads with a short delay and log bounded diagnostic details.

- Integrate the user-verified DLSS layout correction from scale-fix1.
- Load an optional UTF-8 Translation.json at startup; retain built-in English and Japanese.
- Validate translation format tokens and fall back to English for missing or invalid entries. Include an English template and translation instructions.

- Enable Add missing base items by default for new settings or settings without this field. Preserve saved ON/OFF preferences.

## 1.0.8 - GOG loader fix and gallery layout improvements

- Fix the GOG 1.6.1179 SKSE compatibility identifier (store sub-version 1). GOG in-game verification is still pending.
- Reduce the minimum text-tab width from 105 to 60; longer labels still expand to fit.
- Calculate horizontal scrolling from the actual text-tab, icon and column-button widths.
- Fix thumbnail size flickering near the scrolling threshold by always reserving vertical scrollbar space in the photo list. Confirmed in game by the author.
- No preset format or equipment behavior changes.

## 1.0.7 - Steam 1.7.104 and GOG runtime compatibility

- Add Steam 1.7.104 (SKSE 2.3.1), user-tested with the compatibility build.
- Enable GOG 1.6.1179 (GOG SKSE 2.2.6); not yet verified in game.
- Update CommonLibSSE-NG from 4.39.3 to 10.1.0 for current runtime detection and Address Library support.
- Complete main package with INI. Retain the 1.0.6 gallery, independent preview and experimental follower features; no transmog or saved-data format changes.


## 1.0.6 — Follower support (experimental)

- Improve independent preview clarity by aligning the image to physical pixels and avoiding enlargement beyond native resolution. Smaller windows still scale to fit; larger windows may show margins. Full-body and close-up comparisons checked by the author in their environment.
- Add optional crosshair targeting of current humanoid followers for outfit changes and photography. Share the photo library and independent preview with player mode; keep weapons/ammunition. First-person entry is recommended.
- Track generated equipment separately for each actor and safely reclaim unused items after verified changes. Preserve pre-owned, transferred, customized, quest and ambiguous items. Persist cleanup identities in the SKSE co-save.
- Add per-follower **Maintain this follower's outfit** and **Restore standard outfit**. Maintenance is opt-in, checks periodically, skips unsafe states, does not regenerate missing gear and stops repeated conflicts. Standard restoration releases maintenance without resetting inventory, changing AI or rewriting shared NPC outfit data. Leveled/undefined default armor is unsupported and reported.
- Persist maintained actor/armor IDs with all-or-nothing load resolution. Add actor isolation, transfer, serialization and bounded-retry tests.
- User-confirmed: travel/restart maintenance and cleanup, hidden default armor restoration, release staying off after restart, preservation of transferred/pre-owned items, and two-follower independence. Other frameworks and unusual outfits remain unverified.
- Includes the optional movable/resizable independent preview introduced in 1.0.5. Follower support remains labeled experimental and off by default.

## 1.0.5 — Independent preview

- Optional movable/resizable live preview; disabled by default. Uses a centered camera and the same central crop for preview and saved photos. Original right-side mode remains available.
- Persist preview layout, preserve image aspect ratio, reuse GPU texture, rebuild on resolution/device changes, and fall back to normal mode on unsupported capture formats/errors.
- Support 8-bit, 10-bit and 16-bit floating-point preview textures. Improved preview and saved photos with Community Shaders vignette confirmed by the author. Other graphics configurations remain unverified.

## 1.0.4 — Replace conflicting armor in partial modes

- Head/accessory presets now replace overlapping worn armor instead of rejecting pieces that also use other slots. A conflicting multi-slot wig or garment is unequipped as a whole. Unrelated armor, weapons, and ammunition remain outside the replacement scope.
- Preserve incoming-preset slot validation and ownership checks. Update bilingual help to explain replacement behavior.

## 1.0.3 — Preserve weapons when changing outfits

- Capture and apply armor/clothing only in every mode, leaving weapons and ammunition equipped. Legacy weapon/ammo entries remain readable but are ignored during application and verification, including missing weapon mods.
- Extend the camera vertical-position minimum from 20 to -20 for footwear close-ups, including mouse controls and persisted camera settings.

## 1.0.2 — Compact camera toolbar

- Shorten the English registration label to Save headgear and replace the camera preset heading with a font-independent line icon. Hover over the icon for the localized Camera presets tooltip. Preserve wrapping when the available width is too small.

## 1.0.1 — Drag guide visibility

Use a high-contrast outline and insertion-side marker for photo reordering; emphasize tab drop targets and make the drag preview background opaque. Reordering remains delivery-only. Build and existing tests pass; new visual guides await in-game verification.

## 1.0.0 — First public release

Visual outfit registration and immediate application, independent head/accessory collections, favorites/categories/search, drag organization, recoverable Trash, configurable camera and capture controls, mouse/gamepad navigation, and English/Japanese UI.

Promotes the user-tested 0.14.9 feature set. Release metadata and public documentation have been updated; gameplay and data formats are unchanged.
