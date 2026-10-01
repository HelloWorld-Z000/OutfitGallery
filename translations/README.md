# Translating Outfit Gallery

No source-code changes or DLL rebuild are required. English and Japanese remain built in.

1. Copy `Translation.template.json` and rename the copy to `Translation.json`.
2. Edit only the values on the right inside `strings`. Keep the English keys on the left unchanged.
3. Save as UTF-8 JSON. Keep `schema` set to `1`.
4. Install it at `Data/SKSE/Plugins/OutfitGallery/Translation.json`.
   For an MO2 translation mod, the archive starts with `SKSE/Plugins/OutfitGallery/Translation.json`.
5. Restart Skyrim. In Outfit Gallery's Options, select `External (Translation.json)` under `Language / 言語`.

Example:
```json
{
  "schema": 1,
  "strings": {
    "Options": "Your translation",
    "Follower: %s": "Your translation: %s"
  }
}
```

Partial translations are supported. Missing, empty, unknown or invalid entries use English.
A missing or malformed file also falls back to English; see OutfitGallery.log for the load result.
Only one external translation is active. Replace the file to change external languages and restart.
Updates do not ship an active Translation.json, so your translation is not overwritten by this package.

Keep printf tokens exactly unchanged and in the same order: `%s`, `%u`, `%d`, `%.1f`, etc.
Do not add placeholders, `%n`, positional arguments, or `##` / `###` widget IDs.
Use JSON escapes for quotes, backslashes and line breaks. Keep trailing spaces in message prefixes.
Limits: file 4 MiB, each translated value 8192 UTF-8 bytes. A rejected entry falls back independently.

The template covers the gallery translation surface and common status messages. Mod item names,
user-created preset/tab names, framework section names and low-level diagnostic details are not renamed.
Text is rendered using SKSE Menu Framework's existing font. This package does not install fonts;
characters missing from that font require suitable framework font configuration.

You are welcome to distribute your Translation.json as a separate translation package.
