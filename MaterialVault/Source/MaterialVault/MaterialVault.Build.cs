using UnrealBuildTool;

public class MaterialVault : ModuleRules
{
	public MaterialVault(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"EditorStyle",
				"EditorWidgets",
				"ToolMenus",
				"WorkspaceMenuStructure",
				"ContentBrowser",
				"AssetTools",
				"AssetRegistry",
				"PropertyEditor",
				"MaterialEditor",
				"EditorSubsystem",
				"DesktopPlatform"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"ToolWidgets",
				"DeveloperSettings",
				"RenderCore",
				"RHI",
				"ImageWrapper",
				"Json",
				"ApplicationCore",
				"AppFramework",
				"MainFrame",
				"LevelEditor",
				"CollectionManager",
				"ContentBrowserData",
				"SourceControl",
				"AssetDefinition",
				"TypedElementFramework",
				"TypedElementRuntime"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
