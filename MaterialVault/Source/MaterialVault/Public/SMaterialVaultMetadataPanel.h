#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Images/SImage.h"
#include "AssetThumbnail.h"
#include "MaterialVaultTypes.h"

class UMaterialVaultManager;

/**
 * Wrapper for texture soft object pointer to make it compatible with table rows
 */
struct FMaterialVaultTextureItem
{
	TSoftObjectPtr<UTexture2D> Texture;
	
	FMaterialVaultTextureItem() = default;
	FMaterialVaultTextureItem(TSoftObjectPtr<UTexture2D> InTexture) : Texture(InTexture) {}
};

/**
 * Widget for editing material tags
 */
class SMaterialVaultTagEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialVaultTagEditor) {}
		SLATE_ARGUMENT(TArray<FString>*, Tags)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnTagsChanged, const TArray<FString>&);
	FOnTagsChanged OnTagsChanged;

	// Public methods
	void RefreshTagList();
	void SetTags(TArray<FString>* InTags);

private:
	TArray<FString>* TagsPtr;
	TArray<TSharedPtr<FString>> TagItems;
	TSharedPtr<SListView<TSharedPtr<FString>>> TagListView;
	TSharedPtr<SEditableTextBox> NewTagTextBox;

	// List view callbacks
	TSharedRef<ITableRow> OnGenerateTagWidget(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);

	// Tag operations
	FReply OnAddTag();
	void OnRemoveTag(TSharedPtr<FString> TagToRemove);
	void NotifyTagsChanged();
};

/**
 * Widget for displaying texture dependencies
 */
class SMaterialVaultTextureDependencies : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialVaultTextureDependencies) {}
		SLATE_ARGUMENT(TSharedPtr<FMaterialVaultMaterialItem>, MaterialItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Update the displayed material
	void SetMaterialItem(TSharedPtr<FMaterialVaultMaterialItem> InMaterialItem);

private:
	TSharedPtr<FMaterialVaultMaterialItem> MaterialItem;
	TSharedPtr<SListView<TSharedPtr<FMaterialVaultTextureItem>>> TextureListView;
	TArray<TSharedPtr<FMaterialVaultTextureItem>> TextureDependencies;
	UMaterialVaultManager* MaterialVaultManager;

	// List view callbacks
	TSharedRef<ITableRow> OnGenerateTextureWidget(TSharedPtr<FMaterialVaultTextureItem> Item, const TSharedRef<STableViewBase>& OwnerTable);

	// Operations
	void RefreshTextureDependencies();
	void OnTextureDoubleClicked(TSoftObjectPtr<UTexture2D> Texture);
};

/**
 * Individual texture dependency item widget
 */
class SMaterialVaultTextureItem : public STableRow<TSharedPtr<FMaterialVaultTextureItem>>
{
public:
	SLATE_BEGIN_ARGS(SMaterialVaultTextureItem) {}
		SLATE_ARGUMENT(TSharedPtr<FMaterialVaultTextureItem>, TextureItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnTextureDoubleClicked, TSoftObjectPtr<UTexture2D>);
	FOnTextureDoubleClicked OnTextureDoubleClicked;

	// SWidget interface
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;

private:
	TSharedPtr<FMaterialVaultTextureItem> TextureItem;
	TSharedPtr<FAssetThumbnail> AssetThumbnail;

	// UI helpers
	FText GetTextureName() const;
	FText GetTextureInfo() const;
	FText GetTextureTooltip() const;
};

/**
 * Metadata panel widget for MaterialVault
 */
class MATERIALVAULT_API SMaterialVaultMetadataPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialVaultMetadataPanel) {}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	// Public interface
	void SetMaterialItem(TSharedPtr<FMaterialVaultMaterialItem> InMaterialItem);
	void RefreshMetadata();
	void SaveMetadata();
	bool HasUnsavedChanges() const;

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnMetadataChanged, TSharedPtr<FMaterialVaultMaterialItem>);
	FOnMetadataChanged OnMetadataChanged;

private:
	// Current material
	TSharedPtr<FMaterialVaultMaterialItem> MaterialItem;
	
	// Manager reference
	UMaterialVaultManager* MaterialVaultManager;

	// UI components
	TSharedPtr<SScrollBox> ContentScrollBox;
	TSharedPtr<SEditableTextBox> MaterialNameTextBox;
	TSharedPtr<STextBlock> LocationTextBlock;
	TSharedPtr<SEditableTextBox> AuthorTextBox;
	TSharedPtr<STextBlock> LastModifiedTextBlock;
	TSharedPtr<SEditableTextBox> CategoryTextBox;
	TSharedPtr<SMultiLineEditableTextBox> NotesTextBox;
	TSharedPtr<SMaterialVaultTagEditor> TagEditor;
	TSharedPtr<SMaterialVaultTextureDependencies> TextureDependencies;
	TSharedPtr<SButton> SaveButton;
	TSharedPtr<SButton> RevertButton;

	// State tracking
	bool bHasUnsavedChanges;
	FMaterialVaultMetadata OriginalMetadata;

	// UI creation
	TSharedRef<SWidget> CreateMaterialPreview();
	TSharedRef<SWidget> CreateBasicProperties();
	TSharedRef<SWidget> CreateTagsSection();
	TSharedRef<SWidget> CreateNotesSection();
	TSharedRef<SWidget> CreateTextureDependenciesSection();
	TSharedRef<SWidget> CreateActionButtons();

	// Event handlers
	void OnMaterialNameChanged(const FText& NewText);
	void OnAuthorChanged(const FText& NewText);
	void OnCategoryChanged(const FText& NewText);
	void OnNotesChanged(const FText& NewText);
	void OnTagsChanged(const TArray<FString>& NewTags);

	// Button actions
	FReply OnSaveClicked();
	FReply OnRevertClicked();
	FReply OnBrowseToMaterialClicked();
	FReply OnOpenMaterialEditorClicked();
	
	// Thumbnail operations
	TSharedPtr<SWidget> OnMaterialPreviewContextMenuOpening();
	void OnChangeThumbnail();

	// Helper functions
	void UpdateUI();
	void MarkAsChanged();
	void MarkAsClean();
	
	// Asset operations
	bool RenameAsset(const FString& NewName);
	bool IsEnabled() const;
	FText GetMaterialTypeText() const;
	FText GetMaterialSizeText() const;
	EVisibility GetNoSelectionVisibility() const;
	EVisibility GetContentVisibility() const;
	EVisibility GetSaveButtonVisibility() const;
	FSlateColor GetSaveButtonColor() const;
}; 