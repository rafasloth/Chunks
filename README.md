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

Packaged_Game_Directory\Chunks\Saved\Config\Windows\Game.ini

```ini
[/Script/Plugins.ChunkDownloader]
CdnBaseUrls="http://not_127_0_0_1/PatchingDemoCDN"


```

## Tutorials
[Part 1](https://youtu.be/Lb3QNm7b6nQ?si=QMZpKzRoBZeh7bL0)
