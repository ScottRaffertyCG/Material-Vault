#include "MaterialVaultCommands.h"

#define LOCTEXT_NAMESPACE "FMaterialVaultModule"

void FMaterialVaultCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "Material Vault", "Launch the Material Vault library", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
