# MaterialVault - Unreal Engine Material Library Plugin

A comprehensive material library and management plugin for Unreal Engine 5.6+ that provides an intuitive interface for browsing, organizing, and applying materials in your projects.

![MaterialVault Interface](https://github.com/ScottRaffertyCG/Material-Vault/blob/main/Media/Material%20Vault%20Launch.gif)

## ✨ Features

### 📁 **Smart Organization**
- **Folder Tree View**: Hierarchical browser with automatic folder detection
- **Categories System**: Organize materials by categories with automatic categorization
- **Dual Navigation**: Switch between folder-based and category-based browsing
- **Engine Content Support**: Browse materials from Engine, Plugins, and Project content

### 🔍 **Advanced Browsing**
- **Grid & List Views**: Toggle between thumbnail grid and detailed list views
- **Live Search**: Real-time filtering as you type
- **Tag System**: Comprehensive tagging with visual tag management
- **Thumbnail Preview**: High-quality material thumbnails with caching

### 📝 **Rich Metadata**
- **Material Properties**: Author, category, notes, creation date
- **Tag Management**: Add, remove, and filter by custom tags
- **Texture Dependencies**: Automatic tracking of texture dependencies
- **Rename Support**: Update material display names (metadata-only)

### 🎨 **Material Application**
- **One-Click Apply**: Right-click materials to apply to selected objects
- **Batch Operations**: Apply materials to multiple selected static meshes
- **Undo Support**: Full transaction support with undo/redo
- **Smart Notifications**: Success/warning/error feedback

### ⚙️ **Customization**
- **View Preferences**: Adjustable thumbnail sizes and view modes
- **Sort Options**: Multiple sorting methods (name, date, author)
- **Settings Persistence**: Your preferences are automatically saved
- **Toolbar Integration**: Convenient toolbar button for quick access

## 🚀 Installation

### Prerequisites
- Unreal Engine 5.6 or later
- Windows development environment
- Visual Studio 2022 (for building from source)

### Option 1: Plugin Installation
1. Download the latest release from [Releases](../../releases)
2. Extract the `MaterialVault` folder to your project's `Plugins` directory
3. Restart Unreal Engine
4. Enable the plugin in **Edit → Plugins → Project → Other**

### Option 2: Engine Installation
1. Extract the `MaterialVault` folder to `[UE5_Install]/Engine/Plugins/Editor/`
2. Restart Unreal Engine (available in all projects)

### Option 3: Build from Source
```bash
git clone https://github.com/YourUsername/MaterialVault.git
cd MaterialVault
# Build using UE5 build tools or Visual Studio
```

## 📖 Usage

### Opening MaterialVault
- **Toolbar**: Click the MaterialVault button in the Level Editor toolbar
- **Menu**: **Window → Material Vault**
- **Shortcut**: Dock the tab wherever you prefer

### Basic Workflow
1. **Browse Materials**: Use the folder tree or categories panel to navigate
2. **Search & Filter**: Use the search box or click tags to filter materials
3. **Preview**: View material thumbnails and metadata in the right panel
4. **Apply**: Select objects in the viewport, then right-click a material and choose "Apply Material"

### Advanced Features
- **Edit Metadata**: Select a material and edit its properties in the metadata panel
- **Manage Tags**: Add/remove tags using the tag editor
- **Organize**: Create custom categories and organize your material library
- **Refresh**: Use the refresh button to update the library after adding new materials

## 🛠️ Development

### Architecture
```
MaterialVault/
├── Source/MaterialVault/
│   ├── Private/
│   │   ├── MaterialVault.cpp              # Main module
│   │   ├── MaterialVaultManager.cpp       # Core management system
│   │   ├── MaterialVaultStyle.cpp         # UI styling
│   │   ├── SMaterialVaultWidget.cpp       # Main interface
│   │   ├── SMaterialVaultFolderTree.cpp   # Folder navigation
│   │   ├── SMaterialVaultMaterialGrid.cpp # Material browser
│   │   ├── SMaterialVaultMetadataPanel.cpp # Property editor
│   │   └── SMaterialVaultCategoriesPanel.cpp # Categories system
│   └── Public/
│       ├── MaterialVaultTypes.h           # Data structures
│       ├── MaterialVaultManager.h         # Manager interface
│       └── [Other headers...]
├── Resources/
│   └── Icon128.png                        # Plugin icon
└── MaterialVault.uplugin                  # Plugin definition
```

### Key Components
- **UMaterialVaultManager**: EditorSubsystem handling data management
- **SMaterialVaultWidget**: Main 3-panel interface (folders, grid, metadata)
- **FMaterialVaultMetadata**: JSON-serialized metadata storage
- **MaterialVaultTypes.h**: Comprehensive data structures and enums

### Building
```bash
# Generate project files
UnrealBuildTool.exe -projectfiles -project="YourProject.uproject" -game -rocket -progress

# Build the plugin
MSBuild.exe YourProject.sln -p:Configuration=Development -p:Platform=x64
```

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Development Setup
1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Make your changes and test thoroughly
4. Commit with clear messages: `git commit -m "Add amazing feature"`
5. Push to your fork: `git push origin feature/amazing-feature`
6. Open a Pull Request

### Code Style
- Follow UE5 coding standards
- Use meaningful variable and function names
- Add comments for complex logic
- Ensure all public APIs are documented

## 📋 Changelog

### Version 1.0.0 (Latest)
- ✅ Initial release
- ✅ Three-panel interface (folders, materials, metadata)
- ✅ Grid and list view modes
- ✅ Comprehensive metadata system
- ✅ Tag management and filtering
- ✅ Material application with undo support
- ✅ Folder and category-based organization
- ✅ Texture dependency tracking
- ✅ Settings persistence

See [CHANGELOG.md](CHANGELOG.md) for detailed version history.

## 🐛 Known Issues

- Material renaming only affects display names (not actual asset names)
- Large material libraries may experience slower initial loading
- Thumbnail generation depends on UE5's asset thumbnail system

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Epic Games for the Unreal Engine framework
- The UE5 community for inspiration and feedback
- Contributors who helped improve this plugin

## 📞 Support

- **Issues**: Report bugs and request features via [GitHub Issues](../../issues)
- **Discussions**: Join the conversation in [GitHub Discussions](../../discussions)
- **Documentation**: Additional docs available in the [Wiki](../../wiki)

---

**Made with ❤️ for the Unreal Engine community**

*If this plugin helps your workflow, consider giving it a ⭐ on GitHub!* 
