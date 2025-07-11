// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "AssetRegistry/AssetData.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "UObject/SoftObjectPath.h"
#include "MaterialVaultTypes.generated.h"

USTRUCT()
struct MATERIALVAULT_API FMaterialVaultMetadata
{
	GENERATED_BODY()

	UPROPERTY()
	FString MaterialName;

	UPROPERTY()
	FString Location;

	UPROPERTY()
	FString Author;

	UPROPERTY()
	FDateTime LastModified;

	UPROPERTY()
	FString Notes;

	UPROPERTY()
	TArray<FString> Tags;

	UPROPERTY()
	FString Category;

	FMaterialVaultMetadata()
		: MaterialName(TEXT(""))
		, Location(TEXT(""))
		, Author(TEXT(""))
		, LastModified(FDateTime::Now())
		, Notes(TEXT(""))
		, Category(TEXT(""))
	{
	}
};

USTRUCT()
struct MATERIALVAULT_API FMaterialVaultMaterialItem
{
	GENERATED_BODY()

	// Asset data
	UPROPERTY()
	FAssetData AssetData;

	// Soft reference to the material
	UPROPERTY()
	TSoftObjectPtr<UMaterialInterface> MaterialPtr;

	// Cached thumbnail
	TSharedPtr<struct FSlateBrush> ThumbnailBrush;

	// Metadata
	UPROPERTY()
	FMaterialVaultMetadata Metadata;

	// Texture dependencies
	UPROPERTY()
	TArray<TSoftObjectPtr<UTexture2D>> TextureDependencies;

	// Display name
	FString DisplayName;

	// Whether thumbnail is loaded
	bool bThumbnailLoaded = false;

	FMaterialVaultMaterialItem()
		: MaterialPtr(nullptr)
		, ThumbnailBrush(nullptr)
		, bThumbnailLoaded(false)
	{
	}

	FMaterialVaultMaterialItem(const FAssetData& InAssetData)
		: AssetData(InAssetData)
		, MaterialPtr(InAssetData.ToSoftObjectPath())
		, ThumbnailBrush(nullptr)
		, bThumbnailLoaded(false)
	{
		DisplayName = AssetData.AssetName.ToString();
		Metadata.MaterialName = DisplayName;
		Metadata.Location = AssetData.PackageName.ToString();
	}
};

USTRUCT()
struct MATERIALVAULT_API FMaterialVaultFolderNode
{
	GENERATED_BODY()

	// Folder name
	FString FolderName;

	// Full folder path
	FString FolderPath;

	// Parent folder
	TSharedPtr<FMaterialVaultFolderNode> Parent;

	// Child folders
	TArray<TSharedPtr<FMaterialVaultFolderNode>> Children;

	// Materials in this folder
	TArray<TSharedPtr<FMaterialVaultMaterialItem>> Materials;

	// Whether this folder is expanded in the tree
	bool bIsExpanded = false;

	FMaterialVaultFolderNode()
		: FolderName(TEXT(""))
		, FolderPath(TEXT(""))
		, Parent(nullptr)
		, bIsExpanded(false)
	{
	}

	FMaterialVaultFolderNode(const FString& InFolderName, const FString& InFolderPath)
		: FolderName(InFolderName)
		, FolderPath(InFolderPath)
		, Parent(nullptr)
		, bIsExpanded(false)
	{
	}
};

UENUM()
enum class EMaterialVaultViewMode : uint8
{
	Grid,
	List
};

UENUM()
enum class EMaterialVaultSortMode : uint8
{
	Name,
	DateModified,
	Size,
	Type
};

USTRUCT()
struct MATERIALVAULT_API FMaterialVaultSettings
{
	GENERATED_BODY()

	UPROPERTY()
	EMaterialVaultViewMode ViewMode = EMaterialVaultViewMode::Grid;

	UPROPERTY()
	EMaterialVaultSortMode SortMode = EMaterialVaultSortMode::Name;

	UPROPERTY()
	float ThumbnailSize = 128.0f;

	UPROPERTY()
	bool bShowMetadata = true;

	UPROPERTY()
	bool bShowFolderTree = true;

	UPROPERTY()
	FString RootFolder = TEXT("/Game");

	UPROPERTY()
	bool bAutoRefresh = true;

	UPROPERTY()
	float RefreshInterval = 5.0f;
};

// Delegate declarations
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMaterialVaultFolderSelected, TSharedPtr<FMaterialVaultFolderNode>);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMaterialVaultMaterialSelected, TSharedPtr<FMaterialVaultMaterialItem>);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMaterialVaultMaterialDoubleClicked, TSharedPtr<FMaterialVaultMaterialItem>);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMaterialVaultSettingsChanged, const FMaterialVaultSettings&);
DECLARE_MULTICAST_DELEGATE(FOnMaterialVaultRefreshRequested); 