// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SSplitter.h"
#include "MaterialVaultTypes.h"

class UMaterialVaultManager;

/**
 * Category item structure for the category tree
 */
struct FMaterialVaultCategoryItem
{
	FString CategoryName;
	TArray<TSharedPtr<FMaterialVaultMaterialItem>> Materials;
	TArray<TSharedPtr<FMaterialVaultCategoryItem>> Children;
	TSharedPtr<FMaterialVaultCategoryItem> Parent;
	bool bIsExpanded = false;

	FMaterialVaultCategoryItem(const FString& InCategoryName)
		: CategoryName(InCategoryName)
		, Parent(nullptr)
		, bIsExpanded(false)
	{
	}
};

/**
 * Table row widget for category items
 */
class SMaterialVaultCategoryTreeItem : public STableRow<TSharedPtr<FMaterialVaultCategoryItem>>
{
public:
	SLATE_BEGIN_ARGS(SMaterialVaultCategoryTreeItem) {}
		SLATE_ARGUMENT(TSharedPtr<FMaterialVaultCategoryItem>, CategoryItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

private:
	TSharedPtr<FMaterialVaultCategoryItem> CategoryItem;

	FText GetCategoryName() const;
	FText GetMaterialCount() const;
	const FSlateBrush* GetCategoryIcon() const;
};

/**
 * Categories panel widget for MaterialVault
 */
class MATERIALVAULT_API SMaterialVaultCategoriesPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialVaultCategoriesPanel) {}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	// Category management
	void RefreshCategories();
	void SetFilterText(const FString& FilterText);
	
	// Tag management
	void RefreshTags();
	
	// Selection
	TSharedPtr<FMaterialVaultCategoryItem> GetSelectedCategory() const;
	void SetSelectedCategory(TSharedPtr<FMaterialVaultCategoryItem> Category);

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnCategorySelected, TSharedPtr<FMaterialVaultCategoryItem>);
	DECLARE_DELEGATE_OneParam(FOnTagSelected, FString);
	
	FOnCategorySelected OnCategorySelected;
	FOnTagSelected OnTagSelected;

private:
	// Tree view
	TSharedPtr<STreeView<TSharedPtr<FMaterialVaultCategoryItem>>> CategoryTreeView;
	
	// Tags view
	TSharedPtr<SListView<TSharedPtr<FString>>> TagsListView;
	
	// Data
	TArray<TSharedPtr<FMaterialVaultCategoryItem>> RootCategories;
	TArray<TSharedPtr<FMaterialVaultCategoryItem>> FilteredCategories;
	TSharedPtr<FMaterialVaultCategoryItem> SelectedCategory;
	
	// Tags data
	TArray<TSharedPtr<FString>> AllTags;
	TArray<TSharedPtr<FString>> FilteredTags;
	
	// Filtering
	FString CurrentFilterText;
	
	// Manager reference
	UMaterialVaultManager* MaterialVaultManager;

	// Tree view callbacks
	TSharedRef<ITableRow> OnGenerateCategoryWidget(TSharedPtr<FMaterialVaultCategoryItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnCategorySelectionChanged(TSharedPtr<FMaterialVaultCategoryItem> SelectedItem, ESelectInfo::Type SelectInfo);
	void OnCategoryExpansionChanged(TSharedPtr<FMaterialVaultCategoryItem> Item, bool bIsExpanded);
	void OnGetCategoryChildren(TSharedPtr<FMaterialVaultCategoryItem> Item, TArray<TSharedPtr<FMaterialVaultCategoryItem>>& OutChildren);
	TSharedPtr<SWidget> OnCategoryContextMenuOpening();

	// Filter callbacks
	void OnFilterTextChanged(const FText& FilterText);

	// Category building
	void BuildCategoryStructure();
	TSharedPtr<FMaterialVaultCategoryItem> GetOrCreateCategory(const FString& CategoryName);
	void AddMaterialToCategory(TSharedPtr<FMaterialVaultMaterialItem> Material, const FString& CategoryName);
	
	// Category operations
	void OnDeleteCategory(TSharedPtr<FMaterialVaultCategoryItem> CategoryToDelete);
	void DeleteCategoryRecursive(TSharedPtr<FMaterialVaultCategoryItem> Category);

	// UI creation
	TSharedRef<SWidget> CreateTagsPanel();
	
	// Tags functionality
	TSharedRef<ITableRow> OnGenerateTagWidget(TSharedPtr<FString> TagItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnTagSelectionChanged(TSharedPtr<FString> SelectedTag, ESelectInfo::Type SelectInfo);
	TSharedPtr<SWidget> OnTagContextMenuOpening();
	void OnDeleteTag(TSharedPtr<FString> TagToDelete);
	
	// Filtering
	void ApplyFilter();
	bool DoesCategoryPassFilter(TSharedPtr<FMaterialVaultCategoryItem> Category) const;
	bool HasFilteredChildren(TSharedPtr<FMaterialVaultCategoryItem> Category) const;
}; 