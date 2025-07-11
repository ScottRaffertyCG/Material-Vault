// Copyright Epic Games, Inc. All Rights Reserved.

#include "MaterialVaultThumbnailManager.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialRenderProxy.h"
#include "RenderingThread.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Slate/SlateTextures.h"
#include "TextureResource.h"
#include "Async/Async.h"

FMaterialVaultThumbnailManager::FMaterialVaultThumbnailManager()
	: DefaultThumbnailSize(128)
	, MaxCacheSize(1000)
	, DefaultMaterialTexture(nullptr)
	, ErrorTexture(nullptr)
	, bIsInitialized(false)
{
}

FMaterialVaultThumbnailManager::~FMaterialVaultThumbnailManager()
{
	if (bIsInitialized)
	{
		Shutdown();
	}
}

void FMaterialVaultThumbnailManager::Initialize()
{
	if (bIsInitialized)
	{
		return;
	}
	
	// Initialize default textures
	DefaultMaterialTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EditorMaterials/DefaultMaterial"));
	ErrorTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EditorMaterials/DefaultDiffuse"));
	
	bIsInitialized = true;
}

void FMaterialVaultThumbnailManager::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}
	
	// Clear cache
	ClearThumbnailCache();
	PendingThumbnails.Empty();
	
	DefaultMaterialTexture = nullptr;
	ErrorTexture = nullptr;
	
	bIsInitialized = false;
}

TSharedPtr<FSlateBrush> FMaterialVaultThumbnailManager::GetMaterialThumbnail(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem, int32 ThumbnailSize)
{
	if (!MaterialItem.IsValid() || !bIsInitialized)
	{
		return nullptr;
	}
	
	FString CacheKey = GetCacheKey(MaterialItem->AssetData.GetObjectPathString(), ThumbnailSize);
	
	// Check cache first
	if (ThumbnailCache.Contains(CacheKey))
	{
		FThumbnailCacheEntry& Entry = ThumbnailCache[CacheKey];
		Entry.LastAccessTime = FDateTime::Now();
		return Entry.Brush;
	}
	
	// Generate thumbnail if not cached
	RequestThumbnail(MaterialItem, ThumbnailSize);
	
	// Return default thumbnail while generating
	if (DefaultMaterialTexture)
	{
		return CreateBrushFromTexture(DefaultMaterialTexture, ThumbnailSize);
	}
	
	return nullptr;
}

void FMaterialVaultThumbnailManager::RequestThumbnail(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem, int32 ThumbnailSize)
{
	if (!MaterialItem.IsValid() || !bIsInitialized)
	{
		return;
	}
	
	FString MaterialPath = MaterialItem->AssetData.GetObjectPathString();
	
	// Check if already pending
	if (PendingThumbnails.Contains(MaterialPath))
	{
		return;
	}
	
	// Add to pending list
	PendingThumbnails.Add(MaterialPath, MaterialItem);
	
	// Generate thumbnail asynchronously
	LoadThumbnailAsync(MaterialItem, ThumbnailSize);
}

void FMaterialVaultThumbnailManager::ClearThumbnailCache()
{
	ThumbnailCache.Empty();
}

void FMaterialVaultThumbnailManager::ClearThumbnailForMaterial(const FString& MaterialPath)
{
	for (auto It = ThumbnailCache.CreateIterator(); It; ++It)
	{
		if (It.Key().Contains(MaterialPath))
		{
			It.RemoveCurrent();
		}
	}
}

UTexture2D* FMaterialVaultThumbnailManager::GenerateMaterialThumbnail(UMaterialInterface* Material, int32 ThumbnailSize)
{
	if (!Material || !bIsInitialized)
	{
		return GetDefaultMaterialThumbnail();
	}
	
	// For now, return a default thumbnail
	// In a full implementation, this would render the material to a texture
	return GetDefaultMaterialThumbnail();
}

TSharedPtr<FSlateDynamicImageBrush> FMaterialVaultThumbnailManager::CreateBrushFromTexture(UTexture2D* Texture, int32 ThumbnailSize)
{
	if (!Texture)
	{
		return nullptr;
	}
	
	return MakeShareable(new FSlateDynamicImageBrush(
		Texture,
		FVector2D(ThumbnailSize, ThumbnailSize),
		FName(*FString::Printf(TEXT("MaterialThumbnail_%d"), GetTypeHash(Texture)))
	));
}

void FMaterialVaultThumbnailManager::LoadThumbnailAsync(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem, int32 ThumbnailSize)
{
	if (!MaterialItem.IsValid())
	{
		return;
	}
	
	FString MaterialPath = MaterialItem->AssetData.GetObjectPathString();
	
	// Load material asynchronously
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, MaterialItem, ThumbnailSize, MaterialPath]()
	{
		UMaterialInterface* Material = MaterialItem->MaterialPtr.LoadSynchronous();
		if (Material)
		{
			UTexture2D* Thumbnail = GenerateMaterialThumbnail(Material, ThumbnailSize);
			if (Thumbnail)
			{
				// Switch back to game thread to update UI
				AsyncTask(ENamedThreads::GameThread, [this, MaterialPath, Thumbnail, ThumbnailSize]()
				{
					OnThumbnailGenerated(MaterialPath, Thumbnail, ThumbnailSize);
				});
			}
		}
		
		// Remove from pending list
		PendingThumbnails.Remove(MaterialPath);
	});
}

void FMaterialVaultThumbnailManager::SetThumbnailSize(int32 NewSize)
{
	DefaultThumbnailSize = FMath::Clamp(NewSize, 32, 512);
}

void FMaterialVaultThumbnailManager::TrimCache()
{
	if (ThumbnailCache.Num() <= MaxCacheSize)
	{
		return;
	}
	
	// Sort by last access time and remove oldest entries
	TArray<TPair<FString, FDateTime>> SortedEntries;
	for (const auto& Entry : ThumbnailCache)
	{
		SortedEntries.Add(TPair<FString, FDateTime>(Entry.Key, Entry.Value.LastAccessTime));
	}
	
	SortedEntries.Sort([](const TPair<FString, FDateTime>& A, const TPair<FString, FDateTime>& B)
	{
		return A.Value < B.Value;
	});
	
	// Remove oldest entries
	int32 EntriesToRemove = ThumbnailCache.Num() - MaxCacheSize;
	for (int32 i = 0; i < EntriesToRemove; ++i)
	{
		ThumbnailCache.Remove(SortedEntries[i].Key);
	}
}

FString FMaterialVaultThumbnailManager::GetCacheKey(const FString& MaterialPath, int32 ThumbnailSize) const
{
	return FString::Printf(TEXT("%s_%d"), *MaterialPath, ThumbnailSize);
}

void FMaterialVaultThumbnailManager::OnThumbnailGenerated(const FString& MaterialPath, UTexture2D* Thumbnail, int32 ThumbnailSize)
{
	if (!Thumbnail)
	{
		return;
	}
	
	FString CacheKey = GetCacheKey(MaterialPath, ThumbnailSize);
	
	FThumbnailCacheEntry Entry;
	Entry.Texture = Thumbnail;
	Entry.Brush = CreateBrushFromTexture(Thumbnail, ThumbnailSize);
	Entry.ThumbnailSize = ThumbnailSize;
	Entry.LastAccessTime = FDateTime::Now();
	
	ThumbnailCache.Add(CacheKey, Entry);
	
	// Trim cache if needed
	TrimCache();
}

UTexture2D* FMaterialVaultThumbnailManager::GetDefaultMaterialThumbnail() const
{
	if (DefaultMaterialTexture)
	{
		return DefaultMaterialTexture;
	}
	
	if (ErrorTexture)
	{
		return ErrorTexture;
	}
	
	return nullptr;
} 