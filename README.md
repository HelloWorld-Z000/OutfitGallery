# Outfit Gallery — Visual Outfit Manager

**Version 1.0.0** · English / 日本語 · Native SKSE plugin

Build a visual wardrobe: photograph your equipped items, browse your collection, and change equipment by selecting a picture. Outfit changes appear while the gallery is open.

## Features

- Full outfit, head equipment, and accessory collections.
- Favorites, named categories, search, drag-to-category, manual ordering, and recoverable Trash.
- Two to five thumbnail columns, clear selection highlights, mouse and gamepad navigation.
- Mouse camera controls, seven camera presets, and PNG photos capped at 1024 pixels wide.
- Configurable opening tab, menu/capture keys, gamepad hold duration, and English/Japanese interface.
- Presets and settings stored outside saves for use across characters and new games.

## Requirements and compatibility

Install the SKSE build and Address Library matching your Skyrim executable, plus **SKSE Menu Framework 3** and its own dependencies. This plugin needs the framework's event-priority and input-event API; older incompatible framework builds will not open the gallery. Dependencies are not bundled.

| Steam Skyrim runtime | Status |
| --- | --- |
| 1.5.97 | Startup and equipment switching confirmed by the tester during development |
| 1.6.1170 | Main development test environment; gallery, camera and equipment workflows user-tested |
| 1.6.353, 1.6.640, 1.6.1130 | Accepted by the DLL; not verified in game |
| GOG / VR / other runtimes | Not enabled |

The 1.0.0 build has automated build/test verification. The final in-game appearance was confirmed on 0.14.9; 1.0.0 changes release metadata and documentation only. The table does not imply every feature was tested on every runtime.

SmoothCam and True Directional Movement are optional; their public APIs are used when available. No outfits, wigs, accessories, fonts or other mods are included. Install the original item mods referenced by your presets.

## Install or update

1. Exit Skyrim. Install the main ZIP through your mod manager. It contains `SKSE/Plugins/OutfitGallery.dll` and `OutfitGallery.ini`; there is no ESP.
2. Install/enable the required dependencies and launch through SKSE.
3. Load a game and press **F8**, or hold **Back / View for 0.8 seconds**, to open the gallery.

When updating, retain `Data/SKSE/Plugins/OutfitGallery/` and any customized INI. In MO2, generated files normally appear in **Overwrite**, or in the mod already providing those files. Back up that directory before replacing/removing an installation. The release ZIP contains no personal photos, presets or saved settings.

## Register and apply

| Mode | Registration | Selecting a photo |
| --- | --- | --- |
| Outfit / text tabs | Wear the complete outfit and register | Replace the full equipment set |
| Head / person icon | Register head items in enabled slots (30, 31, 41, 42 by default) | Change head equipment while retaining other equipment |
| Accessories / ring icon | Wear **only the items you want to add**, then register | Add those items, replacing equipment in overlapping slots |

Accessory mode uses what you are wearing, not an automatic jewelry classifier. For close-up accessory photos, adjust the camera before registration. Head and accessory photos have separate collections and do not appear in All or regular categories. They can be moved to Trash.

Registration opens the matching collection and selects the new photo. The upper-right **?** button opens help. Options controls the opening tab (default: All), language, keys, head slots and display settings.

## Default controls

| Action | Keyboard / mouse | Gamepad (Xbox labels) |
| --- | --- | --- |
| Open gallery | F8 | Hold Back / View for 0.8 s |
| Register outfit | F9 / button | Y |
| Register head equipment | F10 / button | X |
| Register accessories | F11 / button | Left stick click |
| Apply photo | Left click | A |
| Select photos / move through tab strip | Mouse | D-pad / left stick |
| Cycle tabs | Click tab | LB / RB |
| Head / accessory tab | Click icon | LT / RT (L2 / R2) |
| Photo menu | Right click photo | Right stick click |
| Change columns | Click grid icon | Move sideways to grid icon, then A |
| Close gallery | Upper-right X / Esc | B |

Up from the first photo row selects the tab strip. Down returns to photos. Menu and registration bindings can be changed in Options. Gamepad hotkeys can also activate bindings in other mods; choose an unused button if necessary.

In the right camera frame: **left drag** pans, **right drag** orbits/tilts, and the **wheel** changes distance. Preset 1 restores the built-in composition; slots 2–7 can store custom compositions. Entry from an existing free camera is optional and experimental.

## Persistence and item handling

`Data/SKSE/Plugins/OutfitGallery/` contains `Captures/`, `Presets/`, `Library.json`, `StudioSettings.json`, `CameraPresets.json` and `Hotkeys.json`. Preserve the entire directory to retain the gallery. Hotkeys.json takes precedence over INI defaults.

Presets record **base item identities** (source plugin and local FormID). Tempering, custom enchantments/names, poisons, spells and shouts are not restored. Original item plugins must be installed with unchanged identities. Equipment with changed slot assignments or overlapping multi-slot armor can prevent partial application; the tool reports this rather than removing unrelated equipment.

Enable **Add missing base items** to restore items you do not own, including on a new character. Newly created items are tracked conservatively and reclaimed after they are unequipped by a verified gallery change. Existing owned items are retained. Ambiguous, transferred, customized or previously untracked items may remain. Ownership tracking belongs to the character's SKSE co-save; retain matching `.ess` and `.skse` files. Permanent deletion from Trash removes the photo/preset, not inventory items.

## Troubleshooting

- Gallery does not open: check the executable/SKSE/Address Library match and the Menu Framework API/dependencies. Check `Documents/My Games/Skyrim Special Edition/SKSE/OutfitGallery.log` (the Documents folder may be redirected).
- Japanese displays as `?`: configure a Japanese-capable font in Menu Framework. Switching this plugin's language alone does not install a font. Use font/settings files appropriate for your framework version.
- Blurred scene: check Menu Framework's `BlurBackgroundOnMenu` setting and your depth-of-field configuration. This is a photo tool, so a framework background blur can obscure the subject.
- Photo cannot restore: ensure the original item plugins are enabled and the items are owned, or enable Add missing base items. See the status message/log for slot conflicts.

## Source and credits

See [BUILD.md](BUILD.md) for rebuilding, [THIRD_PARTY.md](THIRD_PARTY.md) for dependency provenance, and [LICENSE](LICENSE) / `LICENSES/` for licenses. Project source is provided under GPL-3.0 as documented in THIRD_PARTY.md. The implementation was authored with substantial AI assistance. Third-party runtime dependencies remain separately installed.
