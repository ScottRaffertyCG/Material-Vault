#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "MaterialVaultTypes.h"

class UMaterialVaultManager;

/**
 * Tree row widget for folder items
 */
class SMaterialVaultFolderTreeItem : public STableRow<TSharedPtr<FMaterialVaultFolderNode>>
{
public:
	SLATE_BEGIN_ARGS(SMaterialVaultFolderTreeItem) {}
		SLATE_ARGUMENT(TSharedPtr<FMaterialVaultFolderNode>, FolderNode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

private:
	TSharedPtr<FMaterialVaultFolderNode> FolderNode;
	
	const FSlateBrush* GetFolderIcon() const;
	FText GetFolderText() const;
	FText GetFolderTooltip() const;
	FSlateColor GetFolderTextColor() const;
};

/**
 * Folder tree widget for MaterialVault
 */
class MATERIALVAULT_API SMaterialVaultFolderTree : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialVaultFolderTree) {}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	// Public interface
	void RefreshTree();
	void SetSelectedFolder(TSharedPtr<FMaterialVaultFolderNode> Folder);
	TSharedPtr<FMaterialVaultFolderNode> GetSelectedFolder() const;
	void ExpandFolder(TSharedPtr<FMaterialVaultFolderNode> Folder);
	void CollapseFolder(TSharedPtr<FMaterialVaultFolderNode> Folder);

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnFolderSelected, TSharedPtr<FMaterialVaultFolderNode>);
	FOnFolderSelected OnFolderSelected;

private:
	// Tree view widget
	TSharedPtr<STreeView<TSharedPtr<FMaterialVaultFolderNode>>> TreeView;
	
	// Data
	TArray<TSharedPtr<FMaterialVaultFolderNode>> RootNodes;
	TSharedPtr<FMaterialVaultFolderNode> SelectedFolder;
	
	// Manager reference
	UMaterialVaultManager* MaterialVaultManager;
	
	// Tree view callbacks
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FMaterialVaultFolderNode> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnGetChildren(TSharedPtr<FMaterialVaultFolderNode> Item, TArray<TSharedPtr<FMaterialVaultFolderNode>>& OutChildren);
	void OnSelectionChanged(TSharedPtr<FMaterialVaultFolderNode> SelectedItem, ESelectInfo::Type SelectInfo);
	void OnExpansionChanged(TSharedPtr<FMaterialVaultFolderNode> Item, bool bExpanded);
	void OnFolderDoubleClick(TSharedPtr<FMaterialVaultFolderNode> Item);
	
	// Context menu
	TSharedPtr<SWidget> OnContextMenuOpening();
	void OnCreateFolder();
	void OnRenameFolder();
	void OnDeleteFolder();
	void OnRefreshFolder();
	
	// Helper functions
	void BuildTreeFromManager();
	void ExpandDefaultFolders();
	void ScrollToFolder(TSharedPtr<FMaterialVaultFolderNode> Folder);
	bool DoesNodeMatchFilter(TSharedPtr<FMaterialVaultFolderNode> Node, const FString& FilterText) const;
	void StoreExpandedFolders(const TArray<TSharedPtr<FMaterialVaultFolderNode>>& Folders, TSet<FString>& OutExpandedFolders);
	void RestoreExpandedFolders(const TArray<TSharedPtr<FMaterialVaultFolderNode>>& Folders, const TSet<FString>& ExpandedFolders);
	
	// Manager event handlers
	void OnManagerRefreshRequested();
	
	// Filter support
	FString CurrentFilterText;
	void SetFilterText(const FString& FilterText);
	void ApplyFilter();
}; 