// This file is part of SurroundSelection.
// 
// Copyright (C)2017 Justin Dailey <dail8859@yahoo.com>
// 
// SurroundSelection is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginInterface.h"
#include "ScintillaGateway.h"
#include "resource.h"
#include "AboutDialog.h"

#include <algorithm>
#include <vector>

static HANDLE _hModule;
static NppData nppData;
static HHOOK hook = NULL;
static bool hasFocus = true;
static ScintillaGateway editor;

static void showAbout();

FuncItem funcItem[] = {
	{ TEXT("About..."), showAbout, 0, false, nullptr }
};

static void showAbout() {
	ShowAboutDialog((HINSTANCE)_hModule, MAKEINTRESOURCE(IDD_ABOUTDLG), nppData._nppHandle);
}

static HWND GetCurrentScintilla() {
	int which = 0;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, SCI_UNUSED, (LPARAM)&which);
	return (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}

static void SurroundSelectionsWith(char ch1, char ch2) {
	std::vector<std::pair<int, int>> selections;

	int num = editor.GetSelections();
	for (int i = 0; i < num; ++i) {
		int start = editor.GetSelectionNStart(i);
		int end = editor.GetSelectionNEnd(i);

		if (start != end /* && editor.LineFromPosition(start) == editor.LineFromPosition(end) */)
			selections.push_back(std::make_pair(start, end));
	}

	// Sort so they are replaced top to bottom
	std::sort(selections.begin(), selections.end());

	editor.BeginUndoAction();
	editor.ClearSelections();

	int offset = 0;
	for (size_t i = 0; i < selections.size(); ++i) {
		const auto &selection = selections[i];
		editor.SetTargetRange(selection.first + offset, selection.second + offset);

		auto target = editor.GetTargetText();

		// Add in the characters
		target.insert(target.begin(), 1, ch1);
		target.insert(target.end(), 1, ch2);

		editor.ReplaceTarget(target);

		if (i == 0)
			editor.SetSelection(selection.first + offset + 1, selection.second + offset + 1);
		else
			editor.AddSelection(selection.first + offset + 1, selection.second + offset + 1);

		offset += 2; // Add 2 since the replaced string is 2 chars longer
	}

	editor.EndUndoAction();
}

LRESULT CALLBACK KeyboardProc(int ncode, WPARAM wparam, LPARAM lparam) {
	if (ncode == HC_ACTION) {
		if ((HIWORD(lparam) & KF_UP) == 0) {
			if (hasFocus && !(GetKeyState(VK_CONTROL) & KF_UP) && !(GetKeyState(VK_MENU) & KF_UP)) {
				char ch1 = 0, ch2 = 0;

				if (wparam == VK_OEM_7) {
					if (GetKeyState(VK_SHIFT) & KF_UP) {
						ch1 = ch2 = '"';
					}
					else {
						ch1 = ch2 = '\'';
					}
				}
				else if (wparam == VK_OEM_4 || wparam == VK_OEM_6) {
					if (GetKeyState(VK_SHIFT) & KF_UP) {
						ch1 = '{';
						ch2 = '}';
					}
					else {
						ch1 = '[';
						ch2 = ']';
					}
				}
				else if (wparam == 0x39 || wparam == 0x3A) {
					if (GetKeyState(VK_SHIFT) & KF_UP) {
						ch1 = '(';
						ch2 = ')';
					}
				}
				//else if (wparam == VK_OEM_COMMA) {
				//	if (GetKeyState(VK_SHIFT) & KF_UP) {
				//		ch1 = '<';
				//		ch2 = '>';
				//	}
				//}

				if (ch1 != 0 && editor.GetSelectionEmpty() == 0) {
					SurroundSelectionsWith(ch1, ch2);
					return TRUE; // This key has been "handled" and won't propogate
				}
			}
		}
	}
	return CallNextHookEx(hook, ncode, wparam, lparam); //pass control to next hook in the hook chain.
}


BOOL APIENTRY DllMain(HANDLE hModule, DWORD  reasonForCall, LPVOID lpReserved) {
	switch (reasonForCall) {
		case DLL_PROCESS_ATTACH:
			_hModule = hModule;
			break;
		case DLL_PROCESS_DETACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
	nppData = notepadPlusData;

	// Set this as early as possible so its in a valid state
	editor.SetScintillaInstance(nppData._scintillaMainHandle);
}

extern "C" __declspec(dllexport) const wchar_t *getName() {
	return TEXT("SurroundSelection");
}

extern "C" __declspec(dllexport) FuncItem *getFuncsArray(int *nbF) {
	*nbF = sizeof(funcItem) / sizeof(funcItem[0]);;
	return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {
	switch (notifyCode->nmhdr.code) {
		case SCN_FOCUSIN:
			hasFocus = true;
			break;
		case SCN_FOCUSOUT:
			hasFocus = false;
			break;
		case NPPN_READY:
			hook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, (HINSTANCE)_hModule, ::GetCurrentThreadId());
			break;
		case NPPN_SHUTDOWN:
			if (hook != NULL) UnhookWindowsHookEx(hook);
			break;
		case NPPN_BUFFERACTIVATED:
			editor.SetScintillaInstance(GetCurrentScintilla());
			break;
	}
	return;
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam) {
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode() {
	return TRUE;
}
#endif
