// Copyright Epic Games, Inc. All Rights Reserved.

#include "SMaterialVaultWidget.h"
#include "MaterialVaultManager.h"
#include "SMaterialVaultFolderTree.h"
#include "SMaterialVaultMaterialGrid.h"
#include "SMaterialVaultMetadataPanel.h"
#include "SMaterialVaultCategoriesPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#define LOCTEXT_NAMESPACE "MaterialVaultWidget"

void SMaterialVaultWidget::Construct(const FArguments& InArgs)
{
	// Get the MaterialVault manager
	MaterialVaultManager = GEditor->GetEditorSubsystem<UMaterialVaultManager>();
	
	// Initialize settings
	CurrentSettings = FMaterialVaultSettings();
	
	// Create the main layout
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			CreateToolbar()
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			CreateMainContent()
		]
	];
	
	// Bind to manager events
	if (MaterialVaultManager)
	{
		MaterialVaultManager->OnFolderSelected.AddSP(this, &SMaterialVaultWidget::OnFolderSelected);
		MaterialVaultManager->OnMaterialSelected.AddSP(this, &SMaterialVaultWidget::OnMaterialSelected);
		MaterialVaultManager->OnMaterialDoubleClicked.AddSP(this, &SMaterialVaultWidget::OnMaterialDoubleClicked);
		MaterialVaultManager->OnSettingsChanged.AddSP(this, &SMaterialVaultWidget::OnSettingsChanged);
		MaterialVaultManager->OnRefreshRequested.AddSP(this, &SMaterialVaultWidget::OnRefreshRequested);
	}
	
	// Bind widget events
	if (FolderTreeWidget.IsValid())
	{
		FolderTreeWidget->OnFolderSelected.BindSP(this, &SMaterialVaultWidget::OnFolderSelected);
	}
	
	if (CategoriesWidget.IsValid())
	{
		CategoriesWidget->OnCategorySelected.BindSP(this, &SMaterialVaultWidget::OnCategorySelected);
		CategoriesWidget->OnTagSelected.BindSP(this, &SMaterialVaultWidget::OnTagSelected);
	}
	
	if (MaterialGridWidget.IsValid())
	{
		MaterialGridWidget->OnMaterialSelected.BindSP(this, &SMaterialVaultWidget::OnMaterialSelected);
		MaterialGridWidget->OnMaterialDoubleClicked.BindSP(this, &SMaterialVaultWidget::OnMaterialDoubleClicked);
		MaterialGridWidget->OnMaterialApplied.BindSP(this, &SMaterialVaultWidget::OnMaterialApplied);
	}
	
	if (MetadataWidget.IsValid())
	{
		MetadataWidget->OnMetadataChanged.BindSP(this, &SMaterialVaultWidget::OnMetadataChanged);
	}
	
	// Initial refresh
	RefreshInterface();
}

void SMaterialVaultWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	
	// Update thumbnails or other time-based operations if needed
}

void SMaterialVaultWidget::RefreshInterface()
{
	if (MaterialVaultManager)
	{
		// Store current selections before refreshing
		FString CurrentFolderPath;
		FString CurrentCategoryName;
		FString CurrentTag = CurrentSelectedTag;
		
		if (CurrentSelectedFolder.IsValid())
		{
			CurrentFolderPath = CurrentSelectedFolder->FolderPath;
		}
		
		if (CurrentSelectedCategory.IsValid())
		{
			CurrentCategoryName = CurrentSelectedCategory->CategoryName;
		}
		
		// Refresh the database
		MaterialVaultManager->RefreshMaterialDatabase();
		
		// Restore selections after refresh
		if (!CurrentFolderPath.IsEmpty() && bShowFolders)
		{
			// Find and restore folder selection
			if (FolderTreeWidget.IsValid())
			{
				TSharedPtr<FMaterialVaultFolderNode> RestoredFolder = MaterialVaultManager->FindFolder(CurrentFolderPath);
				if (RestoredFolder.IsValid())
				{
					CurrentSelectedFolder = RestoredFolder;
					FolderTreeWidget->SetSelectedFolder(RestoredFolder);
				}
			}
		}
		else if (!CurrentCategoryName.IsEmpty() && !bShowFolders)
		{
			// Restore category selection
			if (CategoriesWidget.IsValid())
			{
				CategoriesWidget->RefreshCategories();
				// TODO: Add method to restore category selection by name
			}
		}
		else if (!CurrentTag.IsEmpty())
		{
			// Restore tag selection
			CurrentSelectedTag = CurrentTag;
			if (CategoriesWidget.IsValid())
			{
				CategoriesWidget->RefreshTags();
				// TODO: Add method to restore tag selection
			}
		}
		
		// Update the material grid with restored selection
		UpdateMaterialGrid();
	}
}

void SMaterialVaultWidget::SetSettings(const FMaterialVaultSettings& NewSettings)
{
	CurrentSettings = NewSettings;
	ApplySettings();
}

TSharedRef<SWidget> SMaterialVaultWidget::CreateToolbar()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(4.0f)
		[
			SNew(SHorizontalBox)
			
			// Refresh button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton")
				.OnClicked(this, &SMaterialVaultWidget::OnRefreshClicked)
				.ToolTipText(NSLOCTEXT("MaterialVault", "RefreshTooltip", "Refresh material database"))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Refresh"))
				]
			]
			
			// Browse to folder button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton")
				.OnClicked(this, &SMaterialVaultWidget::OnBrowseToFolderClicked)
				.ToolTipText(NSLOCTEXT("MaterialVault", "BrowseToFolderTooltip", "Browse to selected material location"))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.FolderOpen"))
				]
			]
			
			// Search box
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(10.0f, 2.0f)
			[
				SNew(SSearchBox)
				.OnTextChanged(this, &SMaterialVaultWidget::OnSearchTextChanged)
				.HintText(NSLOCTEXT("MaterialVault", "SearchHint", "Search materials..."))
			]
			
			// Thumbnail size slider
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("MaterialVault", "ThumbnailSize", "Size:"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SBox)
					.WidthOverride(100.0f)
					[
						SNew(SSlider)
						.Value(CurrentSettings.ThumbnailSize / 256.0f)
						.OnValueChanged(this, &SMaterialVaultWidget::OnThumbnailSizeChanged)
						.ToolTipText(NSLOCTEXT("MaterialVault", "ThumbnailSizeTooltip", "Adjust thumbnail size"))
					]
				]
			]
		];
}

TSharedRef<SWidget> SMaterialVaultWidget::CreateMainContent()
{
	MainSplitter = SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		.PhysicalSplitterHandleSize(2.0f)
		.ResizeMode(ESplitterResizeMode::FixedPosition);
	
	// Left panel - Folder tree
	MainSplitter->AddSlot()
		.Value(0.25f)
		.MinSize(200.0f)
		[
			CreateFolderTreePanel()
		];
	
	// Right side splitter for materials and metadata
	ContentSplitter = SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		.PhysicalSplitterHandleSize(2.0f)
		.ResizeMode(ESplitterResizeMode::FixedPosition);
	
	// Center panel - Material grid
	ContentSplitter->AddSlot()
		.Value(0.7f)
		.MinSize(400.0f)
		[
			CreateMaterialGridPanel()
		];
	
	// Right panel - Metadata
	ContentSplitter->AddSlot()
		.Value(0.3f)
		.MinSize(250.0f)
		[
			CreateMetadataPanel()
		];
	
	MainSplitter->AddSlot()
		.Value(0.75f)
		.MinSize(650.0f)
		[
			ContentSplitter.ToSharedRef()
		];
	
	return MainSplitter.ToSharedRef();
}

TSharedRef<SWidget> SMaterialVaultWidget::CreateFolderTreePanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				// Tab selector
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.5f)
				[
													SAssignNew(FoldersTabButton, SButton)
				.ButtonStyle(FAppStyle::Get(), bShowFolders ? "PrimaryButton" : "FlatButton")
				.OnClicked(this, &SMaterialVaultWidget::OnFoldersTabClicked)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FoldersTab", "Folders"))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SAssignNew(CategoriesTabButton, SButton)
				.ButtonStyle(FAppStyle::Get(), !bShowFolders ? "PrimaryButton" : "FlatButton")
				.OnClicked(this, &SMaterialVaultWidget::OnCategoriesTabClicked)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CategoriesTab", "Categories"))
				]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				// Content switcher
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SAssignNew(FolderTreeWidget, SMaterialVaultFolderTree)
					.Visibility(this, &SMaterialVaultWidget::GetFoldersVisibility)
				]
				+ SOverlay::Slot()
				[
					SAssignNew(CategoriesWidget, SMaterialVaultCategoriesPanel)
					.Visibility(this, &SMaterialVaultWidget::GetCategoriesVisibility)
				]
			]
		];
}

TSharedRef<SWidget> SMaterialVaultWidget::CreateMaterialGridPanel()
{
	return SAssignNew(MaterialGridWidget, SMaterialVaultMaterialGrid);
}

TSharedRef<SWidget> SMaterialVaultWidget::CreateMetadataPanel()
{
	return SAssignNew(MetadataWidget, SMaterialVaultMetadataPanel);
}

FReply SMaterialVaultWidget::OnRefreshClicked()
{
	RefreshInterface();
	return FReply::Handled();
}

FReply SMaterialVaultWidget::OnBrowseToFolderClicked()
{
	// Browse to the currently selected material in the Content Browser
	if (CurrentSelectedMaterial.IsValid())
	{
		TArray<FAssetData> AssetDataArray;
		AssetDataArray.Add(CurrentSelectedMaterial->AssetData);

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetDataArray);
	}
	return FReply::Handled();
}

void SMaterialVaultWidget::OnViewModeChanged(EMaterialVaultViewMode NewViewMode)
{
	CurrentSettings.ViewMode = NewViewMode;
	ApplySettings();
}

void SMaterialVaultWidget::OnThumbnailSizeChanged(float NewSize)
{
	CurrentSettings.ThumbnailSize = NewSize * 256.0f;
	ApplySettings();
}

void SMaterialVaultWidget::OnSearchTextChanged(const FText& SearchText)
{
	CurrentSearchText = SearchText.ToString();
	UpdateMaterialGrid();
}

void SMaterialVaultWidget::OnSortModeChanged(EMaterialVaultSortMode NewSortMode)
{
	CurrentSettings.SortMode = NewSortMode;
	ApplySettings();
}

void SMaterialVaultWidget::OnFolderSelected(TSharedPtr<FMaterialVaultFolderNode> SelectedFolder)
{
	CurrentSelectedFolder = SelectedFolder;
	CurrentSelectedCategory.Reset(); // Clear category selection when folder is selected
	UpdateMaterialGrid();
}

void SMaterialVaultWidget::OnCategorySelected(TSharedPtr<FMaterialVaultCategoryItem> SelectedCategory)
{
	CurrentSelectedCategory = SelectedCategory;
	CurrentSelectedFolder.Reset(); // Clear folder selection when category is selected
	CurrentSelectedTag.Reset(); // Clear tag selection when category is selected
	UpdateMaterialGridFromCategory();
}

void SMaterialVaultWidget::OnTagSelected(FString SelectedTag)
{
	// Handle tag selection and clear other selections
	CurrentSelectedTag = SelectedTag;
	CurrentSelectedFolder.Reset(); // Clear folder selection when tag is selected
	CurrentSelectedCategory.Reset(); // Clear category selection when tag is selected
	UpdateMaterialGridFromTag();
}

void SMaterialVaultWidget::OnMaterialSelected(TSharedPtr<FMaterialVaultMaterialItem> SelectedMaterial)
{
	CurrentSelectedMaterial = SelectedMaterial;
	UpdateMetadataPanel();
}

void SMaterialVaultWidget::OnMaterialDoubleClicked(TSharedPtr<FMaterialVaultMaterialItem> SelectedMaterial)
{
	// Apply material to selected objects or open material editor
	if (MaterialVaultManager)
	{
		MaterialVaultManager->ApplyMaterialToSelection(SelectedMaterial);
	}
}

void SMaterialVaultWidget::OnMaterialApplied(TSharedPtr<FMaterialVaultMaterialItem> MaterialToApply)
{
	// Apply material to selected objects
	if (MaterialVaultManager && MaterialToApply.IsValid())
	{
		MaterialVaultManager->ApplyMaterialToSelection(MaterialToApply);
	}
}

void SMaterialVaultWidget::OnMetadataChanged(TSharedPtr<FMaterialVaultMaterialItem> ChangedMaterial)
{
	// Refresh the material grid to show updated metadata
	if (MaterialGridWidget.IsValid())
	{
		MaterialGridWidget->RefreshGrid();
	}
}

void SMaterialVaultWidget::OnSettingsChanged(const FMaterialVaultSettings& NewSettings)
{
	CurrentSettings = NewSettings;
}

void SMaterialVaultWidget::OnRefreshRequested()
{
	UpdateMaterialGrid();
}

void SMaterialVaultWidget::UpdateMaterialGrid()
{
	if (MaterialGridWidget.IsValid())
	{
		if (bShowFolders && CurrentSelectedFolder.IsValid())
		{
			FString FolderPath = CurrentSelectedFolder->FolderPath;
			MaterialGridWidget->SetFolder(FolderPath);
			MaterialGridWidget->SetFilterText(CurrentSearchText);
		}
		else if (!bShowFolders && CurrentSelectedCategory.IsValid())
		{
			// For categories, set materials directly
			MaterialGridWidget->SetMaterials(CurrentSelectedCategory->Materials);
			MaterialGridWidget->SetFilterText(CurrentSearchText);
		}
		else
		{
			// Clear selection
			MaterialGridWidget->SetFolder(FString());
			MaterialGridWidget->SetFilterText(CurrentSearchText);
		}
	}
}

void SMaterialVaultWidget::UpdateMaterialGridFromCategory()
{
	if (MaterialGridWidget.IsValid() && CurrentSelectedCategory.IsValid())
	{
		MaterialGridWidget->SetMaterials(CurrentSelectedCategory->Materials);
		MaterialGridWidget->SetFilterText(CurrentSearchText);
	}
}

void SMaterialVaultWidget::UpdateMaterialGridFromTag()
{
	if (MaterialGridWidget.IsValid() && !CurrentSelectedTag.IsEmpty() && MaterialVaultManager)
	{
		// Get materials with the selected tag
		TArray<TSharedPtr<FMaterialVaultMaterialItem>> TaggedMaterials = MaterialVaultManager->FilterMaterialsByTag(CurrentSelectedTag);
		MaterialGridWidget->SetMaterials(TaggedMaterials);
		MaterialGridWidget->SetFilterText(CurrentSearchText);
	}
}

void SMaterialVaultWidget::UpdateMetadataPanel()
{
	if (MetadataWidget.IsValid())
	{
		MetadataWidget->SetMaterialItem(CurrentSelectedMaterial);
	}
}

void SMaterialVaultWidget::ApplySettings()
{
	if (MaterialVaultManager)
	{
		MaterialVaultManager->SetSettings(CurrentSettings);
	}
	
	// Apply settings directly to widgets
	if (MaterialGridWidget.IsValid())
	{
		MaterialGridWidget->SetViewMode(CurrentSettings.ViewMode);
		MaterialGridWidget->SetThumbnailSize(CurrentSettings.ThumbnailSize);
	}
}

void SMaterialVaultWidget::SaveSettings()
{
	// TODO: Save settings to config file
}

void SMaterialVaultWidget::LoadSettings()
{
	// TODO: Load settings from config file
}

FReply SMaterialVaultWidget::OnFoldersTabClicked()
{
	bShowFolders = true;
	CurrentSelectedCategory.Reset();
	
	// Update button styles
	if (FoldersTabButton.IsValid())
	{
		FoldersTabButton->SetButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"));
	}
	if (CategoriesTabButton.IsValid())
	{
		CategoriesTabButton->SetButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton"));
	}
	
	UpdateMaterialGrid();
	return FReply::Handled();
}

FReply SMaterialVaultWidget::OnCategoriesTabClicked()
{
	bShowFolders = false;
	CurrentSelectedFolder.Reset();
	
	// Update button styles
	if (FoldersTabButton.IsValid())
	{
		FoldersTabButton->SetButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton"));
	}
	if (CategoriesTabButton.IsValid())
	{
		CategoriesTabButton->SetButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"));
	}
	
	if (CategoriesWidget.IsValid())
	{
		CategoriesWidget->RefreshCategories();
	}
	UpdateMaterialGrid();
	return FReply::Handled();
}

EVisibility SMaterialVaultWidget::GetFoldersVisibility() const
{
	return bShowFolders ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SMaterialVaultWidget::GetCategoriesVisibility() const
{
	return !bShowFolders ? EVisibility::Visible : EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE