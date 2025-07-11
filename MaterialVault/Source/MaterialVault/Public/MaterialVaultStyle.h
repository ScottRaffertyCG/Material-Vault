#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;

/**
 * Minimal style system for MaterialVault - only handles custom icon registration
 */
class FMaterialVaultStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static FName GetStyleSetName();
	static const class ISlateStyle& Get();

private:
	static TSharedRef< FSlateStyleSet > Create();
	static TSharedPtr< FSlateStyleSet > StyleInstance;
}; 