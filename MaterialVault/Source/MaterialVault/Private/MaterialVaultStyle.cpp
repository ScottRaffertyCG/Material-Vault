// Copyright Epic Games, Inc. All Rights Reserved.

#include "MaterialVaultStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateTypes.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FMaterialVaultStyle::StyleInstance = nullptr;

void FMaterialVaultStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FMaterialVaultStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FMaterialVaultStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("MaterialVaultStyle"));
	return StyleSetName;
}

TSharedRef< FSlateStyleSet > FMaterialVaultStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("MaterialVaultStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("MaterialVault")->GetBaseDir() / TEXT("Resources"));

	// MaterialVault toolbar button icon
	Style->Set("MaterialVault.PluginAction", new IMAGE_BRUSH(TEXT("Icon128"), FVector2D(40.0f, 40.0f)));

	return Style;
}

const ISlateStyle& FMaterialVaultStyle::Get()
{
	return *StyleInstance;
} 