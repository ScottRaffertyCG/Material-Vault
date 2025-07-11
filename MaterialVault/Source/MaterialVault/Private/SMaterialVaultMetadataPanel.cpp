// Copyright Epic Games, Inc. All Rights Reserved.

#include "SMaterialVaultMetadataPanel.h"
#include "MaterialVaultManager.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Images/SThrobber.h"
#include "AssetThumbnail.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ToolMenus.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "MaterialVaultMetadataPanel"

void SMaterialVaultTagEditor::Construct(const FArguments& InArgs)
{
	TagsPtr = InArgs._Tags;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(0, 0, 0, 4)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(4)
			[
							SAssignNew(TagListView, SListView<TSharedPtr<FString>>)
			.ListItemsSource(&TagItems)
			.OnGenerateRow(this, &SMaterialVaultTagEditor::OnGenerateTagWidget)
			.SelectionMode(ESelectionMode::None)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0, 0, 4, 0)
			[
				SAssignNew(NewTagTextBox, SEditableTextBox)
				.HintText(LOCTEXT("AddTagHint", "Enter new tag..."))
				.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type CommitType)
				{
					if (CommitType == ETextCommit::OnEnter)
					{
						OnAddTag();
					}
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
				.OnClicked(this, &SMaterialVaultTagEditor::OnAddTag)
				.ToolTipText(LOCTEXT("AddTagTooltip", "Add tag"))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Plus"))
				]
			]
		]
	];

	RefreshTagList();
}

TSharedRef<ITableRow> SMaterialVaultTagEditor::OnGenerateTagWidget(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		.Padding(FMargin(2))
		[
					SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(4, 2))
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(*Item))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4, 0, 0, 0)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
					.ContentPadding(FMargin(2))
					.OnClicked_Lambda([this, Item]()
					{
						OnRemoveTag(Item);
						return FReply::Handled();
					})
					.ToolTipText(LOCTEXT("RemoveTagTooltip", "Remove tag"))
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.X"))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
				]
			]
		];
}

FReply SMaterialVaultTagEditor::OnAddTag()
{
	FString NewTag = NewTagTextBox->GetText().ToString().TrimStartAndEnd();
	if (!NewTag.IsEmpty() && TagsPtr && !TagsPtr->Contains(NewTag))
	{
		TagsPtr->Add(NewTag);
		RefreshTagList();
		NotifyTagsChanged();
		NewTagTextBox->SetText(FText::GetEmpty());
	}
	return FReply::Handled();
}

void SMaterialVaultTagEditor::OnRemoveTag(TSharedPtr<FString> TagToRemove)
{
	if (TagToRemove.IsValid() && TagsPtr)
	{
		TagsPtr->Remove(*TagToRemove);
		RefreshTagList();
		NotifyTagsChanged();
	}
}

void SMaterialVaultTagEditor::RefreshTagList()
{
	TagItems.Empty();
	if (TagsPtr)
	{
			for (const FString& ItemTag : *TagsPtr)
	{
		TagItems.Add(MakeShareable(new FString(ItemTag)));
	}
	}
	
	if (TagListView.IsValid())
	{
		TagListView->RequestListRefresh();
	}
}

void SMaterialVaultTagEditor::SetTags(TArray<FString>* InTags)
{
	TagsPtr = InTags;
	RefreshTagList();
}

void SMaterialVaultTagEditor::NotifyTagsChanged()
{
	if (TagsPtr)
	{
		OnTagsChanged.ExecuteIfBound(*TagsPtr);
	}
}

void SMaterialVaultTextureItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	TextureItem = InArgs._TextureItem;

	// Create asset thumbnail
	if (TextureItem.IsValid() && !TextureItem->Texture.IsNull())
	{
		FAssetData TextureAssetData = FAssetData(TextureItem->Texture.Get());
		AssetThumbnail = MakeShareable(new FAssetThumbnail(TextureAssetData, 32, 32, UThumbnailManager::Get().GetSharedThumbnailPool()));
	}

	STableRow<TSharedPtr<FMaterialVaultTextureItem>>::Construct(
		STableRow::FArguments()
		.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow")
		.Padding(FMargin(0, 1, 0, 1))
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
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SMaterialVaultTextureItem::GetTextureName)
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.ToolTipText(this, &SMaterialVaultTextureItem::GetTextureTooltip)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SMaterialVaultTextureItem::GetTextureInfo)
					.Font(FAppStyle::GetFontStyle("PropertyWindow.SmallFont"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		],
		InOwnerTableView
	);
}

FReply SMaterialVaultTextureItem::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && TextureItem.IsValid())
	{
		OnTextureDoubleClicked.ExecuteIfBound(TextureItem->Texture);
		return FReply::Handled();
	}

	return STableRow<TSharedPtr<FMaterialVaultTextureItem>>::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}

FText SMaterialVaultTextureItem::GetTextureName() const
{
	if (TextureItem.IsValid() && !TextureItem->Texture.IsNull())
	{
		return FText::FromString(TextureItem->Texture.GetAssetName());
	}
	return LOCTEXT("InvalidTexture", "Invalid Texture");
}

FText SMaterialVaultTextureItem::GetTextureInfo() const
{
	if (TextureItem.IsValid() && !TextureItem->Texture.IsNull())
	{
		UTexture2D* LoadedTexture = TextureItem->Texture.LoadSynchronous();
		if (LoadedTexture)
		{
			return FText::Format(LOCTEXT("TextureInfoFormat", "{0}x{1}"), 
				FText::AsNumber(LoadedTexture->GetSizeX()), 
				FText::AsNumber(LoadedTexture->GetSizeY()));
		}
	}
	return FText::GetEmpty();
}

FText SMaterialVaultTextureItem::GetTextureTooltip() const
{
	if (TextureItem.IsValid() && !TextureItem->Texture.IsNull())
	{
		FString TooltipText = FString::Printf(TEXT("Texture: %s\nPath: %s"),
			*TextureItem->Texture.GetAssetName(),
			*TextureItem->Texture.ToString()
		);
		return FText::FromString(TooltipText);
	}
	return FText::GetEmpty();
}

void SMaterialVaultTextureDependencies::Construct(const FArguments& InArgs)
{
	MaterialItem = InArgs._MaterialItem;
	MaterialVaultManager = GEditor->GetEditorSubsystem<UMaterialVaultManager>();

	ChildSlot
	[
			SAssignNew(TextureListView, SListView<TSharedPtr<FMaterialVaultTextureItem>>)
	.ListItemsSource(&TextureDependencies)
	.OnGenerateRow(this, &SMaterialVaultTextureDependencies::OnGenerateTextureWidget)
	.SelectionMode(ESelectionMode::None)
	];

	RefreshTextureDependencies();
}

void SMaterialVaultTextureDependencies::SetMaterialItem(TSharedPtr<FMaterialVaultMaterialItem> InMaterialItem)
{
	MaterialItem = InMaterialItem;
	RefreshTextureDependencies();
}

TSharedRef<ITableRow> SMaterialVaultTextureDependencies::OnGenerateTextureWidget(TSharedPtr<FMaterialVaultTextureItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedRef<SMaterialVaultTextureItem> TextureWidget = SNew(SMaterialVaultTextureItem, OwnerTable)
		.TextureItem(Item);

	TextureWidget->OnTextureDoubleClicked.BindSP(this, &SMaterialVaultTextureDependencies::OnTextureDoubleClicked);

	return TextureWidget;
}

void SMaterialVaultTextureDependencies::RefreshTextureDependencies()
{
	TextureDependencies.Empty();

	if (MaterialItem.IsValid() && MaterialVaultManager)
	{
		// Load dependencies if not already loaded
		if (MaterialItem->TextureDependencies.Num() == 0)
		{
			MaterialVaultManager->LoadMaterialDependencies(MaterialItem);
		}

		// Convert to wrapper items
		for (const auto& TexturePtr : MaterialItem->TextureDependencies)
		{
			TextureDependencies.Add(MakeShareable(new FMaterialVaultTextureItem(TexturePtr)));
		}
	}

	if (TextureListView.IsValid())
	{
		TextureListView->RequestListRefresh();
	}
}

void SMaterialVaultTextureDependencies::OnTextureDoubleClicked(TSoftObjectPtr<UTexture2D> Texture)
{
	if (!Texture.IsNull())
	{
		// Browse to texture in Content Browser
		FAssetData AssetData = FAssetData(Texture.Get());
		TArray<FAssetData> AssetDataArray;
		AssetDataArray.Add(AssetData);

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetDataArray);
	}
}

void SMaterialVaultMetadataPanel::Construct(const FArguments& InArgs)
{
	MaterialVaultManager = GEditor->GetEditorSubsystem<UMaterialVaultManager>();
	bHasUnsavedChanges = false;

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
				.Text(LOCTEXT("MetadataTitle", "Metadata"))
				.Font(FAppStyle::GetFontStyle("ContentBrowser.SourceTitleFont"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					// No selection message
					SNew(SBox)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Visibility(this, &SMaterialVaultMetadataPanel::GetNoSelectionVisibility)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("NoMaterialSelected", "Select a material to view its metadata"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Justification(ETextJustify::Center)
					]
				]
				+ SOverlay::Slot()
				[
					// Content
					SAssignNew(ContentScrollBox, SScrollBox)
					.Visibility(this, &SMaterialVaultMetadataPanel::GetContentVisibility)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 8)
						[
							CreateMaterialPreview()
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 8)
						[
							CreateBasicProperties()
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 8)
						[
							CreateTagsSection()
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 8)
						[
							CreateNotesSection()
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 8)
						[
							CreateTextureDependenciesSection()
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							CreateActionButtons()
						]
					]
				]
			]
		]
	];
}

void SMaterialVaultMetadataPanel::SetMaterialItem(TSharedPtr<FMaterialVaultMaterialItem> InMaterialItem)
{
	// Save current changes if any
	if (bHasUnsavedChanges && MaterialItem.IsValid())
	{
		SaveMetadata();
	}

	MaterialItem = InMaterialItem;
	
	if (MaterialItem.IsValid())
	{
		// Load metadata if not already loaded
		if (MaterialVaultManager)
		{
			MaterialVaultManager->LoadMaterialMetadata(MaterialItem);
		}
		
		OriginalMetadata = MaterialItem->Metadata;
		bHasUnsavedChanges = false;
	}

	UpdateUI();
}

void SMaterialVaultMetadataPanel::RefreshMetadata()
{
	if (MaterialItem.IsValid() && MaterialVaultManager)
	{
		MaterialVaultManager->LoadMaterialMetadata(MaterialItem);
		OriginalMetadata = MaterialItem->Metadata;
		bHasUnsavedChanges = false;
		UpdateUI();
	}
}

void SMaterialVaultMetadataPanel::SaveMetadata()
{
	if (MaterialItem.IsValid() && MaterialVaultManager && bHasUnsavedChanges)
	{
		// Check if material name changed and offer to rename asset
		FString NewName = MaterialItem->Metadata.MaterialName;
		FString CurrentAssetName = MaterialItem->AssetData.AssetName.ToString();
		
		if (!NewName.IsEmpty() && NewName != CurrentAssetName)
		{
			// Store original name in metadata before renaming
			if (OriginalMetadata.MaterialName.IsEmpty())
			{
				MaterialItem->Metadata.MaterialName = CurrentAssetName;
			}
			
			// Try to rename the actual asset
			if (RenameAsset(NewName))
			{
				// If successful, keep the new name in metadata
				MaterialItem->Metadata.MaterialName = NewName;
			}
			else
			{
				// If failed, revert to original asset name but keep user's desired name in metadata
				MaterialItem->Metadata.MaterialName = NewName;
			}
		}
		
		MaterialVaultManager->SaveMaterialMetadata(MaterialItem);
		OriginalMetadata = MaterialItem->Metadata;
		bHasUnsavedChanges = false;
		OnMetadataChanged.ExecuteIfBound(MaterialItem);
	}
}

bool SMaterialVaultMetadataPanel::HasUnsavedChanges() const
{
	return bHasUnsavedChanges;
}

TSharedRef<SWidget> SMaterialVaultMetadataPanel::CreateMaterialPreview()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0, 0, 0, 8)
			[
				SNew(SBox)
				.WidthOverride(256)
				.HeightOverride(256)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ContentBrowser.AssetTileItem.DropShadow"))
					.OnMouseButtonUp(FPointerEventHandler::CreateLambda([this](const FGeometry& Geometry, const FPointerEvent& MouseEvent) -> FReply
					{
						if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
						{
							TSharedPtr<SWidget> ContextMenu = OnMaterialPreviewContextMenuOpening();
							if (ContextMenu.IsValid())
							{
								FSlateApplication::Get().PushMenu(
									AsShared(),
									FWidgetPath(),
									ContextMenu.ToSharedRef(),
									MouseEvent.GetScreenSpacePosition(),
									FPopupTransitionEffect::ContextMenu
								);
								return FReply::Handled();
							}
						}
						return FReply::Unhandled();
					}))
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MaterialPreview", "Material Preview\n(Right-click to change thumbnail)"))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 8, 0)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton")
					.OnClicked(this, &SMaterialVaultMetadataPanel::OnBrowseToMaterialClicked)
					.ToolTipText(LOCTEXT("BrowseToMaterialTooltip", "Browse to material in Content Browser"))
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.FolderOpen"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton")
					.OnClicked(this, &SMaterialVaultMetadataPanel::OnOpenMaterialEditorClicked)
					.ToolTipText(LOCTEXT("OpenMaterialEditorTooltip", "Open material in Material Editor"))
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.Edit"))
					]
				]
			]
		];
}

TSharedRef<SWidget> SMaterialVaultMetadataPanel::CreateBasicProperties()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BasicPropertiesTitle", "Properties"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(0, 2))
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("MaterialNameLabel", "Name:"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SAssignNew(MaterialNameTextBox, SEditableTextBox)
					.IsEnabled(this, &SMaterialVaultMetadataPanel::IsEnabled)
					.OnTextChanged(this, &SMaterialVaultMetadataPanel::OnMaterialNameChanged)
					.ToolTipText(LOCTEXT("MaterialNameTooltip", "Display name for this material (metadata only, does not rename the actual asset)"))
				]
				+ SUniformGridPanel::Slot(0, 1)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("LocationLabel", "Location:"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SUniformGridPanel::Slot(1, 1)
				[
					SAssignNew(LocationTextBlock, STextBlock)
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SUniformGridPanel::Slot(0, 2)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AuthorLabel", "Author:"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SUniformGridPanel::Slot(1, 2)
				[
					SAssignNew(AuthorTextBox, SEditableTextBox)
					.IsEnabled(this, &SMaterialVaultMetadataPanel::IsEnabled)
					.OnTextChanged(this, &SMaterialVaultMetadataPanel::OnAuthorChanged)
				]
				+ SUniformGridPanel::Slot(0, 3)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CategoryLabel", "Category:"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SUniformGridPanel::Slot(1, 3)
				[
					SAssignNew(CategoryTextBox, SEditableTextBox)
					.IsEnabled(this, &SMaterialVaultMetadataPanel::IsEnabled)
					.OnTextChanged(this, &SMaterialVaultMetadataPanel::OnCategoryChanged)
				]
				+ SUniformGridPanel::Slot(0, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("LastModifiedLabel", "Modified:"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SUniformGridPanel::Slot(1, 4)
				[
					SAssignNew(LastModifiedTextBlock, STextBlock)
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		];
}

TSharedRef<SWidget> SMaterialVaultMetadataPanel::CreateTagsSection()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TagsTitle", "Tags"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBox)
				.HeightOverride(120)
				[
					SAssignNew(TagEditor, SMaterialVaultTagEditor)
					.Tags(MaterialItem.IsValid() ? &MaterialItem->Metadata.Tags : nullptr)
				]
			]
		];
}

TSharedRef<SWidget> SMaterialVaultMetadataPanel::CreateNotesSection()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NotesTitle", "Notes"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBox)
				.HeightOverride(80)
				[
					SAssignNew(NotesTextBox, SMultiLineEditableTextBox)
					.IsEnabled(this, &SMaterialVaultMetadataPanel::IsEnabled)
					.OnTextChanged(this, &SMaterialVaultMetadataPanel::OnNotesChanged)
					.WrapTextAt(0)
				]
			]
		];
}

TSharedRef<SWidget> SMaterialVaultMetadataPanel::CreateTextureDependenciesSection()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TextureDependenciesTitle", "Texture Dependencies"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBox)
				.HeightOverride(120)
				[
					SAssignNew(TextureDependencies, SMaterialVaultTextureDependencies)
					.MaterialItem(MaterialItem)
				]
			]
		];
}

TSharedRef<SWidget> SMaterialVaultMetadataPanel::CreateActionButtons()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 4, 0)
			[
				SAssignNew(RevertButton, SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton")
				.OnClicked(this, &SMaterialVaultMetadataPanel::OnRevertClicked)
				.IsEnabled_Lambda([this]() { return bHasUnsavedChanges; })
				.ToolTipText(LOCTEXT("RevertTooltip", "Revert changes"))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RevertButton", "Revert"))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(SaveButton, SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
				.OnClicked(this, &SMaterialVaultMetadataPanel::OnSaveClicked)
				.Visibility(this, &SMaterialVaultMetadataPanel::GetSaveButtonVisibility)
				.ToolTipText(LOCTEXT("SaveTooltip", "Save metadata changes"))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SaveButton", "Save"))
					.ColorAndOpacity(this, &SMaterialVaultMetadataPanel::GetSaveButtonColor)
				]
			]
		];
}

void SMaterialVaultMetadataPanel::OnMaterialNameChanged(const FText& NewText)
{
	if (MaterialItem.IsValid())
	{
		// Just update metadata, don't rename asset until Save is clicked
		MaterialItem->Metadata.MaterialName = NewText.ToString();
		MarkAsChanged();
	}
}

void SMaterialVaultMetadataPanel::OnAuthorChanged(const FText& NewText)
{
	if (MaterialItem.IsValid())
	{
		MaterialItem->Metadata.Author = NewText.ToString();
		MarkAsChanged();
	}
}

void SMaterialVaultMetadataPanel::OnCategoryChanged(const FText& NewText)
{
	if (MaterialItem.IsValid())
	{
		MaterialItem->Metadata.Category = NewText.ToString();
		MarkAsChanged();
	}
}

void SMaterialVaultMetadataPanel::OnNotesChanged(const FText& NewText)
{
	if (MaterialItem.IsValid())
	{
		MaterialItem->Metadata.Notes = NewText.ToString();
		MarkAsChanged();
	}
}

void SMaterialVaultMetadataPanel::OnTagsChanged(const TArray<FString>& NewTags)
{
	if (MaterialItem.IsValid())
	{
		MaterialItem->Metadata.Tags = NewTags;
		MarkAsChanged();
	}
}

FReply SMaterialVaultMetadataPanel::OnSaveClicked()
{
	SaveMetadata();
	return FReply::Handled();
}

FReply SMaterialVaultMetadataPanel::OnRevertClicked()
{
	if (MaterialItem.IsValid())
	{
		MaterialItem->Metadata = OriginalMetadata;
		bHasUnsavedChanges = false;
		UpdateUI();
	}
	return FReply::Handled();
}

FReply SMaterialVaultMetadataPanel::OnBrowseToMaterialClicked()
{
	if (MaterialItem.IsValid())
	{
		TArray<FAssetData> AssetDataArray;
		AssetDataArray.Add(MaterialItem->AssetData);

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetDataArray);
	}
	return FReply::Handled();
}

FReply SMaterialVaultMetadataPanel::OnOpenMaterialEditorClicked()
{
	if (MaterialItem.IsValid())
	{
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (AssetEditorSubsystem)
		{
			UObject* MaterialObject = MaterialItem->AssetData.GetAsset();
			if (MaterialObject)
			{
				AssetEditorSubsystem->OpenEditorForAsset(MaterialObject);
			}
		}
	}
	return FReply::Handled();
}

void SMaterialVaultMetadataPanel::UpdateUI()
{
	if (MaterialItem.IsValid())
	{
		// Update text boxes
		if (MaterialNameTextBox.IsValid())
		{
			MaterialNameTextBox->SetText(FText::FromString(MaterialItem->Metadata.MaterialName));
		}
		
		if (LocationTextBlock.IsValid())
		{
			LocationTextBlock->SetText(FText::FromString(MaterialItem->Metadata.Location));
		}
		
		if (AuthorTextBox.IsValid())
		{
			AuthorTextBox->SetText(FText::FromString(MaterialItem->Metadata.Author));
		}
		
		if (CategoryTextBox.IsValid())
		{
			CategoryTextBox->SetText(FText::FromString(MaterialItem->Metadata.Category));
		}
		
		if (LastModifiedTextBlock.IsValid())
		{
			LastModifiedTextBlock->SetText(FText::FromString(MaterialItem->Metadata.LastModified.ToString()));
		}
		
		if (NotesTextBox.IsValid())
		{
			NotesTextBox->SetText(FText::FromString(MaterialItem->Metadata.Notes));
		}

		// Update tag editor
		if (TagEditor.IsValid())
		{
			TagEditor->SetTags(&MaterialItem->Metadata.Tags);
			TagEditor->OnTagsChanged.BindSP(this, &SMaterialVaultMetadataPanel::OnTagsChanged);
		}

		// Update texture dependencies
		if (TextureDependencies.IsValid())
		{
			TextureDependencies->SetMaterialItem(MaterialItem);
		}
	}
}

void SMaterialVaultMetadataPanel::MarkAsChanged()
{
	if (!bHasUnsavedChanges)
	{
		bHasUnsavedChanges = true;
		if (MaterialItem.IsValid())
		{
			MaterialItem->Metadata.LastModified = FDateTime::Now();
		}
	}
}

void SMaterialVaultMetadataPanel::MarkAsClean()
{
	bHasUnsavedChanges = false;
}

bool SMaterialVaultMetadataPanel::IsEnabled() const
{
	return MaterialItem.IsValid();
}

FText SMaterialVaultMetadataPanel::GetMaterialTypeText() const
{
	if (MaterialItem.IsValid())
	{
		return FText::FromString(MaterialItem->AssetData.AssetClassPath.GetAssetName().ToString());
	}
	return FText::GetEmpty();
}

FText SMaterialVaultMetadataPanel::GetMaterialSizeText() const
{
	// TODO: Calculate material memory usage
	return LOCTEXT("SizeUnknown", "Size: Unknown");
}

EVisibility SMaterialVaultMetadataPanel::GetNoSelectionVisibility() const
{
	return MaterialItem.IsValid() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SMaterialVaultMetadataPanel::GetContentVisibility() const
{
	return MaterialItem.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SMaterialVaultMetadataPanel::GetSaveButtonVisibility() const
{
	return bHasUnsavedChanges ? EVisibility::Visible : EVisibility::Collapsed;
}

FSlateColor SMaterialVaultMetadataPanel::GetSaveButtonColor() const
{
	return bHasUnsavedChanges ? FSlateColor(FLinearColor::White) : FSlateColor::UseSubduedForeground();
}

TSharedPtr<SWidget> SMaterialVaultMetadataPanel::OnMaterialPreviewContextMenuOpening()
{
	if (!MaterialItem.IsValid())
	{
		return SNullWidget::NullWidget;
	}
	
	FMenuBuilder MenuBuilder(true, nullptr);
	
	MenuBuilder.AddMenuEntry(
		FUIAction(FExecuteAction::CreateSP(this, &SMaterialVaultMetadataPanel::OnChangeThumbnail)),
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 4, 0)
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush("Icons.Image"))
		]
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ChangeThumbnail", "Change Thumbnail/Swatch"))
		],
		NAME_None,
		LOCTEXT("ChangeThumbnailTooltip", "Select a custom image file to use as thumbnail")
	);
	
	return MenuBuilder.MakeWidget();
}

void SMaterialVaultMetadataPanel::OnChangeThumbnail()
{
	if (!MaterialItem.IsValid())
	{
		return;
	}
	
	// TODO: Open file dialog to select image file
	// For now, show a placeholder notification
	FNotificationInfo Info(LOCTEXT("CustomThumbnailNotImplemented", "Custom thumbnail feature will be implemented in a future update. Currently supports material preview only."));
	Info.ExpireDuration = 3.0f;
	Info.bFireAndForget = true;
	Info.Image = FAppStyle::GetBrush("Icons.Info");
	
	FSlateNotificationManager::Get().AddNotification(Info);
}

bool SMaterialVaultMetadataPanel::RenameAsset(const FString& NewName)
{
	if (!MaterialItem.IsValid())
	{
		return false;
	}

	// Validate new name
	if (NewName.IsEmpty())
	{
		return false;
	}

	// Just update the metadata name (no actual asset renaming)
	FString OldName = MaterialItem->Metadata.MaterialName;
	MaterialItem->Metadata.MaterialName = NewName;
	MaterialItem->DisplayName = NewName;
	
	// Mark as changed for saving
	MarkAsChanged();

	// Show success notification
	FNotificationInfo Info(FText::Format(LOCTEXT("MetadataNameUpdated", "Updated material display name from '{0}' to '{1}'"), 
		FText::FromString(OldName), FText::FromString(NewName)));
	Info.ExpireDuration = 3.0f;
	Info.bFireAndForget = true;
	Info.Image = FAppStyle::GetBrush("Icons.SuccessWithColor");
	
	FSlateNotificationManager::Get().AddNotification(Info);

	return true;
}

#undef LOCTEXT_NAMESPACE 