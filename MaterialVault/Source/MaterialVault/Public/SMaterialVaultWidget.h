// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SComboBox.h"
#include "MaterialVaultTypes.h"
#include "MaterialVaultManager.h"

class SMaterialVaultFolderTree;
class SMaterialVaultMaterialGrid;
class SMaterialVaultMetadataPanel;
class SMaterialVaultCategoriesPanel;
class SMaterialVaultToolbar;

/**
 * Main MaterialVault widget with three-panel layout
 */
class MATERIALVAULT_API SMaterialVaultWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialVaultWidget)
		{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	// SWidget interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	// Refresh the entire interface
	void RefreshInterface();

	// Settings
	void SetSettings(const FMaterialVaultSettings& NewSettings);
	const FMaterialVaultSettings& GetSettings() const { return CurrentSettings; }

private:
	// UI Components
	TSharedPtr<SMaterialVaultToolbar> ToolbarWidget;
	TSharedPtr<SMaterialVaultFolderTree> FolderTreeWidget;
	TSharedPtr<SMaterialVaultCategoriesPanel> CategoriesWidget;
	TSharedPtr<SMaterialVaultMaterialGrid> MaterialGridWidget;
	TSharedPtr<SMaterialVaultMetadataPanel> MetadataWidget;
	TSharedPtr<SSplitter> MainSplitter;
	TSharedPtr<SSplitter> ContentSplitter;
	TSharedPtr<SButton> FoldersTabButton;
	TSharedPtr<SButton> CategoriesTabButton;

	// Event handlers
	void OnFolderSelected(TSharedPtr<FMaterialVaultFolderNode> SelectedFolder);
	void OnCategorySelected(TSharedPtr<struct FMaterialVaultCategoryItem> SelectedCategory);
	void OnTagSelected(FString SelectedTag); // Handle tag selection
	void OnMaterialSelected(TSharedPtr<FMaterialVaultMaterialItem> SelectedMaterial);
	void OnMaterialDoubleClicked(TSharedPtr<FMaterialVaultMaterialItem> SelectedMaterial);
	void OnMaterialApplied(TSharedPtr<FMaterialVaultMaterialItem> MaterialToApply);
	void OnMetadataChanged(TSharedPtr<FMaterialVaultMaterialItem> ChangedMaterial);
	void OnSettingsChanged(const FMaterialVaultSettings& NewSettings);
	void OnRefreshRequested();

	// Toolbar event handlers
	FReply OnRefreshClicked();
	FReply OnBrowseToFolderClicked();
	void OnViewModeChanged(EMaterialVaultViewMode NewViewMode);
	void OnThumbnailSizeChanged(float NewSize);
	void OnSearchTextChanged(const FText& SearchText);
	void OnSortModeChanged(EMaterialVaultSortMode NewSortMode);

	// Tab event handlers
	FReply OnFoldersTabClicked();
	FReply OnCategoriesTabClicked();
	EVisibility GetFoldersVisibility() const;
	EVisibility GetCategoriesVisibility() const;

	// UI state
	FMaterialVaultSettings CurrentSettings;
	TSharedPtr<FMaterialVaultFolderNode> CurrentSelectedFolder;
	TSharedPtr<struct FMaterialVaultCategoryItem> CurrentSelectedCategory;
	TSharedPtr<FMaterialVaultMaterialItem> CurrentSelectedMaterial;
	FString CurrentSelectedTag; // Currently selected tag for filtering
	FString CurrentSearchText;
	bool bShowFolders = true;

	// Manager reference
	UMaterialVaultManager* MaterialVaultManager;

	// Layout helpers
	TSharedRef<SWidget> CreateToolbar();
	TSharedRef<SWidget> CreateMainContent();
	TSharedRef<SWidget> CreateFolderTreePanel();
	TSharedRef<SWidget> CreateMaterialGridPanel();
	TSharedRef<SWidget> CreateMetadataPanel();

	// Utility functions
	void UpdateMaterialGrid();
	void UpdateMaterialGridFromCategory();
	void UpdateMaterialGridFromTag(); // Update grid for tag filtering
	void UpdateMetadataPanel();
	void ApplySettings();
	void SaveSettings();
	void LoadSettings();
}; 