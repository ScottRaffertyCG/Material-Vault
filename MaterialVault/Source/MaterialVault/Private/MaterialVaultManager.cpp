#include "MaterialVaultManager.h"
#include "MaterialVaultThumbnailManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EditorActorFolders.h"
#include "Selection.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "MaterialVaultManager"

UMaterialVaultManager::UMaterialVaultManager()
	: AssetRegistryModule(nullptr)
	, bIsInitialized(false)
{
}

void UMaterialVaultManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// Initialize asset registry
	AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	if (AssetRegistryModule)
	{
		IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
		AssetRegistry.OnAssetAdded().AddUObject(this, &UMaterialVaultManager::OnAssetAdded);
		AssetRegistry.OnAssetRemoved().AddUObject(this, &UMaterialVaultManager::OnAssetRemoved);
		AssetRegistry.OnAssetRenamed().AddUObject(this, &UMaterialVaultManager::OnAssetRenamed);
		AssetRegistry.OnAssetUpdated().AddUObject(this, &UMaterialVaultManager::OnAssetUpdated);
	}
	
	// Initialize thumbnail manager
	ThumbnailManager = MakeShared<FMaterialVaultThumbnailManager>();
	ThumbnailManager->Initialize();
	
	// Initialize root folder
	RootFolderNode = MakeShared<FMaterialVaultFolderNode>(TEXT("Root"), Settings.RootFolder);
	FolderMap.Add(Settings.RootFolder, RootFolderNode);
	
	// Load initial data
	RefreshMaterialDatabase();
	
	bIsInitialized = true;
}

void UMaterialVaultManager::Deinitialize()
{
	if (AssetRegistryModule)
	{
		IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
		AssetRegistry.OnAssetAdded().RemoveAll(this);
		AssetRegistry.OnAssetRemoved().RemoveAll(this);
		AssetRegistry.OnAssetRenamed().RemoveAll(this);
		AssetRegistry.OnAssetUpdated().RemoveAll(this);
	}
	
	if (ThumbnailManager.IsValid())
	{
		ThumbnailManager->Shutdown();
		ThumbnailManager.Reset();
	}
	
	// Clean up data
	FolderMap.Empty();
	MaterialMap.Empty();
	MetadataCache.Empty();
	RootFolderNode.Reset();
	
	bIsInitialized = false;
	
	Super::Deinitialize();
}

void UMaterialVaultManager::RefreshMaterialDatabase()
{
	if (!bIsInitialized)
	{
		return;
	}
	
	// Clear existing data
	MaterialMap.Empty();
	if (RootFolderNode.IsValid())
	{
		RootFolderNode->Materials.Empty();
		RootFolderNode->Children.Empty();
	}
	
	// Get all material assets
	TArray<FAssetData> MaterialAssets;
	if (AssetRegistryModule)
	{
		IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
		
		// Get materials and material instances
		AssetRegistry.GetAssetsByClass(UMaterial::StaticClass()->GetClassPathName(), MaterialAssets);
		
		TArray<FAssetData> MaterialInstanceAssets;
		AssetRegistry.GetAssetsByClass(UMaterialInstance::StaticClass()->GetClassPathName(), MaterialInstanceAssets);
		MaterialAssets.Append(MaterialInstanceAssets);
		
		AssetRegistry.GetAssetsByClass(UMaterialInstanceConstant::StaticClass()->GetClassPathName(), MaterialInstanceAssets);
		MaterialAssets.Append(MaterialInstanceAssets);
	}
	
	// Process each material
	for (const FAssetData& AssetData : MaterialAssets)
	{
		ProcessMaterialAsset(AssetData);
	}
	
	// Build folder structure
	BuildFolderStructure();
	
	// Broadcast refresh complete
	OnRefreshRequested.Broadcast();
}

void UMaterialVaultManager::BuildFolderStructure()
{
	if (!RootFolderNode.IsValid())
	{
		return;
	}
	
	// Clear existing structure
	RootFolderNode->Children.Empty();
	FolderMap.Empty();
	FolderMap.Add(Settings.RootFolder, RootFolderNode);
	
	// Create main category folders
	TSharedPtr<FMaterialVaultFolderNode> ContentFolder = CreateFolderNode(TEXT("/Game"));
	ContentFolder->FolderName = TEXT("Content");
	TSharedPtr<FMaterialVaultFolderNode> EngineFolder = CreateFolderNode(TEXT("/Engine"));
	EngineFolder->FolderName = TEXT("Engine");
	TSharedPtr<FMaterialVaultFolderNode> PluginFolder = CreateFolderNode(TEXT("/Plugins"));
	PluginFolder->FolderName = TEXT("Plugins");
	
	if (RootFolderNode.IsValid())
	{
		ContentFolder->Parent = RootFolderNode;
		EngineFolder->Parent = RootFolderNode;
		PluginFolder->Parent = RootFolderNode;
		RootFolderNode->Children.Add(ContentFolder);
		RootFolderNode->Children.Add(EngineFolder);
		RootFolderNode->Children.Add(PluginFolder);
	}
	
	FolderMap.Add(TEXT("/Game"), ContentFolder);
	FolderMap.Add(TEXT("/Engine"), EngineFolder);
	FolderMap.Add(TEXT("/Plugins"), PluginFolder);
	
	// Build structure from materials
	for (const auto& MaterialPair : MaterialMap)
	{
		const TSharedPtr<FMaterialVaultMaterialItem>& MaterialItem = MaterialPair.Value;
		if (MaterialItem.IsValid())
		{
			FString PackagePath = MaterialItem->AssetData.PackagePath.ToString();
			FString OrganizedPath = OrganizePackagePath(PackagePath);
			
			// Create folder nodes for this path
			TSharedPtr<FMaterialVaultFolderNode> FolderNode = GetOrCreateFolderNode(OrganizedPath);
			if (FolderNode.IsValid())
			{
				FolderNode->Materials.Add(MaterialItem);
			}
		}
	}
}

void UMaterialVaultManager::LoadMaterialsFromFolder(const FString& FolderPath)
{
	TSharedPtr<FMaterialVaultFolderNode> FolderNode = FindFolder(FolderPath);
	if (FolderNode.IsValid())
	{
		// Load thumbnails for materials in this folder
		for (const auto& MaterialItem : FolderNode->Materials)
		{
			if (MaterialItem.IsValid() && ThumbnailManager.IsValid())
			{
				LoadMaterialThumbnail(MaterialItem);
			}
		}
	}
}

TSharedPtr<FMaterialVaultFolderNode> UMaterialVaultManager::FindFolder(const FString& FolderPath) const
{
	return FolderMap.FindRef(FolderPath);
}

TArray<TSharedPtr<FMaterialVaultFolderNode>> UMaterialVaultManager::GetChildFolders(const FString& FolderPath) const
{
	TSharedPtr<FMaterialVaultFolderNode> FolderNode = FindFolder(FolderPath);
	if (FolderNode.IsValid())
	{
		return FolderNode->Children;
	}
	return TArray<TSharedPtr<FMaterialVaultFolderNode>>();
}

TArray<TSharedPtr<FMaterialVaultMaterialItem>> UMaterialVaultManager::GetMaterialsInFolder(const FString& FolderPath) const
{
	TSharedPtr<FMaterialVaultFolderNode> FolderNode = FindFolder(FolderPath);
	if (FolderNode.IsValid())
	{
		TArray<TSharedPtr<FMaterialVaultMaterialItem>> SortedMaterials = FolderNode->Materials;
		SortMaterials(SortedMaterials);
		return SortedMaterials;
	}
	return TArray<TSharedPtr<FMaterialVaultMaterialItem>>();
}

TSharedPtr<FMaterialVaultMaterialItem> UMaterialVaultManager::GetMaterialByPath(const FString& AssetPath) const
{
	return MaterialMap.FindRef(AssetPath);
}

void UMaterialVaultManager::LoadMaterialThumbnail(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem)
{
	if (MaterialItem.IsValid() && ThumbnailManager.IsValid())
	{
		ThumbnailManager->LoadThumbnailAsync(MaterialItem, (int32)Settings.ThumbnailSize);
	}
}

void UMaterialVaultManager::LoadMaterialDependencies(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem)
{
	if (!MaterialItem.IsValid())
	{
		return;
	}
	
	// Load the material if not already loaded
	UMaterialInterface* Material = MaterialItem->MaterialPtr.LoadSynchronous();
	if (!Material)
	{
		return;
	}
	
	// Get texture dependencies
	TArray<UTexture*> ReferencedTextures;
	Material->GetUsedTextures(ReferencedTextures, EMaterialQualityLevel::Num, true, ERHIFeatureLevel::Num, true);
	
	MaterialItem->TextureDependencies.Empty();
	for (UTexture* Texture : ReferencedTextures)
	{
		if (UTexture2D* Texture2D = Cast<UTexture2D>(Texture))
		{
			MaterialItem->TextureDependencies.Add(Texture2D);
		}
	}
}

void UMaterialVaultManager::ApplyMaterialToSelection(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem)
{
	if (!MaterialItem.IsValid())
	{
		return;
	}
	
	// Get the material interface
	UMaterialInterface* Material = MaterialItem->MaterialPtr.LoadSynchronous();
	if (!Material)
	{
		// Show error notification
		FNotificationInfo Info(LOCTEXT("MaterialLoadFailed", "Failed to load material for application"));
		Info.ExpireDuration = 3.0f;
		Info.bFireAndForget = true;
		Info.Image = FAppStyle::GetBrush("Icons.ErrorWithColor");
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}
	
	// Get selected actors
	TArray<AActor*> SelectedActors;
	if (GEditor && GEditor->GetSelectedActors())
	{
		GEditor->GetSelectedActors()->GetSelectedObjects(SelectedActors);
	}
	
	if (SelectedActors.Num() == 0)
	{
		// Show info notification
		FNotificationInfo Info(LOCTEXT("NoActorsSelected", "No actors selected. Please select actors with mesh components to apply material."));
		Info.ExpireDuration = 3.0f;
		Info.bFireAndForget = true;
		Info.Image = FAppStyle::GetBrush("Icons.Info");
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}
	
	// Start transaction for undo/redo
	FScopedTransaction Transaction(LOCTEXT("ApplyMaterial", "Apply Material"));
	
	int32 ComponentsModified = 0;
	
	// Apply material to all selected actors
	for (AActor* Actor : SelectedActors)
	{
		if (!Actor)
		{
			continue;
		}
		
		// Mark actor for modification
		Actor->Modify();
		
		// Find all static mesh components in the actor
		TArray<UStaticMeshComponent*> StaticMeshComponents;
		Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
		
		for (UStaticMeshComponent* MeshComponent : StaticMeshComponents)
		{
			if (MeshComponent)
			{
				MeshComponent->Modify();
				
				// Apply material to all material slots
				for (int32 MaterialIndex = 0; MaterialIndex < MeshComponent->GetNumMaterials(); ++MaterialIndex)
				{
					MeshComponent->SetMaterial(MaterialIndex, Material);
					ComponentsModified++;
				}
			}
		}
		
		// Also handle skeletal mesh components
		TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
		Actor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
		
		for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
		{
			if (MeshComponent)
			{
				MeshComponent->Modify();
				
				// Apply material to all material slots
				for (int32 MaterialIndex = 0; MaterialIndex < MeshComponent->GetNumMaterials(); ++MaterialIndex)
				{
					MeshComponent->SetMaterial(MaterialIndex, Material);
					ComponentsModified++;
				}
			}
		}
	}
	
	if (ComponentsModified > 0)
	{
		// Mark level as modified
		if (GEditor && GEditor->GetEditorWorldContext().World())
		{
			GEditor->GetEditorWorldContext().World()->MarkPackageDirty();
		}
		
		// Show success notification
		FNotificationInfo Info(FText::Format(LOCTEXT("MaterialApplied", "Applied material '{0}' to {1} component(s)"), 
			FText::FromString(MaterialItem->DisplayName), FText::AsNumber(ComponentsModified)));
		Info.ExpireDuration = 3.0f;
		Info.bFireAndForget = true;
		Info.Image = FAppStyle::GetBrush("Icons.SuccessWithColor");
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	else
	{
		// Show warning notification
		FNotificationInfo Info(LOCTEXT("NoMeshComponentsFound", "No mesh components found on selected actors"));
		Info.ExpireDuration = 3.0f;
		Info.bFireAndForget = true;
		Info.Image = FAppStyle::GetBrush("Icons.Warning");
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void UMaterialVaultManager::SaveMaterialMetadata(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem)
{
	if (!MaterialItem.IsValid())
	{
		return;
	}
	
	// Update cache
	MetadataCache.Add(MaterialItem->AssetData.GetObjectPathString(), MaterialItem->Metadata);
	
	// Save to file
	FString MetadataPath = GetMetadataFilePath(MaterialItem->AssetData);
	
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("MaterialName"), MaterialItem->Metadata.MaterialName);
	JsonObject->SetStringField(TEXT("Location"), MaterialItem->Metadata.Location);
	JsonObject->SetStringField(TEXT("Author"), MaterialItem->Metadata.Author);
	JsonObject->SetStringField(TEXT("LastModified"), MaterialItem->Metadata.LastModified.ToString());
	JsonObject->SetStringField(TEXT("Notes"), MaterialItem->Metadata.Notes);
	JsonObject->SetStringField(TEXT("Category"), MaterialItem->Metadata.Category);
	
	TArray<TSharedPtr<FJsonValue>> TagsArray;
	for (const FString& Tag : MaterialItem->Metadata.Tags)
	{
		TagsArray.Add(MakeShareable(new FJsonValueString(Tag)));
	}
	JsonObject->SetArrayField(TEXT("Tags"), TagsArray);
	
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	
	FFileHelper::SaveStringToFile(OutputString, *MetadataPath);
}

void UMaterialVaultManager::LoadMaterialMetadata(TSharedPtr<FMaterialVaultMaterialItem> MaterialItem)
{
	if (!MaterialItem.IsValid())
	{
		return;
	}
	
	// Check cache first
	FString ObjectPath = MaterialItem->AssetData.GetObjectPathString();
	if (MetadataCache.Contains(ObjectPath))
	{
		MaterialItem->Metadata = MetadataCache[ObjectPath];
		return;
	}
	
	// Load from file
	FString MetadataPath = GetMetadataFilePath(MaterialItem->AssetData);
	
	FString FileContents;
	if (FFileHelper::LoadFileToString(FileContents, *MetadataPath))
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
		
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			MaterialItem->Metadata.MaterialName = JsonObject->GetStringField(TEXT("MaterialName"));
			MaterialItem->Metadata.Location = JsonObject->GetStringField(TEXT("Location"));
			MaterialItem->Metadata.Author = JsonObject->GetStringField(TEXT("Author"));
			MaterialItem->Metadata.Notes = JsonObject->GetStringField(TEXT("Notes"));
			MaterialItem->Metadata.Category = JsonObject->GetStringField(TEXT("Category"));
			
			FString DateString = JsonObject->GetStringField(TEXT("LastModified"));
			FDateTime::Parse(DateString, MaterialItem->Metadata.LastModified);
			
			const TArray<TSharedPtr<FJsonValue>>* TagsArray;
			if (JsonObject->TryGetArrayField(TEXT("Tags"), TagsArray))
			{
				MaterialItem->Metadata.Tags.Empty();
				for (const auto& TagValue : *TagsArray)
				{
					MaterialItem->Metadata.Tags.Add(TagValue->AsString());
				}
			}
			
			// Cache the loaded metadata
			MetadataCache.Add(ObjectPath, MaterialItem->Metadata);
		}
	}
}

void UMaterialVaultManager::SetSettings(const FMaterialVaultSettings& NewSettings)
{
	Settings = NewSettings;
	OnSettingsChanged.Broadcast(Settings);
}

TArray<TSharedPtr<FMaterialVaultMaterialItem>> UMaterialVaultManager::SearchMaterials(const FString& SearchTerm) const
{
	TArray<TSharedPtr<FMaterialVaultMaterialItem>> Results;
	
	if (SearchTerm.IsEmpty())
	{
		return Results;
	}
	
	FString LowerSearchTerm = SearchTerm.ToLower();
	
	for (const auto& MaterialPair : MaterialMap)
	{
		const TSharedPtr<FMaterialVaultMaterialItem>& MaterialItem = MaterialPair.Value;
		if (MaterialItem.IsValid())
		{
			FString MaterialName = MaterialItem->DisplayName.ToLower();
			FString MaterialPath = MaterialItem->AssetData.PackagePath.ToString().ToLower();
			
			if (MaterialName.Contains(LowerSearchTerm) || MaterialPath.Contains(LowerSearchTerm))
			{
				Results.Add(MaterialItem);
			}
		}
	}
	
	SortMaterials(Results);
	return Results;
}

TArray<TSharedPtr<FMaterialVaultMaterialItem>> UMaterialVaultManager::FilterMaterialsByTag(const FString& Tag) const
{
	TArray<TSharedPtr<FMaterialVaultMaterialItem>> Results;
	
	for (const auto& MaterialPair : MaterialMap)
	{
		const TSharedPtr<FMaterialVaultMaterialItem>& MaterialItem = MaterialPair.Value;
		if (MaterialItem.IsValid() && MaterialItem->Metadata.Tags.Contains(Tag))
		{
			Results.Add(MaterialItem);
		}
	}
	
	SortMaterials(Results);
	return Results;
}

void UMaterialVaultManager::OnAssetAdded(const FAssetData& AssetData)
{
	if (AssetData.AssetClassPath == UMaterial::StaticClass()->GetClassPathName() ||
		AssetData.AssetClassPath == UMaterialInstance::StaticClass()->GetClassPathName() ||
		AssetData.AssetClassPath == UMaterialInstanceConstant::StaticClass()->GetClassPathName())
	{
		ProcessMaterialAsset(AssetData);
		BuildFolderStructure();
	}
}

void UMaterialVaultManager::OnAssetRemoved(const FAssetData& AssetData)
{
	RemoveMaterialAsset(AssetData);
	BuildFolderStructure();
}

void UMaterialVaultManager::OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath)
{
	RemoveMaterialAsset(AssetData);
	ProcessMaterialAsset(AssetData);
	BuildFolderStructure();
}

void UMaterialVaultManager::OnAssetUpdated(const FAssetData& AssetData)
{
	if (AssetData.AssetClassPath == UMaterial::StaticClass()->GetClassPathName() ||
		AssetData.AssetClassPath == UMaterialInstance::StaticClass()->GetClassPathName() ||
		AssetData.AssetClassPath == UMaterialInstanceConstant::StaticClass()->GetClassPathName())
	{
		ProcessMaterialAsset(AssetData);
	}
}

void UMaterialVaultManager::ProcessMaterialAsset(const FAssetData& AssetData)
{
	FString ObjectPath = AssetData.GetObjectPathString();
	
	// Create or update material item
	TSharedPtr<FMaterialVaultMaterialItem> MaterialItem = MaterialMap.FindRef(ObjectPath);
	if (!MaterialItem.IsValid())
	{
		MaterialItem = MakeShared<FMaterialVaultMaterialItem>(AssetData);
		MaterialMap.Add(ObjectPath, MaterialItem);
	}
	else
	{
		// Update existing item
		MaterialItem->AssetData = AssetData;
		MaterialItem->MaterialPtr = AssetData.ToSoftObjectPath();
		MaterialItem->DisplayName = AssetData.AssetName.ToString();
	}
	
	// Load metadata
	LoadMaterialMetadata(MaterialItem);
}

void UMaterialVaultManager::RemoveMaterialAsset(const FAssetData& AssetData)
{
	FString ObjectPath = AssetData.GetObjectPathString();
	MaterialMap.Remove(ObjectPath);
	MetadataCache.Remove(ObjectPath);
}

TSharedPtr<FMaterialVaultFolderNode> UMaterialVaultManager::CreateFolderNode(const FString& FolderPath)
{
	if (FolderPath.IsEmpty())
	{
		return nullptr;
	}
	
	FString FolderName = FPaths::GetCleanFilename(FolderPath);
	if (FolderName.IsEmpty())
	{
		FolderName = TEXT("Root");
	}
	
	return MakeShared<FMaterialVaultFolderNode>(FolderName, FolderPath);
}

TSharedPtr<FMaterialVaultFolderNode> UMaterialVaultManager::GetOrCreateFolderNode(const FString& FolderPath)
{
	// Check if folder already exists
	if (FolderMap.Contains(FolderPath))
	{
		return FolderMap[FolderPath];
	}
	
	// Create new folder
	TSharedPtr<FMaterialVaultFolderNode> NewFolder = CreateFolderNode(FolderPath);
	if (!NewFolder.IsValid())
	{
		return nullptr;
	}
	
	// Add to map
	FolderMap.Add(FolderPath, NewFolder);
	
	// Find parent folder
	FString ParentPath = FPaths::GetPath(FolderPath);
	if (!ParentPath.IsEmpty() && ParentPath != FolderPath)
	{
		TSharedPtr<FMaterialVaultFolderNode> ParentFolder = GetOrCreateFolderNode(ParentPath);
		if (ParentFolder.IsValid())
		{
			NewFolder->Parent = ParentFolder;
			ParentFolder->Children.Add(NewFolder);
		}
	}
	else
	{
		// This is a root level folder
		if (RootFolderNode.IsValid())
		{
			NewFolder->Parent = RootFolderNode;
			RootFolderNode->Children.Add(NewFolder);
		}
	}
	
	return NewFolder;
}

void UMaterialVaultManager::SortMaterials(TArray<TSharedPtr<FMaterialVaultMaterialItem>>& Materials) const
{
	switch (Settings.SortMode)
	{
		case EMaterialVaultSortMode::Name:
			Materials.Sort([](const TSharedPtr<FMaterialVaultMaterialItem>& A, const TSharedPtr<FMaterialVaultMaterialItem>& B)
			{
				return A->DisplayName < B->DisplayName;
			});
			break;
		case EMaterialVaultSortMode::DateModified:
			Materials.Sort([](const TSharedPtr<FMaterialVaultMaterialItem>& A, const TSharedPtr<FMaterialVaultMaterialItem>& B)
			{
				return A->Metadata.LastModified > B->Metadata.LastModified;
			});
			break;
		case EMaterialVaultSortMode::Type:
			Materials.Sort([](const TSharedPtr<FMaterialVaultMaterialItem>& A, const TSharedPtr<FMaterialVaultMaterialItem>& B)
			{
				return A->AssetData.AssetClassPath.ToString() < B->AssetData.AssetClassPath.ToString();
			});
			break;
		default:
			break;
	}
}

FString UMaterialVaultManager::GetMetadataFilePath(const FAssetData& AssetData) const
{
	FString ProjectDir = FPaths::ProjectDir();
	FString MetadataDir = FPaths::Combine(ProjectDir, TEXT("Saved"), TEXT("MaterialVault"), TEXT("Metadata"));
	
	FString AssetPath = AssetData.PackageName.ToString();
	AssetPath.RemoveFromStart(TEXT("/Game/"));
	AssetPath.ReplaceInline(TEXT("/"), TEXT("_"));
	
	FString MetadataFileName = FString::Printf(TEXT("%s_%s.json"), *AssetPath, *AssetData.AssetName.ToString());
	
	return FPaths::Combine(MetadataDir, MetadataFileName);
}

FString UMaterialVaultManager::OrganizePackagePath(const FString& PackagePath) const
{
	// Organize package paths into Engine/Content/Plugins structure like Content Browser
	if (PackagePath.StartsWith(TEXT("/Game")))
	{
		// Game content goes to Content folder
		return PackagePath;
	}
	else if (PackagePath.StartsWith(TEXT("/Engine")))
	{
		// Engine content stays in Engine folder
		return PackagePath;
	}
	else
	{
		// Check for plugin patterns - plugins typically start with plugin name
		TArray<FString> PathComponents;
		PackagePath.ParseIntoArray(PathComponents, TEXT("/"), true);
		
		if (PathComponents.Num() > 0)
		{
			FString FirstComponent = PathComponents[0];
			
			// Check if this looks like a plugin (not Engine, not Game, not Script)
			if (!FirstComponent.Equals(TEXT("Engine"), ESearchCase::IgnoreCase) &&
				!FirstComponent.Equals(TEXT("Game"), ESearchCase::IgnoreCase) &&
				!FirstComponent.Equals(TEXT("Script"), ESearchCase::IgnoreCase) &&
				!FirstComponent.Equals(TEXT("Temp"), ESearchCase::IgnoreCase) &&
				!FirstComponent.Equals(TEXT("Memory"), ESearchCase::IgnoreCase))
			{
				// This is likely a plugin
				return FString::Printf(TEXT("/Plugins%s"), *PackagePath);
			}
		}
		
		// Check if it starts with known engine patterns
		if (PackagePath.StartsWith(TEXT("/Script")) || 
			PackagePath.StartsWith(TEXT("/Temp")) ||
			PackagePath.StartsWith(TEXT("/Memory")) ||
			PackagePath.Contains(TEXT("Engine")))
		{
			return FString::Printf(TEXT("/Engine%s"), *PackagePath);
		}
		
		// Unknown content, put in Content by default
		return FString::Printf(TEXT("/Game%s"), *PackagePath);
	}
}

#undef LOCTEXT_NAMESPACE 