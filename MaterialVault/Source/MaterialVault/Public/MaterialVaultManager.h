// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
#include "MaterialVaultTypes.h"
#include "EditorSubsystem.h"
#include "MaterialVaultManager.generated.h"

UCLASS()
class MATERIALVAULT_API UMaterialVaultManager : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UMaterialVaultManager();

	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Main functionality
	void RefreshMaterialDatabase();
	void BuildFolderStructure();
	void LoadMaterialsFromFolder(const FString& FolderPath);
	
	// Folder operations
	TSharedPtr<FMaterialVaultFolderNode> GetRootFolder() const { return RootFolderNode; }
	TSharedPtr<FMaterialVaultFolderNode> FindFolder(const FString& FolderPath) const;
	TArray<TSharedPtr<FMaterialVaultFolderNode>> GetChildFolders(const FString& FolderPath) const;
	
	// Material operations
	TArray<TSharedPtr<FMaterialVaultMaterialItem>> GetMaterialsInFolder(const FString& FolderPath) const;
	TSharedPtr<FMaterialVaultMaterialItem> GetMaterialByPath(const FString& AssetPath) const;
	void LoadMaterialThumbnail(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem);
	void LoadMaterialDependencies(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem);
	void ApplyMaterialToSelection(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem);
	
	// Metadata operations
	void SaveMaterialMetadata(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem);
	void LoadMaterialMetadata(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem);
	
	// Settings
	const FMaterialVaultSettings& GetSettings() const { return Settings; }
	void SetSettings(const FMaterialVaultSettings& NewSettings);
	
	// Search and filtering
	TArray<TSharedPtr<FMaterialVaultMaterialItem>> SearchMaterials(const FString& SearchTerm) const;
	TArray<TSharedPtr<FMaterialVaultMaterialItem>> FilterMaterialsByTag(const FString& Tag) const;
	
	// Delegates
	FOnMaterialVaultFolderSelected OnFolderSelected;
	FOnMaterialVaultMaterialSelected OnMaterialSelected;
	FOnMaterialVaultMaterialDoubleClicked OnMaterialDoubleClicked;
	FOnMaterialVaultSettingsChanged OnSettingsChanged;
	FOnMaterialVaultRefreshRequested OnRefreshRequested;

private:
	// Asset registry callbacks
	void OnAssetAdded(const FAssetData& AssetData);
	void OnAssetRemoved(const FAssetData& AssetData);
	void OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath);
	void OnAssetUpdated(const FAssetData& AssetData);
	
	// Internal helpers
	void ProcessMaterialAsset(const FAssetData& AssetData);
	void RemoveMaterialAsset(const FAssetData& AssetData);
	TSharedPtr<FMaterialVaultFolderNode> CreateFolderNode(const FString& FolderPath);
	TSharedPtr<FMaterialVaultFolderNode> GetOrCreateFolderNode(const FString& FolderPath);
	void SortMaterials(TArray<TSharedPtr<FMaterialVaultMaterialItem>>& Materials) const;
	FString GetMetadataFilePath(const FAssetData& AssetData) const;
	FString OrganizePackagePath(const FString& PackagePath) const;
	
	// Data members
	TSharedPtr<FMaterialVaultFolderNode> RootFolderNode;
	TMap<FString, TSharedPtr<FMaterialVaultFolderNode>> FolderMap;
	TMap<FString, TSharedPtr<FMaterialVaultMaterialItem>> MaterialMap;
	
	FMaterialVaultSettings Settings;
	
	// Asset registry
	FAssetRegistryModule* AssetRegistryModule;
	
	// Thumbnail manager
	TSharedPtr<class FMaterialVaultThumbnailManager> ThumbnailManager;
	
	// Metadata cache
	TMap<FString, FMaterialVaultMetadata> MetadataCache;
	
	bool bIsInitialized = false;
}; 