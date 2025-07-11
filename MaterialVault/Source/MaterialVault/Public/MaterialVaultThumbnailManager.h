// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "Slate/SlateGameResources.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "MaterialVaultTypes.h"

/**
 * Manages thumbnail generation and caching for materials
 */
class MATERIALVAULT_API FMaterialVaultThumbnailManager
{
public:
	FMaterialVaultThumbnailManager();
	~FMaterialVaultThumbnailManager();

	// Initialize/cleanup
	void Initialize();
	void Shutdown();

	// Thumbnail operations
	TSharedPtr<FSlateBrush> GetMaterialThumbnail(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem, int32 ThumbnailSize = 128);
	void RequestThumbnail(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem, int32 ThumbnailSize = 128);
	void ClearThumbnailCache();
	void ClearThumbnailForMaterial(const FString& MaterialPath);
	
	// Thumbnail generation
	UTexture2D* GenerateMaterialThumbnail(UMaterialInterface* Material, int32 ThumbnailSize = 128);
	TSharedPtr<FSlateDynamicImageBrush> CreateBrushFromTexture(UTexture2D* Texture, int32 ThumbnailSize = 128);
	
	// Async thumbnail loading
	void LoadThumbnailAsync(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem, int32 ThumbnailSize = 128);
	
	// Settings
	void SetThumbnailSize(int32 NewSize);
	int32 GetThumbnailSize() const { return DefaultThumbnailSize; }
	
	// Cache management
	void SetMaxCacheSize(int32 MaxSize) { MaxCacheSize = MaxSize; }
	int32 GetCacheSize() const { return ThumbnailCache.Num(); }
	void TrimCache();

private:
	// Thumbnail cache
	struct FThumbnailCacheEntry
	{
		TSharedPtr<FSlateDynamicImageBrush> Brush;
		UTexture2D* Texture;
		int32 ThumbnailSize;
		FDateTime LastAccessTime;
		
		FThumbnailCacheEntry()
			: Brush(nullptr)
			, Texture(nullptr)
			, ThumbnailSize(128)
			, LastAccessTime(FDateTime::Now())
		{
		}
	};
	
	TMap<FString, FThumbnailCacheEntry> ThumbnailCache;
	
	// Settings
	int32 DefaultThumbnailSize;
	int32 MaxCacheSize;
	
	// Async loading
	TMap<FString, TSharedPtr<FMaterialVaultMaterialItem>> PendingThumbnails;
	
	// Helper functions
	FString GetCacheKey(const FString& MaterialPath, int32 ThumbnailSize) const;
	void OnThumbnailGenerated(const FString& MaterialPath, UTexture2D* Thumbnail, int32 ThumbnailSize);
	UTexture2D* GetDefaultMaterialThumbnail() const;
	
	// Default textures
	UTexture2D* DefaultMaterialTexture;
	UTexture2D* ErrorTexture;
	
	bool bIsInitialized = false;
}; 