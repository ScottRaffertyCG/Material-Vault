# MaterialVault

A comprehensive material library management plugin for Unreal Engine 5.6+ that transforms how you organize, browse, and apply materials in your projects.

![MaterialVault Interface](https://github.com/ScottRaffertyCG/Material-Vault/blob/main/Media/MaterialVaultLogo.png)

## Features

**Current Implementation**
- Hierarchical folder structure displaying content organization
- Visual thumbnail grid with material previews
- Metadata panel for viewing material information
- Basic search functionality across material names
- Tag management system for organizing materials
- Asset Registry integration for material discovery

**Powerful Search & Discovery**
- Real-time search across material names and metadata
- Tag-based filtering for quick material discovery
- Combined search and filter operations
- Asset Registry integration for fast lookups

**Streamlined Workflow**
- Right-click context menus for material operations
- One-click material application to selected actors
- Undo/redo support for material changes
- Drag-and-drop support for intuitive interaction
- Thumbnail caching system for responsive UI
- Memory-efficient asset handling

## Installation

### Option 1: Plugin Directory
1. Download or clone this repository
2. Copy the `MaterialVault` folder to your project's `Plugins` directory
3. Restart Unreal Engine
4. Enable the plugin in Project Settings → Plugins

### Option 2: Engine Plugins
1. Copy the `MaterialVault` folder to `[UE Install]/Engine/Plugins/Editor/`
2. Restart Unreal Engine
3. The plugin will be available for all projects

## Usage

### Opening MaterialVault
- **Menu**: Tools → MaterialVault
- **Toolbar**: Click the MaterialVault button in the main toolbar

  ![MaterialVault Interface](https://github.com/ScottRaffertyCG/Material-Vault/blob/main/Media/Material%20Vault%20Launch.gif)

### Interface Overview
The MaterialVault window features a three-panel layout:

**Left Panel - Folder Tree**
- Navigate through your material folder hierarchy
- Create new folders with right-click menu
- Organize materials by project, type, or any structure you prefer
- Switch between folder view and categories view
- Browse materials organized by content folder structure

**Center Panel - Material Grid**
- Browse materials with thumbnail previews
- Search materials by name using the search bar
- Right-click materials for context menu options
- Double-click to apply materials to selected actors

**Right Panel - Metadata & Categories**
- View detailed information about selected materials
- Add and remove tags for better organization
- Filter materials by selected tags
- Manage material categories

### Material Management

**Current Functionality**
- Materials are automatically discovered through the Asset Registry
- Browse materials using the hierarchical folder structure
- View material thumbnails and basic information
- Search materials by name using the search bar
- Add/remove tags for material organization (UI implemented)

**Applying Materials**
- Select one or more actors in the viewport
- Right-click a material in MaterialVault
- Choose "Apply to Selected Actors"
- Changes support full undo/redo functionality

**Organizing with Tags**
- Select a material to view its metadata
- Add relevant tags in the Categories panel
- Use tag filtering to quickly find materials
- Tags are stored in JSON files alongside your materials

## Technical Details

### Architecture
- **UMaterialVaultManager**: EditorSubsystem managing material data and operations
- **SMaterialVaultWidget**: Main Slate widget orchestrating the three-panel interface
- **Asset Registry Integration**: Efficient material discovery and monitoring
- **JSON Metadata**: Extensible storage for tags and custom properties

### File Structure
```
MaterialVault/
├── Source/MaterialVault/
│   ├── Private/           # Implementation files
│   └── Public/            # Header files
├── Resources/
│   └── Icon128.png        # Plugin icon
└── MaterialVault.uplugin  # Plugin descriptor
```

### Performance Considerations
- Thumbnail generation is cached to disk for fast subsequent loads
- Asset Registry queries minimize memory usage
- Lazy loading of material data improves startup times
- Efficient UI updates through Slate's reactive system

## Development

### Building from Source
1. Ensure you have UE 5.6+ installed with C++ development tools
2. Clone this repository to your project's Plugins folder
3. Generate project files and compile

### Plugin Dependencies
- UnrealEd
- EditorStyle
- EditorWidgets
- ToolMenus
- AssetRegistry
- ContentBrowser

## Compatibility

- **Unreal Engine**: 5.6+
- **Platforms**: Windows,(editor only). Untested on Mac or Linux yet
- **Project Types**: C++ and Blueprint projects

## Contributing

Contributions are welcome!

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for detailed version history and feature additions. 
