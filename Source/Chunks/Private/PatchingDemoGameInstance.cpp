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
        UE_LOG(LogTemp, Display, TEXT("Patch Content ID Response: %s"), "ContentBuildId");
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

bool UPatchingDemoGameInstance::PatchGame()
{
    // make sure the download manifest is up to date
    if (bIsDownloadManifestUpToDate)
    {
        // get the chunk downloader
        TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

        TArray<int32> OutChunkIds;

        Downloader->GetAllChunkIds(OutChunkIds);
        
        // report manifest file's chunk status
        for (int32 ChunkID : OutChunkIds)
        {
            int32 ChunkStatus = static_cast<int32>(Downloader->GetChunkStatus(ChunkID));
            UE_LOG(LogTemp, Display, TEXT("Chunk %i status: %i"), ChunkID, ChunkStatus);

            // Get the medatada and parse it for building the UI.
            // Using the Deployment ID as key:
                // CdnBaseUrls + / + Downloader->GetContentBuildId() + / + db.json
                // Parse JSON to a c++ object
                // Grab info for UI from that object

            FString dbUrl = "http://127.0.0.1/" + DeploymentName + "/" + Downloader->GetContentBuildId() + "/db.json";
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

        // report current chunk status
        for (int32 ChunkID : ChunkDownloadList)
        {
            int32 ChunkStatus = static_cast<int32>(Downloader->GetChunkStatus(ChunkID));
            UE_LOG(LogTemp, Display, TEXT("Chunk %i status: %i"), ChunkID, ChunkStatus);
        }

        TFunction<void(bool bSuccess)> DownloadCompleteCallback = [&](bool bSuccess) {OnDownloadComplete(bSuccess); };
        Downloader->DownloadChunks(ChunkDownloadList, DownloadCompleteCallback, 1);

        // TODO: Replace with something like this:
        // Downloader->DownloadChunk(ClickedButtonChunkID, DownloadCompleteCallback, 1);

        // start loading mode
        TFunction<void(bool bSuccess)> LoadingModeCompleteCallback = [&](bool bSuccess) {OnLoadingModeComplete(bSuccess); };
        Downloader->BeginLoadingMode(LoadingModeCompleteCallback);
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
        UE_LOG(LogTemp, Display, TEXT("DB Content Response: %s"), "Db");

        ProcessDbResponse(Db);
    }
}

void UPatchingDemoGameInstance::ProcessDbResponse(const FString& ResponseContent)
{
    // Validate http called us back on the Game Thread...
    check(IsInGameThread());

    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ResponseContent);
    TArray<TSharedPtr<FJsonValue>> OutArray;

    if (FJsonSerializer::Deserialize(JsonReader, OutArray)) {
        if (OutArray.Num() > 0) {
            UE_LOG(LogTemp, Display, TEXT("Found items in the DB"));
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