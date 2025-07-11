#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STileView.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AssetThumbnail.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "MaterialVaultTypes.h"

class UMaterialVaultManager;

/**
 * Tile widget for material items in grid view
 */
class SMaterialVaultMaterialTile : public STableRow<TSharedPtr<FMaterialVaultMaterialItem>>
{
public:
	SLATE_BEGIN_ARGS(SMaterialVaultMaterialTile) {}
		SLATE_ARGUMENT(TSharedPtr<FMaterialVaultMaterialItem>, MaterialItem)
		SLATE_ARGUMENT(float, ThumbnailSize)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	// SWidget interface
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnMaterialClicked, TSharedPtr<FMaterialVaultMaterialItem>);
	DECLARE_DELEGATE_OneParam(FOnMaterialDoubleClicked, TSharedPtr<FMaterialVaultMaterialItem>);
	
	FOnMaterialClicked OnMaterialLeftClicked;
	FOnMaterialClicked OnMaterialRightClicked;
	FOnMaterialClicked OnMaterialMiddleClicked;
	FOnMaterialDoubleClicked OnMaterialDoubleClicked;

private:
	TSharedPtr<FMaterialVaultMaterialItem> MaterialItem;
	TSharedPtr<FAssetThumbnail> AssetThumbnail;
	float ThumbnailSize;

	// UI helpers
	FText GetMaterialName() const;
	FText GetMaterialTooltip() const;
	EVisibility GetLoadingVisibility() const;
	
	// Thumbnail helpers
	void RefreshThumbnail();
};

/**
 * List row widget for material items in list view
 */
class SMaterialVaultMaterialListItem : public STableRow<TSharedPtr<FMaterialVaultMaterialItem>>
{
public:
	SLATE_BEGIN_ARGS(SMaterialVaultMaterialListItem) {}
		SLATE_ARGUMENT(TSharedPtr<FMaterialVaultMaterialItem>, MaterialItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	// SWidget interface
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnMaterialClicked, TSharedPtr<FMaterialVaultMaterialItem>);
	DECLARE_DELEGATE_OneParam(FOnMaterialDoubleClicked, TSharedPtr<FMaterialVaultMaterialItem>);
	
	FOnMaterialClicked OnMaterialLeftClicked;
	FOnMaterialClicked OnMaterialRightClicked;
	FOnMaterialDoubleClicked OnMaterialDoubleClicked;

private:
	TSharedPtr<FMaterialVaultMaterialItem> MaterialItem;
	TSharedPtr<FAssetThumbnail> AssetThumbnail;

	// UI helpers
	FText GetMaterialName() const;
	FText GetMaterialType() const;
	FText GetMaterialPath() const;
	FText GetMaterialTooltip() const;
};

/**
 * Material grid widget for MaterialVault
 */
class MATERIALVAULT_API SMaterialVaultMaterialGrid : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialVaultMaterialGrid) {}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	// Public interface
	void RefreshGrid();
	void SetMaterials(const TArray<TSharedPtr<FMaterialVaultMaterialItem>>& InMaterials);
	void SetSelectedMaterial(TSharedPtr<FMaterialVaultMaterialItem> Material);
	TSharedPtr<FMaterialVaultMaterialItem> GetSelectedMaterial() const;
	void SetViewMode(EMaterialVaultViewMode InViewMode);
	void SetThumbnailSize(float InThumbnailSize);
	void ClearSelection();
	void SetFolder(const FString& FolderPath);

	// Search and filtering
	void SetFilterText(const FString& FilterText);
	void ApplyFilters();

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnMaterialSelected, TSharedPtr<FMaterialVaultMaterialItem>);
	DECLARE_DELEGATE_OneParam(FOnMaterialDoubleClicked, TSharedPtr<FMaterialVaultMaterialItem>);
	DECLARE_DELEGATE_OneParam(FOnMaterialApplied, TSharedPtr<FMaterialVaultMaterialItem>);
	
	FOnMaterialSelected OnMaterialSelected;
	FOnMaterialDoubleClicked OnMaterialDoubleClicked;
	FOnMaterialApplied OnMaterialApplied;

private:
	// View widgets
	TSharedPtr<STileView<TSharedPtr<FMaterialVaultMaterialItem>>> TileView;
	TSharedPtr<SListView<TSharedPtr<FMaterialVaultMaterialItem>>> ListView;
	TSharedPtr<SBorder> ViewContainer;

	// Data
	TArray<TSharedPtr<FMaterialVaultMaterialItem>> AllMaterials;
	TArray<TSharedPtr<FMaterialVaultMaterialItem>> FilteredMaterials;
	TSharedPtr<FMaterialVaultMaterialItem> SelectedMaterial;

	// Settings
	EMaterialVaultViewMode ViewMode;
	float ThumbnailSize;
	FString CurrentFilterText;

	// Manager reference
	UMaterialVaultManager* MaterialVaultManager;

	// Thumbnail management
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;

	// View creation
	TSharedRef<SWidget> CreateTileView();
	TSharedRef<SWidget> CreateListView();
	void SwitchToViewMode(EMaterialVaultViewMode NewViewMode);

	// Tile view callbacks
	TSharedRef<ITableRow> OnGenerateTileWidget(TSharedPtr<FMaterialVaultMaterialItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnTileSelectionChanged(TSharedPtr<FMaterialVaultMaterialItem> SelectedItem, ESelectInfo::Type SelectInfo);
	
	// List view callbacks
	TSharedRef<ITableRow> OnGenerateListWidget(TSharedPtr<FMaterialVaultMaterialItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnListSelectionChanged(TSharedPtr<FMaterialVaultMaterialItem> SelectedItem, ESelectInfo::Type SelectInfo);

	// Material interaction callbacks
	void OnMaterialLeftClicked(TSharedPtr<FMaterialVaultMaterialItem> Material);
	void OnMaterialRightClicked(TSharedPtr<FMaterialVaultMaterialItem> Material);
	void OnMaterialMiddleClicked(TSharedPtr<FMaterialVaultMaterialItem> Material);
	void OnMaterialDoubleClickedInternal(TSharedPtr<FMaterialVaultMaterialItem> Material);

	// Context menu
	TSharedPtr<SWidget> OnContextMenuOpening();
	void OnApplyMaterial();
	void OnBrowseToMaterial();
	void OnCopyMaterialPath();
	void OnEditMaterialMetadata();

	// Filtering
	void UpdateFilteredMaterials();
	bool DoesItemPassFilter(TSharedPtr<FMaterialVaultMaterialItem> Item) const;

	// Helper functions
	void UpdateSelection(TSharedPtr<FMaterialVaultMaterialItem> NewSelection);
	void ScrollToMaterial(TSharedPtr<FMaterialVaultMaterialItem> Material);
	FText GetStatusText() const;
}; 