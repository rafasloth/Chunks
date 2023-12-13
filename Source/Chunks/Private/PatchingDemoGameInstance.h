// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "PatchingDemoGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPatchCompleteDelegate, bool, Succeeded);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTestDelegate);

USTRUCT(BlueprintType)
struct FJsonDlcInfo {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DLC")
    int32 ChunkId;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DLC")
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DLC")
    FString Type = "Level";

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DLC")
    FString ThumbnailUrl;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DLC")
    FString Description = "DLC Description goes here";
};

/**
 * 
 */
UCLASS()
class CHUNKS_API UPatchingDemoGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
    // Overrides
    virtual void Init() override;
    virtual void Shutdown() override;
    const FString DeploymentName = "PatchingDemoCDN"; // TODO: Look into making it configurable

public:
    UFUNCTION(BlueprintPure, Category = "Patching|Stats")
    void GetLoadingProgress(int32& BytesDownloaded, int32& TotalBytesToDownload, float& DownloadPercent, int32& ChunksMounted, int32& TotalChunksToMount, float& MountPercent) const;

public:
    UFUNCTION(BlueprintCallable, Category = "Patching")
    void QueryDB();
    // Delegates
    // Fired when the patching process succeeds or fails
    UPROPERTY(BlueprintAssignable, Category = "Patching");
    FPatchCompleteDelegate OnPatchComplete;
    UPROPERTY(BlueprintAssignable, Category = "Patching")
    FTestDelegate OnTestDelegate;

public:
    // Starts the game patching process. Returns false if the patching manifest is not up to date. */
    UFUNCTION(BlueprintCallable, Category = "Patching")
    bool PatchGame(int32 ChunkID);

    void OnPatchVersionResponse(FHttpRequestPtr Request, FHttpResponsePtr response, bool bWasSucessful);
    void OnDbJsonResponse(FHttpRequestPtr Request, FHttpResponsePtr response, bool bWasSucessful);

    void ProcessDbResponse(const FString& ResponseContent);

    UPROPERTY(BlueprintReadOnly, Category = "Patching");
    TArray<FJsonDlcInfo> TempDlcList;

    // Unmount all chunks and cancel any downloads in progress (preserving partial downloads). Call only once, don't reuse this object, make a new one.
    UFUNCTION(BlueprintCallable, Category = "Patching")
    void Finalize();

protected:
    //Tracks if our local manifest file is up to date with the one hosted on our website
    bool bIsDownloadManifestUpToDate;

protected:
    //Called when the chunk download process finishes
    void OnManifestUpdateComplete(bool bSuccess);

protected:
    // List of Chunk IDs to try and download
    TArray<int32> ChunkDownloadList;
    TArray<int32> ChunksInManifestList;

    // Base URL for the Deployment
    FString BaseUrl;
    // URL variable to ContentBuildId.txt file
    FString PatchVersionURL;

protected:
    // Called when the chunk download process finishes
    void OnDownloadComplete(bool bSuccess);

    // Called whenever ChunkDownloader's loading mode is finished
    void OnLoadingModeComplete(bool bSuccess);

    // Called when ChunkDownloader finishes mounting chunks
    void OnMountComplete(bool bSuccess);
};
