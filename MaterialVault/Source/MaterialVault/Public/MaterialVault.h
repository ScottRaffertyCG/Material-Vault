#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "MaterialVaultTypes.h"

class FToolBarBuilder;
class FMenuBuilder;
class SMaterialVaultWidget;
class UMaterialVaultManager;

class FMaterialVaultModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** Gets the MaterialVault manager instance */
	UMaterialVaultManager* GetMaterialVaultManager() const { return MaterialVaultManager; }
	
	/** Opens the MaterialVault tab */
	void OpenMaterialVaultTab();
	
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	
	/** Gets the singleton instance of this module */
	static FMaterialVaultModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FMaterialVaultModule>("MaterialVault");
	}
	
	/** Checks if the module is loaded and ready */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("MaterialVault");
	}

private:
	/** Registers menus and toolbars */
	void RegisterMenus();
	
	/** Tab management */
	TSharedRef<SDockTab> OnSpawnMaterialVaultTab(const FSpawnTabArgs& Args);
	void OnMaterialVaultTabClosed(TSharedRef<SDockTab> Tab);
	
	/** UI Commands */
	TSharedPtr<class FUICommandList> PluginCommands;
	
	/** Main widget instance */
	TSharedPtr<SMaterialVaultWidget> MaterialVaultWidget;
	
	/** Manager instance */
	UMaterialVaultManager* MaterialVaultManager;
	
	/** Tab identifier */
	static const FName MaterialVaultTabName;
	
	/** Whether the tab is currently open */
	bool bIsTabOpen;
};
