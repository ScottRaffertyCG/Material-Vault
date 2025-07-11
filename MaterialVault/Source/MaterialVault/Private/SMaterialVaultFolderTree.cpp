#include "SMaterialVaultFolderTree.h"
#include "MaterialVaultManager.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Widgets/Layout/SScrollBox.h"

void SMaterialVaultFolderTreeItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	FolderNode = InArgs._FolderNode;

	STableRow<TSharedPtr<FMaterialVaultFolderNode>>::Construct(
		STableRow::FArguments()
		.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow")
		.Padding(FMargin(0, 2, 0, 0))
		.Content()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 4, 0)
			[
				SNew(SImage)
				.Image(this, &SMaterialVaultFolderTreeItem::GetFolderIcon)
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SMaterialVaultFolderTreeItem::GetFolderText)
				.ColorAndOpacity(this, &SMaterialVaultFolderTreeItem::GetFolderTextColor)
				.ToolTipText(this, &SMaterialVaultFolderTreeItem::GetFolderTooltip)
				.HighlightText_Lambda([this]() { return FText::GetEmpty(); }) // TODO: Add search highlighting
			]
		],
		InOwnerTableView
	);
}

const FSlateBrush* SMaterialVaultFolderTreeItem::GetFolderIcon() const
{
	if (FolderNode.IsValid())
	{
		// Use the same folder icon for all folders to match Engine and Content
		return FAppStyle::GetBrush("ContentBrowser.AssetTreeFolderClosed");
	}
	return FAppStyle::GetBrush("ContentBrowser.AssetTreeFolderClosed");
}

FText SMaterialVaultFolderTreeItem::GetFolderText() const
{
	if (FolderNode.IsValid())
	{
		return FText::FromString(FolderNode->FolderName);
	}
	return FText::GetEmpty();
}

FText SMaterialVaultFolderTreeItem::GetFolderTooltip() const
{
	if (FolderNode.IsValid())
	{
		int32 MaterialCount = FolderNode->Materials.Num();
		int32 SubfolderCount = FolderNode->Children.Num();
		
		FString TooltipText = FString::Printf(TEXT("Path: %s\nMaterials: %d\nSubfolders: %d"), 
			*FolderNode->FolderPath, MaterialCount, SubfolderCount);
		
		return FText::FromString(TooltipText);
	}
	return FText::GetEmpty();
}

FSlateColor SMaterialVaultFolderTreeItem::GetFolderTextColor() const
{
	if (FolderNode.IsValid() && FolderNode->Materials.Num() > 0)
	{
		return FSlateColor::UseForeground();
	}
	return FSlateColor::UseSubduedForeground();
}

void SMaterialVaultFolderTree::Construct(const FArguments& InArgs)
{
	MaterialVaultManager = GEditor->GetEditorSubsystem<UMaterialVaultManager>();
	SelectedFolder = nullptr;
	CurrentFilterText = TEXT("");

	// Create the tree view
	TreeView = SNew(STreeView<TSharedPtr<FMaterialVaultFolderNode>>)
		.TreeItemsSource(&RootNodes)
		.OnGenerateRow(this, &SMaterialVaultFolderTree::OnGenerateRow)
		.OnGetChildren(this, &SMaterialVaultFolderTree::OnGetChildren)
		.OnSelectionChanged(this, &SMaterialVaultFolderTree::OnSelectionChanged)
		.OnExpansionChanged(this, &SMaterialVaultFolderTree::OnExpansionChanged)
		.OnMouseButtonDoubleClick(this, &SMaterialVaultFolderTree::OnFolderDoubleClick)
		.OnContextMenuOpening(this, &SMaterialVaultFolderTree::OnContextMenuOpening)
		.SelectionMode(ESelectionMode::Single)
		.ClearSelectionOnClick(false);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4, 4, 4, 2)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("MaterialVault", "FoldersTitle", "Folders"))
				.Font(FAppStyle::GetFontStyle("ContentBrowser.SourceTitleFont"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2)
			[
				TreeView.ToSharedRef()
			]
		]
	];

	// Bind to manager events
	if (MaterialVaultManager)
	{
		MaterialVaultManager->OnRefreshRequested.AddSP(this, &SMaterialVaultFolderTree::OnManagerRefreshRequested);
	}

	// Initial setup
	RefreshTree();
}

void SMaterialVaultFolderTree::RefreshTree()
{
	// Store expanded folders before refresh
	TSet<FString> ExpandedFolders;
	StoreExpandedFolders(RootNodes, ExpandedFolders);
	
	BuildTreeFromManager();
	TreeView->RequestTreeRefresh();
	
	// Restore expanded folders or expand defaults
	if (ExpandedFolders.Num() > 0)
	{
		RestoreExpandedFolders(RootNodes, ExpandedFolders);
	}
	else
	{
		ExpandDefaultFolders();
	}
}

void SMaterialVaultFolderTree::SetSelectedFolder(TSharedPtr<FMaterialVaultFolderNode> Folder)
{
	if (Folder != SelectedFolder)
	{
		SelectedFolder = Folder;
		TreeView->SetSelection(Folder);
		ScrollToFolder(Folder);
	}
}

TSharedPtr<FMaterialVaultFolderNode> SMaterialVaultFolderTree::GetSelectedFolder() const
{
	return SelectedFolder;
}

void SMaterialVaultFolderTree::ExpandFolder(TSharedPtr<FMaterialVaultFolderNode> Folder)
{
	if (Folder.IsValid())
	{
		TreeView->SetItemExpansion(Folder, true);
		Folder->bIsExpanded = true;
	}
}

void SMaterialVaultFolderTree::CollapseFolder(TSharedPtr<FMaterialVaultFolderNode> Folder)
{
	if (Folder.IsValid())
	{
		TreeView->SetItemExpansion(Folder, false);
		Folder->bIsExpanded = false;
	}
}

TSharedRef<ITableRow> SMaterialVaultFolderTree::OnGenerateRow(TSharedPtr<FMaterialVaultFolderNode> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SMaterialVaultFolderTreeItem, OwnerTable)
		.FolderNode(Item);
}

void SMaterialVaultFolderTree::OnGetChildren(TSharedPtr<FMaterialVaultFolderNode> Item, TArray<TSharedPtr<FMaterialVaultFolderNode>>& OutChildren)
{
	if (Item.IsValid())
	{
		OutChildren = Item->Children;
	}
}

void SMaterialVaultFolderTree::OnSelectionChanged(TSharedPtr<FMaterialVaultFolderNode> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem != SelectedFolder)
	{
		SelectedFolder = SelectedItem;
		OnFolderSelected.ExecuteIfBound(SelectedFolder);
	}
}

void SMaterialVaultFolderTree::OnExpansionChanged(TSharedPtr<FMaterialVaultFolderNode> Item, bool bExpanded)
{
	if (Item.IsValid())
	{
		Item->bIsExpanded = bExpanded;
	}
}

void SMaterialVaultFolderTree::OnFolderDoubleClick(TSharedPtr<FMaterialVaultFolderNode> Item)
{
	if (Item.IsValid())
	{
		// Toggle expansion on double click
		bool bIsExpanded = TreeView->IsItemExpanded(Item);
		TreeView->SetItemExpansion(Item, !bIsExpanded);
	}
}

TSharedPtr<SWidget> SMaterialVaultFolderTree::OnContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection(NAME_None, NSLOCTEXT("MaterialVault", "FolderActions", "Folder Actions"));
	{
		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SMaterialVaultFolderTree::OnRefreshFolder)),
			SNew(STextBlock).Text(NSLOCTEXT("MaterialVault", "RefreshFolder", "Refresh")),
			NAME_None,
			NSLOCTEXT("MaterialVault", "RefreshFolderTooltip", "Refresh this folder and its contents")
		);

		if (SelectedFolder.IsValid())
		{
			MenuBuilder.AddMenuEntry(
				FUIAction(FExecuteAction::CreateSP(this, &SMaterialVaultFolderTree::OnCreateFolder)),
				SNew(STextBlock).Text(NSLOCTEXT("MaterialVault", "CreateFolder", "Create Subfolder")),
				NAME_None,
				NSLOCTEXT("MaterialVault", "CreateFolderTooltip", "Create a new subfolder")
			);

			MenuBuilder.AddMenuEntry(
				FUIAction(FExecuteAction::CreateSP(this, &SMaterialVaultFolderTree::OnRenameFolder)),
				SNew(STextBlock).Text(NSLOCTEXT("MaterialVault", "RenameFolder", "Rename")),
				NAME_None,
				NSLOCTEXT("MaterialVault", "RenameFolderTooltip", "Rename this folder")
			);

			// Only allow delete if folder is empty
			if (SelectedFolder->Materials.Num() == 0 && SelectedFolder->Children.Num() == 0)
			{
				MenuBuilder.AddMenuEntry(
					FUIAction(FExecuteAction::CreateSP(this, &SMaterialVaultFolderTree::OnDeleteFolder)),
					SNew(STextBlock).Text(NSLOCTEXT("MaterialVault", "DeleteFolder", "Delete")),
					NAME_None,
					NSLOCTEXT("MaterialVault", "DeleteFolderTooltip", "Delete this empty folder")
				);
			}
		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SMaterialVaultFolderTree::OnCreateFolder()
{
	// TODO: Implement folder creation
	UE_LOG(LogTemp, Log, TEXT("Create folder functionality not yet implemented"));
}

void SMaterialVaultFolderTree::OnRenameFolder()
{
	// TODO: Implement folder renaming
	UE_LOG(LogTemp, Log, TEXT("Rename folder functionality not yet implemented"));
}

void SMaterialVaultFolderTree::OnDeleteFolder()
{
	// TODO: Implement folder deletion
	UE_LOG(LogTemp, Log, TEXT("Delete folder functionality not yet implemented"));
}

void SMaterialVaultFolderTree::OnRefreshFolder()
{
	// Store current selection before refreshing
	FString CurrentFolderPath;
	if (SelectedFolder.IsValid())
	{
		CurrentFolderPath = SelectedFolder->FolderPath;
	}
	
	RefreshTree();
	
	// Restore selection after refresh
	if (!CurrentFolderPath.IsEmpty() && MaterialVaultManager)
	{
		TSharedPtr<FMaterialVaultFolderNode> RestoredFolder = MaterialVaultManager->FindFolder(CurrentFolderPath);
		if (RestoredFolder.IsValid())
		{
			SetSelectedFolder(RestoredFolder);
		}
	}
}

void SMaterialVaultFolderTree::BuildTreeFromManager()
{
	RootNodes.Empty();
	
	if (!MaterialVaultManager)
	{
		return;
	}

	TSharedPtr<FMaterialVaultFolderNode> RootFolder = MaterialVaultManager->GetRootFolder();
	if (RootFolder.IsValid())
	{
		RootNodes = RootFolder->Children;
	}
}

void SMaterialVaultFolderTree::ExpandDefaultFolders()
{
	// Expand the first level of folders by default
	for (const auto& RootNode : RootNodes)
	{
		if (RootNode.IsValid())
		{
			TreeView->SetItemExpansion(RootNode, true);
			RootNode->bIsExpanded = true;
		}
	}
}

void SMaterialVaultFolderTree::ScrollToFolder(TSharedPtr<FMaterialVaultFolderNode> Folder)
{
	if (Folder.IsValid())
	{
		TreeView->RequestScrollIntoView(Folder);
	}
}

bool SMaterialVaultFolderTree::DoesNodeMatchFilter(TSharedPtr<FMaterialVaultFolderNode> Node, const FString& FilterText) const
{
	if (FilterText.IsEmpty())
	{
		return true;
	}

	if (Node.IsValid())
	{
		// Check if folder name contains filter text
		if (Node->FolderName.Contains(FilterText))
		{
			return true;
		}

		// Check if any child folders match
		for (const auto& Child : Node->Children)
		{
			if (DoesNodeMatchFilter(Child, FilterText))
			{
				return true;
			}
		}

		// Check if any materials in this folder match
		for (const auto& Material : Node->Materials)
		{
			if (Material.IsValid() && Material->DisplayName.Contains(FilterText))
			{
				return true;
			}
		}
	}

	return false;
}

void SMaterialVaultFolderTree::OnManagerRefreshRequested()
{
	// Store current selection before refreshing
	FString CurrentFolderPath;
	if (SelectedFolder.IsValid())
	{
		CurrentFolderPath = SelectedFolder->FolderPath;
	}
	
	RefreshTree();
	
	// Restore selection after refresh
	if (!CurrentFolderPath.IsEmpty() && MaterialVaultManager)
	{
		TSharedPtr<FMaterialVaultFolderNode> RestoredFolder = MaterialVaultManager->FindFolder(CurrentFolderPath);
		if (RestoredFolder.IsValid())
		{
			SetSelectedFolder(RestoredFolder);
		}
	}
}

void SMaterialVaultFolderTree::SetFilterText(const FString& FilterText)
{
	CurrentFilterText = FilterText;
	ApplyFilter();
}

void SMaterialVaultFolderTree::ApplyFilter()
{
	// TODO: Implement filtering logic
	// For now, just refresh the tree
	TreeView->RequestTreeRefresh();
}

void SMaterialVaultFolderTree::StoreExpandedFolders(const TArray<TSharedPtr<FMaterialVaultFolderNode>>& Folders, TSet<FString>& OutExpandedFolders)
{
	for (const auto& Folder : Folders)
	{
		if (Folder.IsValid())
		{
			if (TreeView->IsItemExpanded(Folder))
			{
				OutExpandedFolders.Add(Folder->FolderPath);
			}
			
			// Recursively store child folder expansion states
			StoreExpandedFolders(Folder->Children, OutExpandedFolders);
		}
	}
}

void SMaterialVaultFolderTree::RestoreExpandedFolders(const TArray<TSharedPtr<FMaterialVaultFolderNode>>& Folders, const TSet<FString>& ExpandedFolders)
{
	for (const auto& Folder : Folders)
	{
		if (Folder.IsValid())
		{
			if (ExpandedFolders.Contains(Folder->FolderPath))
			{
				TreeView->SetItemExpansion(Folder, true);
				Folder->bIsExpanded = true;
			}
			
			// Recursively restore child folder expansion states
			RestoreExpandedFolders(Folder->Children, ExpandedFolders);
		}
	}
} 