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

The configuration file SurroundSelection.ini provides additional characters to surround with. Use as (for example):

    AdditionalChars=*-_

to use `*`, `-` and `_` to act as surround characters as the above examples do.

## Installation

Use the built in Plugin Admin to automatically install the plugin.

## Development
The code is developed using Visual Studio 2017. Building the code will generate a DLL which can be used by Notepad++. For convenience, Visual Studio copies the DLL into the Notepad++ plugin directory.

## License
This code is released under the [GNU General Public License version 2](http://www.gnu.org/licenses/gpl-2.0.txt).
