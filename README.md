# SurroundSelection

[![Build status](https://ci.appveyor.com/api/projects/status/y3rxu3o00clgxm7f?svg=true)](https://ci.appveyor.com/project/dail8859/surroundselection)

Notepad++ plugin to automatically surround the selection in quotes/brackets/parenthesis/etc. This can also be used on multiple or rectangular selections.

![Demo](/img/Demo.gif)

## Usage
Select some text and type one of the following characters:
- `'`
- `"`
- `` ` ``
- `(` or `)`
- `[` or `]`
- `{` or `}`
- `<` or `>`

## Installation
Install the plugin by downloading it from the [Release](https://github.com/dail8859/SurroundSelection/releases) page and copy `SurroundSelection.dll` to your `plugins` folder. You can also install it via the Plugin Admin or Plugin Manager (depending on your Notepad++ version).

## Development
The code has been developed using MSVC 2015. Building the code will generate the DLL which can be used by Notepad++. For convenience, MSVC copies the DLL into the Notepad++ plugin directory.

## License
This code is released under the [GNU General Public License version 2](http://www.gnu.org/licenses/gpl-2.0.txt).
