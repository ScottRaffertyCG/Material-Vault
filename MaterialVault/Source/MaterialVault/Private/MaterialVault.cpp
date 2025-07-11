#include "MaterialVault.h"
#include "MaterialVaultCommands.h"
#include "MaterialVaultStyle.h"
#include "MaterialVaultManager.h"
#include "SMaterialVaultWidget.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "EditorStyleSet.h"

const FName FMaterialVaultModule::MaterialVaultTabName("MaterialVault");

#define LOCTEXT_NAMESPACE "FMaterialVaultModule"

void FMaterialVaultModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	// Initialize style system for custom 16:9 icon
	FMaterialVaultStyle::Initialize();

	FMaterialVaultCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FMaterialVaultCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FMaterialVaultModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMaterialVaultModule::RegisterMenus));
	
	// Register tab spawner
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(MaterialVaultTabName, FOnSpawnTab::CreateRaw(this, &FMaterialVaultModule::OnSpawnMaterialVaultTab))
		.SetDisplayName(LOCTEXT("MaterialVaultTabTitle", "Material Vault"))
		.SetTooltipText(LOCTEXT("MaterialVaultTooltipText", "Launch the Material Vault library"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FMaterialVaultStyle::GetStyleSetName(), "MaterialVault.PluginAction"));
	
	// Initialize manager
	MaterialVaultManager = nullptr;
	bIsTabOpen = false;
}

void FMaterialVaultModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	// Shutdown style system
	FMaterialVaultStyle::Shutdown();

	FMaterialVaultCommands::Unregister();
	
	// Unregister tab spawner
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MaterialVaultTabName);
	
	// Clean up
	MaterialVaultWidget.Reset();
	MaterialVaultManager = nullptr;
}

void FMaterialVaultModule::PluginButtonClicked()
{
	OpenMaterialVaultTab();
}

void FMaterialVaultModule::OpenMaterialVaultTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(MaterialVaultTabName);
}

TSharedRef<SDockTab> FMaterialVaultModule::OnSpawnMaterialVaultTab(const FSpawnTabArgs& Args)
{
	// Get or create the MaterialVault manager
	if (!MaterialVaultManager)
	{
		MaterialVaultManager = GEditor->GetEditorSubsystem<UMaterialVaultManager>();
		if (!MaterialVaultManager)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to get MaterialVault manager"));
		}
	}
	
	// Create the main widget
	MaterialVaultWidget = SNew(SMaterialVaultWidget);
	
	// Create the tab
	TSharedRef<SDockTab> NewTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("MaterialVaultTabTitle", "Material Vault"))
		.ToolTipText(LOCTEXT("MaterialVaultTooltipText", "Launch the Material Vault library"))
		.OnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FMaterialVaultModule::OnMaterialVaultTabClosed))
		[
			MaterialVaultWidget.ToSharedRef()
		];
	
	bIsTabOpen = true;
	return NewTab;
}

void FMaterialVaultModule::OnMaterialVaultTabClosed(TSharedRef<SDockTab> Tab)
{
	MaterialVaultWidget.Reset();
	bIsTabOpen = false;
}

void FMaterialVaultModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FMaterialVaultCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FMaterialVaultCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMaterialVaultModule, MaterialVault)