# Translations

## List of currently supported languages

This is list of all languages that are currently supported by VCMI. If your languages is missing from the list and you wish to translate VCMI - please contact our team and we'll add support for your language in next release.

- Belarusian
- Bulgarian
- Czech
- Chinese (Simplified)
- Dutch
- English
- Filipino
- Finnish
- French
- German
- Greek
- Hungarian
- Italian
- Japanese
- Korean
- Latvian
- Norwegian
- Polish
- Portuguese (Brazilian)
- Romanian
- Russian
- Serbian
- Spanish
- Swedish
- Turkish
- Ukrainian
- Vietnamese

## Progress of the translations

You can see the current progress of the different translations here:
[Translation progress](https://github.com/vcmi/vcmi-translation-status)

The page will be automatically updated once a week.

## Translating Heroes III data

VCMI allows translating game data into languages other than English. In order to translate Heroes III in your language easiest approach is to:

- Copy existing translation, such as English translation from here: <https://github.com/vcmi-mods/h3-for-vcmi-englisation> (delete sound and video folders)
- Copy text-free images from here: <https://github.com/vcmi-mods/empty-translation>
- Rename mod to indicate your language, preferred form is "(language)-translation"
- Update mod.json to match your mod
- Translate all texts strings from `game.json`, `campaigns.json` and `maps.json`
- Replace images in data and sprites with translated ones (or delete it if you don't want to translate them)
- If unicode characters needed for language: Create a submod with a free font like here: <https://github.com/vcmi-mods/vietnamese-translation/tree/vcmi-1.4/vietnamese-translation/mods/VietnameseTrueTypeFonts>

If you can't produce some content on your own (like the images or the sounds):

- Create a `README.md` file at the root of the mod
- Write into the file the translations and the **detailled** location

This way, a contributor that is not a native speaker can do it for you in the future.

If you have already existing Heroes III translation you can:

- Install VCMI and select your localized Heroes III data files for VCMI data files
- Launch VCMI and start any map to get in game
- Press Tab to activate chat and enter `/translate`

This will export all strings from game into `Documents/My Games/VCMI/extracted/translation/` directory which you can then use to update json files in your translation.

To export maps and campaigns, use `/translate maps` command instead.

### Video subtitles

It's possible to add video subtitles. Create a JSON file in `video` folder of translation mod with the name of the video (e.g. `H3Intro.json`):

```json
[
    {
        "timeStart" : 5.640, // start time, seconds
        "timeEnd" : 8.120, // end time, seconds
        "text" : " ... " // text to show during this period
    },
    ...
]
```

## Translating VCMI data

VCMI contains several new strings, to cover functionality not existing in Heroes III. It can be roughly split into following parts:

- In-game texts, most noticeably - in-game settings menu.
- Game Launcher
- Map Editor
- Linux specific
- Android Launcher

### Translation of in-game data

Most of game data is translated using [Weblate](https://hosted.weblate.org/projects/vcmi/)

For usage of Weblate, please refer to [Weblate documentation](https://docs.weblate.org/en/latest/user/translating.html)

If something is not clear - feel free to ask us on Discord or forum. Translation made via Weblate will be automatically integrated into VCMI for next release

## Translating mods

### Exporting translation

If you want to start new translation for a mod or to update existing one you may need to export it first. To do that:

- Enable mod(s) that you want to export and set game language in Launcher to one that you want to target
- Launch VCMI and start any map to get in game
- Press Tab to activate chat and enter '/translate'

After that, start Launcher, switch to Help tab and open "log files directory". You can find exported json's in `extracted/translation` directory.

If your mod also contains maps or campaigns that you want to translate, then use `/translate maps` command instead.

If you want to update existing translation, you can use `/translate missing` command that will export only strings that were not translated

### Translating mod information

In order to display information in Launcher in language selected by user add following block into your `mod.json`:

```json
	"<language>" : {
		"name" : "<translated name>",
		"description" : "<translated description>",
		"author" : "<translated author>",
		"translations" : [
			"config/<modName>/<language>.json"
		]
	},
```

However, normally you don't need to use block for English. Instead, English text should remain in root section of your `mod.json` file, to be used when game can not find translated version.

### Translating in-game strings

After you have exported translation and added mod information for your language, copy exported file to `<mod directory>/Content/config/<mod name>/<language>.json`.

Use any text editor (Notepad++ is recommended for Windows) and translate all strings from this file to your language

## Developers documentation

### Adding new languages

In order to add new language it needs to be added in multiple locations in source code:

- Generate new .ts files for launcher and map editor, either by running `lupdate` with name of new `.ts` or by copying `english.ts` and editing language tag in the header.
- Add new language into `lib/Languages.h` entry. This will trigger static_assert's in places that needs an update in code
- Add new language into json schemas validation list - settings schema and mod schema
- Add new language into mod json format - in order to allow translation into new language

Also, make full search for a name of an existing language to ensure that there are not other places not referenced here

### Updating translation of Launcher and Map Editor to include new strings

At the moment, build system will generate binary translation files (`.qs`) that can be opened by Qt.
However, any new or changed lines will not be added into existing .ts files.
In order to update `.ts` files manually, open command line shell in `mapeditor` or `launcher` source directories and execute command

```sh
lupdate -no-obsolete * -ts translation/*.ts
```

This will remove any no longer existing lines from translation and add any new lines for all translations. If you want to keep old lines, remove `-no-obsolete` key from the command.
There *may* be a way to do the same via QtCreator UI or via CMake, if you find one feel free to update this information.

### Updating translation of Launcher and Map Editor using new .ts file from translators

Generally, this should be as simple as overwriting old files. Things that may be necessary if translation update is not visible in executable:

- Rebuild subproject (map editor/launcher).
- Regenerate translations via `lupdate -no-obsolete * -ts translation/*.ts`

### Translating the Installer

VCMI uses an Inno Setup installer that supports multiple languages. To add a new translation to the installer, follow these steps:

1. **Download the ISL file for your language:**
   - Visit the Inno Setup repository to find the language file you need:  
     [Inno Setup Languages](https://github.com/jrsoftware/issrc/tree/main/Files/Languages).

2. **Add custom VCMI messages:**
   - Open the downloaded ISL file and include the necessary VCMI-specific custom messages.
   - Refer to the `English.isl` file in the repository for examples of required custom messages.
   - Ensure that all messages, such as `WindowsVersionNotSupported` and `ConfirmUninstall`, are correctly translated and match the functionality described in the English version.

3. **Modify the `ConfirmUninstall` message:**
   - The VCMI installer uses a custom Uninstall Wizard. Ensure the `ConfirmUninstall` message is consistent with the English version and accurately reflects the intended functionality.

4. **Modify the `WindowsVersionNotSupported` message:**
   - Translate and update this message to ensure it aligns with the intended warning in the English version.

5. **Add the new language to the installer script:**
   - Edit the `[Languages]` section of the Inno Setup script.
   - Add an entry for your language, specifying the corresponding ISL file.

Example syntax for adding a language:

```text
[Languages]
Name: "english"; MessagesFile: "{#LangPath}\English.isl"
Name: "czech"; MessagesFile: "{#LangPath}\Czech.isl"
Name: "<your-language>"; MessagesFile: "{#LangPath}\<your-language>.isl"
```
