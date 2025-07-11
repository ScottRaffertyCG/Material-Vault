#include "SMaterialVaultMaterialGrid.h"
#include "MaterialVaultManager.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Widgets/Images/SThrobber.h"
#include "AssetThumbnail.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "MaterialVaultMaterialGrid"

void SMaterialVaultMaterialTile::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	MaterialItem = InArgs._MaterialItem;
	ThumbnailSize = InArgs._ThumbnailSize;

	// Create asset thumbnail
	if (MaterialItem.IsValid())
	{
		FAssetThumbnailConfig ThumbnailConfig;
		ThumbnailConfig.bAllowFadeIn = true;
		ThumbnailConfig.bAllowHintText = true;
		ThumbnailConfig.ThumbnailLabel = EThumbnailLabel::ClassName;
		ThumbnailConfig.HighlightedText = FText::GetEmpty();

		AssetThumbnail = MakeShareable(new FAssetThumbnail(MaterialItem->AssetData, ThumbnailSize, ThumbnailSize, UThumbnailManager::Get().GetSharedThumbnailPool()));
	}

	STableRow<TSharedPtr<FMaterialVaultMaterialItem>>::Construct(
		STableRow::FArguments()
		.Style(FAppStyle::Get(), "TableView.Row")
		.Padding(FMargin(2))
		.Content()
		[
			SNew(SBox)
			.Padding(4)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0, 0, 0, 4)
				[
					SNew(SBox)
					.WidthOverride(ThumbnailSize)
					.HeightOverride(ThumbnailSize)
					[
						AssetThumbnail.IsValid() ? AssetThumbnail->MakeThumbnailWidget(FAssetThumbnailConfig()) : SNullWidget::NullWidget
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SMaterialVaultMaterialTile::GetMaterialName)
					.Font(FAppStyle::GetFontStyle("ContentBrowser.AssetTileViewNameFont"))
					.ColorAndOpacity(FSlateColor::UseForeground())
					.ShadowColorAndOpacity(FLinearColor::Black)
					.ShadowOffset(FVector2D(1, 1))
					.Justification(ETextJustify::Center)
					.WrapTextAt(ThumbnailSize)
					.ToolTipText(this, &SMaterialVaultMaterialTile::GetMaterialTooltip)
				]
			]
		],
		InOwnerTableView
	);
}

FReply SMaterialVaultMaterialTile::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnMaterialLeftClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnMaterialRightClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled();
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		OnMaterialMiddleClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled();
	}

	return STableRow<TSharedPtr<FMaterialVaultMaterialItem>>::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SMaterialVaultMaterialTile::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return STableRow<TSharedPtr<FMaterialVaultMaterialItem>>::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SMaterialVaultMaterialTile::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnMaterialDoubleClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled();
	}

	return STableRow<TSharedPtr<FMaterialVaultMaterialItem>>::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}

void SMaterialVaultMaterialTile::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// Visual feedback for drag and drop
}

void SMaterialVaultMaterialTile::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	// Remove visual feedback
}

FReply SMaterialVaultMaterialTile::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MaterialItem.IsValid() && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		// TODO: Implement drag and drop operation
		// This will be enhanced in Phase 3.2
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FText SMaterialVaultMaterialTile::GetMaterialName() const
{
	if (MaterialItem.IsValid())
	{
		return FText::FromString(MaterialItem->DisplayName);
	}
	return FText::GetEmpty();
}

FText SMaterialVaultMaterialTile::GetMaterialTooltip() const
{
	if (MaterialItem.IsValid())
	{
		FString TooltipText = FString::Printf(TEXT("Material: %s\nPath: %s\nType: %s"),
			*MaterialItem->DisplayName,
			*MaterialItem->AssetData.PackageName.ToString(),
			*MaterialItem->AssetData.AssetClassPath.ToString()
		);
		return FText::FromString(TooltipText);
	}
	return FText::GetEmpty();
}

EVisibility SMaterialVaultMaterialTile::GetLoadingVisibility() const
{
	// Show loading indicator if thumbnail is not ready
	return (AssetThumbnail.IsValid() && AssetThumbnail->GetViewportRenderTargetTexture() == nullptr) ? EVisibility::Visible : EVisibility::Collapsed;
}

void SMaterialVaultMaterialTile::RefreshThumbnail()
{
	if (AssetThumbnail.IsValid())
	{
		AssetThumbnail->RefreshThumbnail();
	}
}

void SMaterialVaultMaterialListItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	MaterialItem = InArgs._MaterialItem;

	// Create asset thumbnail for list view (smaller)
	if (MaterialItem.IsValid())
	{
		AssetThumbnail = MakeShareable(new FAssetThumbnail(MaterialItem->AssetData, 32, 32, UThumbnailManager::Get().GetSharedThumbnailPool()));
	}

	STableRow<TSharedPtr<FMaterialVaultMaterialItem>>::Construct(
		STableRow::FArguments()
		.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow")
		.Padding(FMargin(0, 2, 0, 0))
		.Content()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4, 0, 8, 0)
			[
				SNew(SBox)
				.WidthOverride(32)
				.HeightOverride(32)
				[
					AssetThumbnail.IsValid() ? AssetThumbnail->MakeThumbnailWidget() : SNullWidget::NullWidget
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.4f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SMaterialVaultMaterialListItem::GetMaterialName)
				.ToolTipText(this, &SMaterialVaultMaterialListItem::GetMaterialTooltip)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SMaterialVaultMaterialListItem::GetMaterialType)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SMaterialVaultMaterialListItem::GetMaterialPath)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		],
		InOwnerTableView
	);
}

FReply SMaterialVaultMaterialListItem::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnMaterialLeftClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnMaterialRightClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled();
	}

	return STableRow<TSharedPtr<FMaterialVaultMaterialItem>>::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SMaterialVaultMaterialListItem::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnMaterialDoubleClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled();
	}

	return STableRow<TSharedPtr<FMaterialVaultMaterialItem>>::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}

FReply SMaterialVaultMaterialListItem::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MaterialItem.IsValid() && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		// TODO: Implement drag and drop operation
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FText SMaterialVaultMaterialListItem::GetMaterialName() const
{
	if (MaterialItem.IsValid())
	{
		return FText::FromString(MaterialItem->DisplayName);
	}
	return FText::GetEmpty();
}

FText SMaterialVaultMaterialListItem::GetMaterialType() const
{
	if (MaterialItem.IsValid())
	{
		FString ClassName = MaterialItem->AssetData.AssetClassPath.GetAssetName().ToString();
		return FText::FromString(ClassName);
	}
	return FText::GetEmpty();
}

FText SMaterialVaultMaterialListItem::GetMaterialPath() const
{
	if (MaterialItem.IsValid())
	{
		FString Path = MaterialItem->AssetData.PackagePath.ToString();
		Path.RemoveFromStart(TEXT("/Game/"));
		return FText::FromString(Path);
	}
	return FText::GetEmpty();
}

FText SMaterialVaultMaterialListItem::GetMaterialTooltip() const
{
	if (MaterialItem.IsValid())
	{
		FString TooltipText = FString::Printf(TEXT("Material: %s\nPath: %s\nType: %s"),
			*MaterialItem->DisplayName,
			*MaterialItem->AssetData.PackageName.ToString(),
			*MaterialItem->AssetData.AssetClassPath.ToString()
		);
		return FText::FromString(TooltipText);
	}
	return FText::GetEmpty();
}

void SMaterialVaultMaterialGrid::Construct(const FArguments& InArgs)
{
	MaterialVaultManager = GEditor->GetEditorSubsystem<UMaterialVaultManager>();
	ViewMode = EMaterialVaultViewMode::Grid;
	ThumbnailSize = 128.0f;
	CurrentFilterText = TEXT("");

	// Create thumbnail pool
	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(1000, true));

	// Create view container
	ViewContainer = SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(0);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4, 4, 4, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MaterialsTitle", "Materials"))
				.Font(FAppStyle::GetFontStyle("ContentBrowser.SourceTitleFont"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SMaterialVaultMaterialGrid::GetStatusText)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			ViewContainer.ToSharedRef()
		]
	];

	// Initialize with tile view
	SwitchToViewMode(ViewMode);
}

void SMaterialVaultMaterialGrid::RefreshGrid()
{
	UpdateFilteredMaterials();

	if (TileView.IsValid())
	{
		TileView->RequestListRefresh();
	}
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

void SMaterialVaultMaterialGrid::SetMaterials(const TArray<TSharedPtr<FMaterialVaultMaterialItem>>& InMaterials)
{
	AllMaterials = InMaterials;
	UpdateFilteredMaterials();
	RefreshGrid();
}

void SMaterialVaultMaterialGrid::SetSelectedMaterial(TSharedPtr<FMaterialVaultMaterialItem> Material)
{
	UpdateSelection(Material);
}

TSharedPtr<FMaterialVaultMaterialItem> SMaterialVaultMaterialGrid::GetSelectedMaterial() const
{
	return SelectedMaterial;
}

void SMaterialVaultMaterialGrid::SetViewMode(EMaterialVaultViewMode InViewMode)
{
	if (ViewMode != InViewMode)
	{
		ViewMode = InViewMode;
		SwitchToViewMode(ViewMode);
	}
}

void SMaterialVaultMaterialGrid::SetThumbnailSize(float InThumbnailSize)
{
	ThumbnailSize = FMath::Clamp(InThumbnailSize, 32.0f, 512.0f);
	
	// Recreate the view with new thumbnail size
	if (ViewMode == EMaterialVaultViewMode::Grid)
	{
		SwitchToViewMode(EMaterialVaultViewMode::Grid);
	}
}

void SMaterialVaultMaterialGrid::ClearSelection()
{
	UpdateSelection(nullptr);
}

void SMaterialVaultMaterialGrid::SetFolder(const FString& FolderPath)
{
	if (MaterialVaultManager)
	{
		TArray<TSharedPtr<FMaterialVaultMaterialItem>> FolderMaterials = MaterialVaultManager->GetMaterialsInFolder(FolderPath);
		SetMaterials(FolderMaterials);
	}
}

void SMaterialVaultMaterialGrid::SetFilterText(const FString& FilterText)
{
	CurrentFilterText = FilterText;
	ApplyFilters();
}

void SMaterialVaultMaterialGrid::ApplyFilters()
{
	UpdateFilteredMaterials();
	RefreshGrid();
}

TSharedRef<SWidget> SMaterialVaultMaterialGrid::CreateTileView()
{
	TileView = SNew(STileView<TSharedPtr<FMaterialVaultMaterialItem>>)
		.ListItemsSource(&FilteredMaterials)
		.OnGenerateTile(this, &SMaterialVaultMaterialGrid::OnGenerateTileWidget)
		.OnSelectionChanged(this, &SMaterialVaultMaterialGrid::OnTileSelectionChanged)
		.OnContextMenuOpening(this, &SMaterialVaultMaterialGrid::OnContextMenuOpening)
		.ItemWidth(ThumbnailSize + 32)
		.ItemHeight(ThumbnailSize + 48)
		.SelectionMode(ESelectionMode::Single)
		.ClearSelectionOnClick(false);

	return TileView.ToSharedRef();
}

TSharedRef<SWidget> SMaterialVaultMaterialGrid::CreateListView()
{
	ListView = SNew(SListView<TSharedPtr<FMaterialVaultMaterialItem>>)
		.ListItemsSource(&FilteredMaterials)
		.OnGenerateRow(this, &SMaterialVaultMaterialGrid::OnGenerateListWidget)
		.OnSelectionChanged(this, &SMaterialVaultMaterialGrid::OnListSelectionChanged)
		.OnContextMenuOpening(this, &SMaterialVaultMaterialGrid::OnContextMenuOpening)
		.SelectionMode(ESelectionMode::Single)
		.ClearSelectionOnClick(false);

	return ListView.ToSharedRef();
}

void SMaterialVaultMaterialGrid::SwitchToViewMode(EMaterialVaultViewMode NewViewMode)
{
	ViewMode = NewViewMode;

	TSharedRef<SWidget> NewView = (ViewMode == EMaterialVaultViewMode::Grid) ? CreateTileView() : CreateListView();
	
	ViewContainer->SetContent(NewView);
	RefreshGrid();
}

TSharedRef<ITableRow> SMaterialVaultMaterialGrid::OnGenerateTileWidget(TSharedPtr<FMaterialVaultMaterialItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedRef<SMaterialVaultMaterialTile> TileWidget = SNew(SMaterialVaultMaterialTile, OwnerTable)
		.MaterialItem(Item)
		.ThumbnailSize(ThumbnailSize);

	TileWidget->OnMaterialLeftClicked.BindSP(this, &SMaterialVaultMaterialGrid::OnMaterialLeftClicked);
	TileWidget->OnMaterialRightClicked.BindSP(this, &SMaterialVaultMaterialGrid::OnMaterialRightClicked);
	TileWidget->OnMaterialMiddleClicked.BindSP(this, &SMaterialVaultMaterialGrid::OnMaterialMiddleClicked);
	TileWidget->OnMaterialDoubleClicked.BindSP(this, &SMaterialVaultMaterialGrid::OnMaterialDoubleClickedInternal);

	return TileWidget;
}

void SMaterialVaultMaterialGrid::OnTileSelectionChanged(TSharedPtr<FMaterialVaultMaterialItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
	UpdateSelection(SelectedItem);
}

TSharedRef<ITableRow> SMaterialVaultMaterialGrid::OnGenerateListWidget(TSharedPtr<FMaterialVaultMaterialItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedRef<SMaterialVaultMaterialListItem> ListWidget = SNew(SMaterialVaultMaterialListItem, OwnerTable)
		.MaterialItem(Item);

	ListWidget->OnMaterialLeftClicked.BindSP(this, &SMaterialVaultMaterialGrid::OnMaterialLeftClicked);
	ListWidget->OnMaterialRightClicked.BindSP(this, &SMaterialVaultMaterialGrid::OnMaterialRightClicked);
	ListWidget->OnMaterialDoubleClicked.BindSP(this, &SMaterialVaultMaterialGrid::OnMaterialDoubleClickedInternal);

	return ListWidget;
}

void SMaterialVaultMaterialGrid::OnListSelectionChanged(TSharedPtr<FMaterialVaultMaterialItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
	UpdateSelection(SelectedItem);
}

void SMaterialVaultMaterialGrid::OnMaterialLeftClicked(TSharedPtr<FMaterialVaultMaterialItem> Material)
{
	// Left click should select the material
	UpdateSelection(Material);
}

void SMaterialVaultMaterialGrid::OnMaterialRightClicked(TSharedPtr<FMaterialVaultMaterialItem> Material)
{
	// Right click should select the material and show context menu
	UpdateSelection(Material);
	// Context menu will be shown automatically by the STileView/SListView OnContextMenuOpening delegate
}

void SMaterialVaultMaterialGrid::OnMaterialMiddleClicked(TSharedPtr<FMaterialVaultMaterialItem> Material)
{
	// Middle click should show large thumbnail preview (from original MaterialVault behavior)
	UpdateSelection(Material);
	// TODO: Implement large thumbnail preview window
}

void SMaterialVaultMaterialGrid::OnMaterialDoubleClickedInternal(TSharedPtr<FMaterialVaultMaterialItem> Material)
{
	OnMaterialDoubleClicked.ExecuteIfBound(Material);
}

TSharedPtr<SWidget> SMaterialVaultMaterialGrid::OnContextMenuOpening()
{
	if (!SelectedMaterial.IsValid())
	{
		return nullptr;
	}

	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("MaterialActions", "Material Actions"));
	{
		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SMaterialVaultMaterialGrid::OnApplyMaterial)),
			SNew(STextBlock).Text(LOCTEXT("ApplyMaterial", "Apply Material")),
			NAME_None,
			LOCTEXT("ApplyMaterialTooltip", "Apply this material to selected meshes")
		);

		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SMaterialVaultMaterialGrid::OnBrowseToMaterial)),
			SNew(STextBlock).Text(LOCTEXT("BrowseToMaterial", "Browse to Asset")),
			NAME_None,
			LOCTEXT("BrowseToMaterialTooltip", "Browse to this material in the Content Browser")
		);

		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SMaterialVaultMaterialGrid::OnCopyMaterialPath)),
			SNew(STextBlock).Text(LOCTEXT("CopyMaterialPath", "Copy Asset Path")),
			NAME_None,
			LOCTEXT("CopyMaterialPathTooltip", "Copy the asset path to clipboard")
		);

		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SMaterialVaultMaterialGrid::OnEditMaterialMetadata)),
			SNew(STextBlock).Text(LOCTEXT("EditMetadata", "Edit Metadata")),
			NAME_None,
			LOCTEXT("EditMetadataTooltip", "Edit material metadata")
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SMaterialVaultMaterialGrid::OnApplyMaterial()
{
	if (SelectedMaterial.IsValid())
	{
		OnMaterialApplied.ExecuteIfBound(SelectedMaterial);
	}
}

void SMaterialVaultMaterialGrid::OnBrowseToMaterial()
{
	if (SelectedMaterial.IsValid())
	{
		TArray<FAssetData> AssetDataArray;
		AssetDataArray.Add(SelectedMaterial->AssetData);

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetDataArray);
	}
}

void SMaterialVaultMaterialGrid::OnCopyMaterialPath()
{
	if (SelectedMaterial.IsValid())
	{
		FString AssetPath = SelectedMaterial->AssetData.GetObjectPathString();
		FPlatformApplicationMisc::ClipboardCopy(*AssetPath);
	}
}

void SMaterialVaultMaterialGrid::OnEditMaterialMetadata()
{
	// TODO: Open metadata editing dialog
	UE_LOG(LogTemp, Log, TEXT("Edit metadata functionality not yet implemented"));
}

void SMaterialVaultMaterialGrid::UpdateFilteredMaterials()
{
	FilteredMaterials.Empty();

	for (const auto& Material : AllMaterials)
	{
		if (DoesItemPassFilter(Material))
		{
			FilteredMaterials.Add(Material);
		}
	}
}

bool SMaterialVaultMaterialGrid::DoesItemPassFilter(TSharedPtr<FMaterialVaultMaterialItem> Item) const
{
	if (!Item.IsValid())
	{
		return false;
	}

	if (CurrentFilterText.IsEmpty())
	{
		return true;
	}

	// Check name
	if (Item->DisplayName.Contains(CurrentFilterText))
	{
		return true;
	}

	// Check path
	if (Item->AssetData.PackagePath.ToString().Contains(CurrentFilterText))
	{
		return true;
	}

	// Check metadata tags
	for (const FString& ItemTag : Item->Metadata.Tags)
	{
		if (ItemTag.Contains(CurrentFilterText))
		{
			return true;
		}
	}

	return false;
}

void SMaterialVaultMaterialGrid::UpdateSelection(TSharedPtr<FMaterialVaultMaterialItem> NewSelection)
{
	if (SelectedMaterial != NewSelection)
	{
		SelectedMaterial = NewSelection;

		// Update view selection
		if (ViewMode == EMaterialVaultViewMode::Grid && TileView.IsValid())
		{
			TileView->SetSelection(SelectedMaterial);
		}
		else if (ViewMode == EMaterialVaultViewMode::List && ListView.IsValid())
		{
			ListView->SetSelection(SelectedMaterial);
		}

		OnMaterialSelected.ExecuteIfBound(SelectedMaterial);
	}
}

void SMaterialVaultMaterialGrid::ScrollToMaterial(TSharedPtr<FMaterialVaultMaterialItem> Material)
{
	if (Material.IsValid())
	{
		if (ViewMode == EMaterialVaultViewMode::Grid && TileView.IsValid())
		{
			TileView->RequestScrollIntoView(Material);
		}
		else if (ViewMode == EMaterialVaultViewMode::List && ListView.IsValid())
		{
			ListView->RequestScrollIntoView(Material);
		}
	}
}

FText SMaterialVaultMaterialGrid::GetStatusText() const
{
	int32 TotalMaterials = AllMaterials.Num();
	int32 FilteredCount = FilteredMaterials.Num();

	if (CurrentFilterText.IsEmpty())
	{
		return FText::Format(LOCTEXT("MaterialCountFormat", "{0} materials"), FText::AsNumber(TotalMaterials));
	}
	else
	{
		return FText::Format(LOCTEXT("FilteredMaterialCountFormat", "{0} of {1} materials"), 
			FText::AsNumber(FilteredCount), FText::AsNumber(TotalMaterials));
	}
}

#undef LOCTEXT_NAMESPACE 