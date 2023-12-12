# Chunks
Unreal Engine 5 DLC Chunk Downloader Asset Manager PAK, UTOC and UCAS files test

## Append URLs to ChunkDownloader's CdnBaseUrls array

Unreal_Engine_Project\Config\DefaultGame.ini

```ini
[/Script/EngineSettings.GeneralProjectSettings]
ProjectID=SOME_LONG_HASH

[/Script/UnrealEd.ProjectPackagingSettings]
bGenerateChunks=True
bUseIoStore=False

[/Script/Plugins.ChunkDownloader]
+CdnBaseUrls="http://127.0.0.1/PatchingDemoCDN"
```

Development Packages:
Packaged_Game_Directory\Chunks\Saved\Config\Windows\Game.ini

Shipping Packages: C:\Users\USERNAME\AppData\Local\Chunks\Saved\Config\Windows or This PC\Quest 2\Internal shared storage\Android\data\MyCompany.MyProject\files\UnrealGame\MyProject\MyProject\Saved\Config\Android
Note: Works in shipping but inconsistent it might get overwritten by the game, it only allows one CdnBaseUrls line it completely makes the original of the base game stop working. You can't mix and match basically, choose one the base game or the custom url. 

```ini
[/Script/Plugins.ChunkDownloader]
CdnBaseUrls="http://not_127_0_0_1/PatchingDemoCDN"


```

## DLC Folder

This prototype only looks at DLC that mounts inside a /Game/DLC subfolder e.g: /Game/DLC/DLC_1

## Common Plugin

This base game uses the same common plugin available [here](https://github.com/rafasloth/ChunksCommon) also used by external projects that are meant to build DLC for this base game.

## Sample External Project

[This](https://github.com/rafasloth/EntirelyNew) is a sample external project in which you can build DLCs to be used with this main project.

## Tutorials
- [Part 1](https://www.youtube.com/watch?v=Lb3QNm7b6nQ&list=PLlwqvWO3ju6OV5zY7IArOqbja56ghdkNS&index=1&pp=iAQB)
- [Part 2](https://www.youtube.com/watch?v=rjlCaVYkEf8&list=PLlwqvWO3ju6OV5zY7IArOqbja56ghdkNS&index=2&pp=iAQB)
- [Part 3](https://www.youtube.com/watch?v=zeosPs_vRFs&list=PLlwqvWO3ju6OV5zY7IArOqbja56ghdkNS&index=3&pp=iAQB)
- [Part 4](https://www.youtube.com/watch?v=h7UwQhyQ2xQ&list=PLlwqvWO3ju6OV5zY7IArOqbja56ghdkNS&index=4&pp=iAQB)
- [Part 5](https://www.youtube.com/watch?v=SDretDE6cvc&list=PLlwqvWO3ju6OV5zY7IArOqbja56ghdkNS&index=5&pp=iAQB)
- [Part 6](https://www.youtube.com/watch?v=57Tou4-BlaU&list=PLlwqvWO3ju6OV5zY7IArOqbja56ghdkNS&index=6&pp=iAQB)
- [Part 7](https://www.youtube.com/watch?v=zyT-GtomK2s&list=PLlwqvWO3ju6OV5zY7IArOqbja56ghdkNS&index=7&pp=iAQB)
- [Part 8](https://www.youtube.com/watch?v=cjxux8O9540&list=PLlwqvWO3ju6OV5zY7IArOqbja56ghdkNS&index=8&pp=iAQB)
- [Part 9](https://www.youtube.com/watch?v=uMEa2eNJlG8&list=PLlwqvWO3ju6OV5zY7IArOqbja56ghdkNS&index=9&pp=iAQB)
- [Part 10](https://www.youtube.com/watch?v=MXAJnUIFY9s&list=PLlwqvWO3ju6OV5zY7IArOqbja56ghdkNS&index=10&pp=gAQBiAQB)
- [Part 10](https://www.youtube.com/watch?v=MXAJnUIFY9s&list=PLlwqvWO3ju6OV5zY7IArOqbja56ghdkNS&index=10&pp=gAQBiAQB)
- [Part 11](https://www.youtube.com/watch?v=ABEjybwr4M4)
- [Part 12](https://www.youtube.com/watch?v=hEG8qJecLbM)
- [Part 13](https://www.youtube.com/watch?v=GYT8uSaKuT8)
