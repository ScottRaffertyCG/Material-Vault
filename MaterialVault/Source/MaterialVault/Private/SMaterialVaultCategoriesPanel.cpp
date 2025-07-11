#include "SMaterialVaultCategoriesPanel.h"
#include "MaterialVaultManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSearchBox.h"
#include "Styling/AppStyle.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "MaterialVaultCategoriesPanel"

void SMaterialVaultCategoryTreeItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	CategoryItem = InArgs._CategoryItem;

	STableRow<TSharedPtr<FMaterialVaultCategoryItem>>::Construct(
		STableRow::FArguments()
		.Padding(FMargin(2.0f, 1.0f))
		.Content()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SImage)
				.Image(this, &SMaterialVaultCategoryTreeItem::GetCategoryIcon)
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
							SNew(STextBlock)
			.Text(this, &SMaterialVaultCategoryTreeItem::GetCategoryName)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(6.0f, 0.0f, 2.0f, 0.0f)
			[
							SNew(STextBlock)
			.Text(this, &SMaterialVaultCategoryTreeItem::GetMaterialCount)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		],
		InOwnerTableView
	);
}

FText SMaterialVaultCategoryTreeItem::GetCategoryName() const
{
	if (CategoryItem.IsValid())
	{
		return FText::FromString(CategoryItem->CategoryName);
	}
	return FText::GetEmpty();
}

FText SMaterialVaultCategoryTreeItem::GetMaterialCount() const
{
	if (CategoryItem.IsValid())
	{
		int32 TotalMaterials = CategoryItem->Materials.Num();
		
		// Add materials from child categories
		TFunction<int32(TSharedPtr<FMaterialVaultCategoryItem>)> CountMaterials = [&](TSharedPtr<FMaterialVaultCategoryItem> Category) -> int32
		{
			int32 Count = Category->Materials.Num();
			for (const auto& Child : Category->Children)
			{
				Count += CountMaterials(Child);
			}
			return Count;
		};
		
		TotalMaterials = CountMaterials(CategoryItem);
		
		return FText::AsNumber(TotalMaterials);
	}
	return FText::FromString(TEXT("0"));
}

const FSlateBrush* SMaterialVaultCategoryTreeItem::GetCategoryIcon() const
{
	if (CategoryItem.IsValid())
	{
		// Different icons for different category types
		if (CategoryItem->CategoryName == TEXT("All Materials"))
		{
			return FAppStyle::GetBrush("Icons.Package");
		}
		else if (CategoryItem->Children.Num() > 0)
		{
			return FAppStyle::GetBrush("Icons.Folder");
		}
		else
		{
			return FAppStyle::GetBrush("Icons.Tag");
		}
	}
	return FAppStyle::GetBrush("Icons.Tag");
}

void SMaterialVaultCategoriesPanel::Construct(const FArguments& InArgs)
{
	MaterialVaultManager = GEditor->GetEditorSubsystem<UMaterialVaultManager>();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(0)
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)
			.PhysicalSplitterHandleSize(2.0f)
			
			// Top panel - Categories
			+ SSplitter::Slot()
			.Value(0.6f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4, 4, 4, 2)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CategoriesTitle", "Categories"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4, 2, 4, 4)
				[
					SNew(SSearchBox)
					.OnTextChanged(this, &SMaterialVaultCategoriesPanel::OnFilterTextChanged)
					.HintText(LOCTEXT("FilterCategoriesHint", "Filter categories..."))
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(2)
				[
									SAssignNew(CategoryTreeView, STreeView<TSharedPtr<FMaterialVaultCategoryItem>>)
				.TreeItemsSource(&FilteredCategories)
				.OnGenerateRow(this, &SMaterialVaultCategoriesPanel::OnGenerateCategoryWidget)
				.OnSelectionChanged(this, &SMaterialVaultCategoriesPanel::OnCategorySelectionChanged)
				.OnExpansionChanged(this, &SMaterialVaultCategoriesPanel::OnCategoryExpansionChanged)
				.OnGetChildren(this, &SMaterialVaultCategoriesPanel::OnGetCategoryChildren)
				.OnContextMenuOpening(this, &SMaterialVaultCategoriesPanel::OnCategoryContextMenuOpening)
				.SelectionMode(ESelectionMode::Single)
				.ClearSelectionOnClick(false)
				]
			]
			
			// Bottom panel - Tags
			+ SSplitter::Slot()
			.Value(0.4f)
			[
				CreateTagsPanel()
			]
		]
	];

	RefreshCategories();
	RefreshTags();
}

void SMaterialVaultCategoriesPanel::RefreshCategories()
{
	RootCategories.Empty();
	BuildCategoryStructure();
	ApplyFilter();
	RefreshTags();
}

void SMaterialVaultCategoriesPanel::SetFilterText(const FString& FilterText)
{
	CurrentFilterText = FilterText;
	ApplyFilter();
}

TSharedPtr<FMaterialVaultCategoryItem> SMaterialVaultCategoriesPanel::GetSelectedCategory() const
{
	return SelectedCategory;
}

void SMaterialVaultCategoriesPanel::SetSelectedCategory(TSharedPtr<FMaterialVaultCategoryItem> Category)
{
	SelectedCategory = Category;
	if (CategoryTreeView.IsValid())
	{
		CategoryTreeView->SetSelection(Category);
	}
}

void SMaterialVaultCategoriesPanel::OnFilterTextChanged(const FText& FilterText)
{
	SetFilterText(FilterText.ToString());
}

TSharedRef<ITableRow> SMaterialVaultCategoriesPanel::OnGenerateCategoryWidget(TSharedPtr<FMaterialVaultCategoryItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SMaterialVaultCategoryTreeItem, OwnerTable)
		.CategoryItem(Item);
}

void SMaterialVaultCategoriesPanel::OnCategorySelectionChanged(TSharedPtr<FMaterialVaultCategoryItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
	SelectedCategory = SelectedItem;
	OnCategorySelected.ExecuteIfBound(SelectedItem);
}

void SMaterialVaultCategoriesPanel::OnCategoryExpansionChanged(TSharedPtr<FMaterialVaultCategoryItem> Item, bool bIsExpanded)
{
	if (Item.IsValid())
	{
		Item->bIsExpanded = bIsExpanded;
	}
}

void SMaterialVaultCategoriesPanel::OnGetCategoryChildren(TSharedPtr<FMaterialVaultCategoryItem> Item, TArray<TSharedPtr<FMaterialVaultCategoryItem>>& OutChildren)
{
	if (Item.IsValid())
	{
		OutChildren = Item->Children;
	}
}

TSharedPtr<SWidget> SMaterialVaultCategoriesPanel::OnCategoryContextMenuOpening()
{
	TArray<TSharedPtr<FMaterialVaultCategoryItem>> SelectedCategories;
	if (CategoryTreeView.IsValid())
	{
		SelectedCategories = CategoryTreeView->GetSelectedItems();
	}
	
	if (SelectedCategories.Num() == 1)
	{
		TSharedPtr<FMaterialVaultCategoryItem> CurrentCategory = SelectedCategories[0];
		
		// Don't allow deleting special categories
		if (CurrentCategory->CategoryName == TEXT("All Materials") || CurrentCategory->CategoryName == TEXT("Uncategorized"))
		{
			return SNullWidget::NullWidget;
		}
		
		FMenuBuilder MenuBuilder(true, nullptr);
		
		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SMaterialVaultCategoriesPanel::OnDeleteCategory, CurrentCategory)),
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 4, 0)
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Icons.Delete"))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DeleteCategory", "Delete Category"))
			],
			NAME_None,
			LOCTEXT("DeleteCategoryTooltip", "Delete this category and move its materials to Uncategorized")
		);
		
		return MenuBuilder.MakeWidget();
	}
	
	return SNullWidget::NullWidget;
}

void SMaterialVaultCategoriesPanel::BuildCategoryStructure()
{
	if (!MaterialVaultManager)
	{
		return;
	}

	// Create "All Materials" root category
	TSharedPtr<FMaterialVaultCategoryItem> AllMaterialsCategory = MakeShared<FMaterialVaultCategoryItem>(TEXT("All Materials"));
	RootCategories.Add(AllMaterialsCategory);

	// Get all materials from manager
	TMap<FString, TSharedPtr<FMaterialVaultMaterialItem>> AllMaterials;
	
	// We need to get all materials from all folders
	TSharedPtr<FMaterialVaultFolderNode> RootFolder = MaterialVaultManager->GetRootFolder();
	if (RootFolder.IsValid())
	{
		TFunction<void(TSharedPtr<FMaterialVaultFolderNode>)> CollectMaterials = [&](TSharedPtr<FMaterialVaultFolderNode> Folder)
		{
			if (Folder.IsValid())
			{
				for (const auto& Material : Folder->Materials)
				{
					if (Material.IsValid())
					{
						AllMaterials.Add(Material->AssetData.GetObjectPathString(), Material);
					}
				}
				
				for (const auto& Child : Folder->Children)
				{
					CollectMaterials(Child);
				}
			}
		};
		
		CollectMaterials(RootFolder);
	}

	// Add all materials to "All Materials" category
	for (const auto& MaterialPair : AllMaterials)
	{
		AllMaterialsCategory->Materials.Add(MaterialPair.Value);
	}

	// Create categories based on material metadata
	TMap<FString, TSharedPtr<FMaterialVaultCategoryItem>> CategoryMap;
	
	for (const auto& MaterialPair : AllMaterials)
	{
		TSharedPtr<FMaterialVaultMaterialItem> Material = MaterialPair.Value;
		if (Material.IsValid())
		{
			// Use category from metadata, default to "Uncategorized"
			FString CategoryName = Material->Metadata.Category;
			if (CategoryName.IsEmpty())
			{
				CategoryName = TEXT("Uncategorized");
			}

			AddMaterialToCategory(Material, CategoryName);
		}
	}

	// Sort categories alphabetically
	RootCategories.Sort([](const TSharedPtr<FMaterialVaultCategoryItem>& A, const TSharedPtr<FMaterialVaultCategoryItem>& B)
	{
		if (A->CategoryName == TEXT("All Materials")) return true;
		if (B->CategoryName == TEXT("All Materials")) return false;
		return A->CategoryName < B->CategoryName;
	});
}

TSharedPtr<FMaterialVaultCategoryItem> SMaterialVaultCategoriesPanel::GetOrCreateCategory(const FString& CategoryName)
{
	// Check if category already exists
	for (const auto& Category : RootCategories)
	{
		if (Category->CategoryName == CategoryName)
		{
			return Category;
		}
	}

	// Create new category
	TSharedPtr<FMaterialVaultCategoryItem> NewCategory = MakeShared<FMaterialVaultCategoryItem>(CategoryName);
	RootCategories.Add(NewCategory);
	return NewCategory;
}

void SMaterialVaultCategoriesPanel::AddMaterialToCategory(TSharedPtr<FMaterialVaultMaterialItem> Material, const FString& CategoryName)
{
	TSharedPtr<FMaterialVaultCategoryItem> Category = GetOrCreateCategory(CategoryName);
	if (Category.IsValid())
	{
		Category->Materials.Add(Material);
	}
}

void SMaterialVaultCategoriesPanel::OnDeleteCategory(TSharedPtr<FMaterialVaultCategoryItem> CategoryToDelete)
{
	if (!CategoryToDelete.IsValid())
	{
		return;
	}
	
	// Get or create "Uncategorized" category
	TSharedPtr<FMaterialVaultCategoryItem> UncategorizedCategory = GetOrCreateCategory(TEXT("Uncategorized"));
	
	// Move all materials from the deleted category to "Uncategorized"
	for (const auto& Material : CategoryToDelete->Materials)
	{
		if (Material.IsValid())
		{
			// Update the material's metadata
			Material->Metadata.Category = TEXT("Uncategorized");
			
			// Add to uncategorized (avoid duplicates)
			if (!UncategorizedCategory->Materials.Contains(Material))
			{
				UncategorizedCategory->Materials.Add(Material);
			}
		}
	}
	
	// Remove the category from root categories
	RootCategories.RemoveAll([CategoryToDelete](const TSharedPtr<FMaterialVaultCategoryItem>& Category)
	{
		return Category == CategoryToDelete;
	});
	
	// Clear selection if the deleted category was selected
	if (SelectedCategory == CategoryToDelete)
	{
		SelectedCategory.Reset();
		if (CategoryTreeView.IsValid())
		{
			CategoryTreeView->ClearSelection();
		}
	}
	
	// Refresh the display
	ApplyFilter();
}

void SMaterialVaultCategoriesPanel::ApplyFilter()
{
	FilteredCategories = RootCategories;

	if (!CurrentFilterText.IsEmpty())
	{
		FilteredCategories = RootCategories.FilterByPredicate([this](const TSharedPtr<FMaterialVaultCategoryItem>& Category)
		{
			return DoesCategoryPassFilter(Category) || HasFilteredChildren(Category);
		});
	}

	if (CategoryTreeView.IsValid())
	{
		CategoryTreeView->RequestTreeRefresh();
	}
}

bool SMaterialVaultCategoriesPanel::DoesCategoryPassFilter(TSharedPtr<FMaterialVaultCategoryItem> Category) const
{
	if (!Category.IsValid() || CurrentFilterText.IsEmpty())
	{
		return true;
	}

	FString LowerFilterText = CurrentFilterText.ToLower();
	return Category->CategoryName.ToLower().Contains(LowerFilterText);
}

bool SMaterialVaultCategoriesPanel::HasFilteredChildren(TSharedPtr<FMaterialVaultCategoryItem> Category) const
{
	if (!Category.IsValid())
	{
		return false;
	}

	for (const auto& Child : Category->Children)
	{
		if (DoesCategoryPassFilter(Child) || HasFilteredChildren(Child))
		{
			return true;
		}
		}
	
	return false;
}

TSharedRef<SWidget> SMaterialVaultCategoriesPanel::CreateTagsPanel()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4, 4, 4, 2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TagsTitle", "Tags"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(2)
		[
			SAssignNew(TagsListView, SListView<TSharedPtr<FString>>)
			.ListItemsSource(&FilteredTags)
			.OnGenerateRow(this, &SMaterialVaultCategoriesPanel::OnGenerateTagWidget)
			.OnSelectionChanged(this, &SMaterialVaultCategoriesPanel::OnTagSelectionChanged)
			.OnContextMenuOpening(this, &SMaterialVaultCategoriesPanel::OnTagContextMenuOpening)
			.SelectionMode(ESelectionMode::Single)
			.ClearSelectionOnClick(true)
		];
}

void SMaterialVaultCategoriesPanel::RefreshTags()
{
	if (!MaterialVaultManager)
	{
		return;
	}

	// Clear existing tags
	AllTags.Empty();
	TSet<FString> UniqueTagsSet;

	// Get all materials from all folders
	TSharedPtr<FMaterialVaultFolderNode> RootFolder = MaterialVaultManager->GetRootFolder();
	if (RootFolder.IsValid())
	{
		TFunction<void(TSharedPtr<FMaterialVaultFolderNode>)> CollectTags = [&](TSharedPtr<FMaterialVaultFolderNode> Folder)
		{
			if (Folder.IsValid())
			{
				for (const auto& Material : Folder->Materials)
				{
					if (Material.IsValid())
					{
						// Ensure metadata is loaded
						MaterialVaultManager->LoadMaterialMetadata(Material);
						
						// Add all tags from this material
						for (const FString& TagName : Material->Metadata.Tags)
						{
							if (!TagName.IsEmpty())
							{
								UniqueTagsSet.Add(TagName);
							}
						}
					}
				}
				
				for (const auto& Child : Folder->Children)
				{
					CollectTags(Child);
				}
			}
		};
		
		CollectTags(RootFolder);
	}

	// Convert set to array of shared pointers for tags list
	for (const FString& TagName : UniqueTagsSet)
	{
		AllTags.Add(MakeShareable(new FString(TagName)));
	}

	// Sort tags alphabetically
	AllTags.Sort([](const TSharedPtr<FString>& A, const TSharedPtr<FString>& B)
	{
		return *A < *B;
	});

	// Apply filtering
	FilteredTags = AllTags; // For now, show all tags. Later we can add tag filtering.

	if (TagsListView.IsValid())
	{
		TagsListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SMaterialVaultCategoriesPanel::OnGenerateTagWidget(TSharedPtr<FString> TagItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	// Count materials with this tag
	int32 MaterialCount = 0;
	if (MaterialVaultManager)
	{
		TArray<TSharedPtr<FMaterialVaultMaterialItem>> TaggedMaterials = MaterialVaultManager->FilterMaterialsByTag(*TagItem);
		MaterialCount = TaggedMaterials.Num();
	}

	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		.Padding(FMargin(2.0f, 1.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Icons.Tag"))
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(*TagItem))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(6.0f, 0.0f, 2.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::AsNumber(MaterialCount))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
}

void SMaterialVaultCategoriesPanel::OnTagSelectionChanged(TSharedPtr<FString> SelectedTag, ESelectInfo::Type SelectInfo)
{
	if (SelectedTag.IsValid())
	{
		// Clear category selection when tag is selected
		SelectedCategory.Reset();
		if (CategoryTreeView.IsValid())
		{
			CategoryTreeView->ClearSelection();
		}
		
		// Notify that a tag was selected
		OnTagSelected.ExecuteIfBound(*SelectedTag);
	}
}

TSharedPtr<SWidget> SMaterialVaultCategoriesPanel::OnTagContextMenuOpening()
{
	TArray<TSharedPtr<FString>> SelectedTags = TagsListView->GetSelectedItems();
	if (SelectedTags.Num() != 1)
	{
		return nullptr;
	}

	TSharedPtr<FString> SelectedTag = SelectedTags[0];
	
	FMenuBuilder MenuBuilder(true, nullptr);
	
	MenuBuilder.BeginSection(NAME_None, LOCTEXT("TagActions", "Tag Actions"));
	{
		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SMaterialVaultCategoriesPanel::OnDeleteTag, SelectedTag)),
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 4, 0)
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Icons.Delete"))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DeleteTag", "Delete Tag"))
			],
			NAME_None,
			FText::Format(LOCTEXT("DeleteTagTooltip", "Remove the tag '{0}' from all materials"), FText::FromString(*SelectedTag))
		);
	}
	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget();
}

void SMaterialVaultCategoriesPanel::OnDeleteTag(TSharedPtr<FString> TagToDelete)
{
	if (!TagToDelete.IsValid() || !MaterialVaultManager)
	{
		return;
	}
	
	FString TagName = *TagToDelete;
	
	// Remove the tag from all materials that have it
	TSharedPtr<FMaterialVaultFolderNode> RootFolder = MaterialVaultManager->GetRootFolder();
	if (RootFolder.IsValid())
	{
		TFunction<void(TSharedPtr<FMaterialVaultFolderNode>)> RemoveTagFromMaterials = [&](TSharedPtr<FMaterialVaultFolderNode> Folder)
		{
			if (Folder.IsValid())
			{
				for (const auto& Material : Folder->Materials)
				{
					if (Material.IsValid())
					{
						// Remove the tag if it exists
						int32 RemovedCount = Material->Metadata.Tags.RemoveAll([&TagName](const FString& MaterialTag)
						{
							return MaterialTag == TagName;
						});
						
						// Save metadata if we removed the tag
						if (RemovedCount > 0)
						{
							MaterialVaultManager->SaveMaterialMetadata(Material);
						}
					}
				}
				
				for (const auto& Child : Folder->Children)
				{
					RemoveTagFromMaterials(Child);
				}
			}
		};
		
		RemoveTagFromMaterials(RootFolder);
	}
	
	// Refresh the tags list
	RefreshTags();
	
	// Clear selection since the tag was deleted
	if (TagsListView.IsValid())
	{
		TagsListView->ClearSelection();
	}
}

#undef LOCTEXT_NAMESPACE 