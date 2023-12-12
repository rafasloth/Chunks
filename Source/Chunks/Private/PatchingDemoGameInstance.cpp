// Fill out your copyright notice in the Description page of Project Settings.


#include "PatchingDemoGameInstance.h"

#include "ChunkDownloader.h"
#include "Misc/CoreDelegates.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/GameplayStatics.h"
#include "Json.h"

void UPatchingDemoGameInstance::Init()
{
    Super::Init();

    // Construct PatchVersionURL
    TArray<FString> TempCdnBaseUrls;
    GConfig->GetArray(TEXT("/Script/Plugins.ChunkDownloader"), TEXT("CdnBaseUrls"), TempCdnBaseUrls, GGameIni);

    if (TempCdnBaseUrls.Num() > 0) {
        BaseUrl = TempCdnBaseUrls[0];

        PatchVersionURL = BaseUrl + "/ContentBuildId.txt";
    }

    // create a new Http request and bind the response callback
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &UPatchingDemoGameInstance::OnPatchVersionResponse);

    // configure and send the request
    Request->SetURL(PatchVersionURL);
    Request->SetVerb("GET");
    Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
    Request->SetHeader("Content-Type", TEXT("application-json"));
    Request->ProcessRequest();

    
}

void UPatchingDemoGameInstance::OnPatchVersionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSucessful) {
    if (bWasSucessful) { // Pretty important to fix "Assertion failed: IsValid()" when CDN is down!!!

        // content build ID. Our Http response will provide this info from txt file. From Blueprint editable variable.
        FString ContentBuildId = Response->GetContentAsString(); // Throws assertion error popup if the CDN is down, because there wasn't a reponse!!!
        UE_LOG(LogTemp, Display, TEXT("Patch Content ID Response: %s"), *ContentBuildId);
        // initialize the chunk downloader with chosen platform
        TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetOrCreate();
        Downloader->Initialize(
            UGameplayStatics::GetPlatformName(), 8);

        // load the cached build ID
        Downloader->LoadCachedBuild(DeploymentName);

        // update the build manifest file
        TFunction<void(bool bSuccess)> UpdateCompleteCallback = [&](bool bSuccess) {bIsDownloadManifestUpToDate = bSuccess; };
        Downloader->UpdateBuild(DeploymentName, ContentBuildId, UpdateCompleteCallback);
    }
}

void UPatchingDemoGameInstance::Shutdown()
{
    Super::Shutdown();
    // Shut down ChunkDownloader
    FChunkDownloader::Shutdown();
}

void UPatchingDemoGameInstance::OnManifestUpdateComplete(bool bSuccess)
{
    bIsDownloadManifestUpToDate = bSuccess;
}

void UPatchingDemoGameInstance::GetLoadingProgress(int32& BytesDownloaded, int32& TotalBytesToDownload, float& DownloadPercent, int32& ChunksMounted, int32& TotalChunksToMount, float& MountPercent) const
{
    //Get a reference to ChunkDownloader
    TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

    //Get the loading stats struct
    FChunkDownloader::FStats LoadingStats = Downloader->GetLoadingStats();

    //Get the bytes downloaded and bytes to download
    BytesDownloaded = LoadingStats.BytesDownloaded;
    TotalBytesToDownload = LoadingStats.TotalBytesToDownload;

    //Get the number of chunks mounted and chunks to download
    ChunksMounted = LoadingStats.ChunksMounted;
    TotalChunksToMount = LoadingStats.TotalChunksToMount;

    //Calculate the download and mount percent using the above stats
    DownloadPercent = ((float)BytesDownloaded / (float)TotalBytesToDownload) * 100.0f;
    MountPercent = ((float)ChunksMounted / (float)TotalChunksToMount) * 100.0f;
}

void UPatchingDemoGameInstance::QueryDB() {
    // get the chunk downloader
    TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

    FString dbUrl = BaseUrl + "/" + Downloader->GetContentBuildId() + "/db.json";

    UE_LOG(LogTemp, Display, TEXT("DB URL is: %s"), *dbUrl);

    // GETS a STRING from contents of db.json
            // create a new Http request and bind the response callback
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &UPatchingDemoGameInstance::OnDbJsonResponse);

    // configure and send the request
    Request->SetURL(dbUrl);
    Request->SetVerb("GET");
    Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
    Request->SetHeader("Content-Type", TEXT("application-json"));
    Request->ProcessRequest();
    // Serialize that string as JSON
    // do stuff with the parsed JSON
}

bool UPatchingDemoGameInstance::PatchGame(int32 ChunkID)
{
    // make sure the download manifest is up to date
    if (bIsDownloadManifestUpToDate)
    {
        // get the chunk downloader
        TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

        // This might not be necessary since QueryDB runs at begin play.
        Downloader->GetAllChunkIds(ChunksInManifestList);
        
        // report manifest file's chunk status
        for (int32 ChunkID : ChunksInManifestList)
        {
            int32 ChunkStatus = static_cast<int32>(Downloader->GetChunkStatus(ChunkID));
            UE_LOG(LogTemp, Display, TEXT("Chunk %i status: %i"), ChunkID, ChunkStatus);

        }

        TFunction<void(bool bSuccess)> DownloadCompleteCallback = [&](bool bSuccess) {OnDownloadComplete(bSuccess); };

        ChunkDownloadList.Empty();

        if (ChunkID > 0) {
            ChunkDownloadList.Add(ChunkID);
            Downloader->DownloadChunks(ChunkDownloadList, DownloadCompleteCallback, 1);
            
            // start loading mode. This outside of the if might fix Assertion failed: !"Pak cannot be unmounted with outstanding requests" 
            TFunction<void(bool bSuccess)> LoadingModeCompleteCallback = [&](bool bSuccess) {OnLoadingModeComplete(bSuccess); };
            Downloader->BeginLoadingMode(LoadingModeCompleteCallback);
        }

        return true;
    }

    // you couldn't contact the server to validate your Manifest, so you can't patch
    UE_LOG(LogTemp, Display, TEXT("Manifest Update Failed. Can't patch the game"));

    return false;
}

void UPatchingDemoGameInstance::OnDbJsonResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSucessful) {
    if (bWasSucessful) { // Pretty important to fix "Assertion failed: IsValid()" when Web Server is down!!!
        // content build ID. Our Http response will provide this info from txt file. From Blueprint editable variable.
        FString Db = Response->GetContentAsString(); // Throws assertion error popup if the Web Server is down, because there wasn't a reponse!!!

        ProcessDbResponse(Db);
    }
}

void UPatchingDemoGameInstance::ProcessDbResponse(const FString& ResponseContent)
{
    // Validate http called us back on the Game Thread...
    check(IsInGameThread());

    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ResponseContent);
    TArray<TSharedPtr<FJsonValue>> OutArray;

    // get the chunk downloader
    TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();
    Downloader->GetAllChunkIds(ChunksInManifestList);

    if (FJsonSerializer::Deserialize(JsonReader, OutArray)) {
        if (OutArray.Num() > 0) {
            UE_LOG(LogTemp, Display, TEXT("Found items in the DB"));
            for(TSharedPtr<FJsonValue> val : OutArray) {
                TSharedPtr<FJsonObject> obj = val->AsObject();
                if (obj.IsValid()) {
                    int32 chunkId;
                    // Grab the int field 'id'.
                    bool result = obj->TryGetNumberField(TEXT("id"), chunkId);
                    // if false return
                    if (result == false) { return; }
                    // check if chunk id in db.json exists in manifest file
                    if (ChunksInManifestList.Contains(chunkId)) {
                        FJsonDlcInfo dlc_item;
                        dlc_item.ChunkId = chunkId;

                        FString pakTitle;
                        result = obj->TryGetStringField(TEXT("title"), pakTitle);
                        if (result == true) {
                            UE_LOG(LogTemp, Display, TEXT("Title of the Pak: %s"), *pakTitle);
                            dlc_item.Title = pakTitle;
                        }
                        FString pakType;
                        result = obj->TryGetStringField(TEXT("type"), pakType);
                        if (result == true) {
                            UE_LOG(LogTemp, Display, TEXT("Type of the Pak: %s"), *pakType);
                            dlc_item.Type = pakType;
                        }
                        FString pakDescription;
                        result = obj->TryGetStringField(TEXT("description"), pakDescription);
                        if (result == true) {
                            UE_LOG(LogTemp, Display, TEXT("Description of the Pak: %s"), *pakDescription);
                            dlc_item.Description = pakDescription;
                        }
                        FString pakThumbnailUrl;
                        result = obj->TryGetStringField(TEXT("thumbnailUrl"), pakThumbnailUrl);
                        if (result == true) {
                            UE_LOG(LogTemp, Display, TEXT("Thumbnail URL of the Pak: %s"), *pakThumbnailUrl);
                            dlc_item.ThumbnailUrl = pakThumbnailUrl;
                        }

                        // Check for duplicates and update existing one instead.
                        bool exists = false;
                        int index = -1;

                        for (int i = 0; i < TempDlcList.Num(); i++) {
                            if (TempDlcList[i].ChunkId == dlc_item.ChunkId) {
                                exists = true;
                                index = i;
                                break;
                            }
                        }
                        
                        if (!exists) {
                            TempDlcList.Add(dlc_item);
                        } else {
                            TempDlcList[index].Description = dlc_item.Description;
                            TempDlcList[index].ThumbnailUrl = dlc_item.ThumbnailUrl;
                            TempDlcList[index].Title = dlc_item.Title;
                            TempDlcList[index].Type = dlc_item.Type;
                        }
                    }
                }
            }

            OnTestDelegate.Broadcast();
        }
    }
}

void UPatchingDemoGameInstance::OnLoadingModeComplete(bool bSuccess)
{
    OnDownloadComplete(bSuccess);
}

void UPatchingDemoGameInstance::OnMountComplete(bool bSuccess)
{
    OnPatchComplete.Broadcast(bSuccess);
}

void UPatchingDemoGameInstance::OnDownloadComplete(bool bSuccess)
{
    if (bSuccess)
    {
        UE_LOG(LogTemp, Display, TEXT("Download complete"));

        // get the chunk downloader
        TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();
        FJsonSerializableArrayInt DownloadedChunks;

        for (int32 ChunkID : ChunkDownloadList)
        {
            DownloadedChunks.Add(ChunkID);
        }

        //Mount the chunks
        TFunction<void(bool bSuccess)> MountCompleteCallback = [&](bool bSuccess) {OnMountComplete(bSuccess); };
        Downloader->MountChunks(DownloadedChunks, MountCompleteCallback);

        OnPatchComplete.Broadcast(true);
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("Load process failed"));

        // call the delegate
        OnPatchComplete.Broadcast(false);
    }
}

void UPatchingDemoGameInstance::Finalize() {
    // get the chunk downloader
    TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();
    Downloader->Finalize();
}
