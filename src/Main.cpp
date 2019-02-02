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

#define buffChars 1

static HANDLE _hModule;
static NppData nppData;
static HHOOK hook = NULL;
static HKL keyboardLayout = NULL;
static bool hasFocus = true;
static ScintillaGateway editor;
static LPWORD kbBuff = (LPWORD)new byte[buffChars];
static PBYTE kbState = new byte[256];

static void enableSurroundSelection();
static void showAbout();

LRESULT CALLBACK KeyboardProc(int ncode, WPARAM wparam, LPARAM lparam);

FuncItem funcItem[] = {
	{ TEXT("Enable"), enableSurroundSelection, 0, false, nullptr },
	{ TEXT(""), nullptr, 0, false, nullptr },
	{ TEXT("About..."), showAbout, 0, false, nullptr }
};

const wchar_t *GetIniFilePath() {
	static wchar_t iniPath[MAX_PATH] = { 0 };

	if (iniPath[0] == 0) {
		SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)iniPath);
		wcscat_s(iniPath, MAX_PATH, L"\\SurroundSelection.ini");
	}

	return iniPath;
}

static void updateKL() {
	keyboardLayout = GetKeyboardLayout(0);
}

static WPARAM keyIfDown(UINT value) {
	return (GetKeyState(value) & KF_UP) != 0;
}

static void enableSurroundSelection() {
	if (hook) {
		UnhookWindowsHookEx(hook);
		hook = NULL;
		SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[0]._cmdID, 0);
	}
	else {
		hook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, (HINSTANCE)_hModule, ::GetCurrentThreadId());
		SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[0]._cmdID, 1);
	}
}

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
	if (ncode != HC_ACTION)
		goto proceed;
	bool keyState = (lparam >> 30) & 1;
	/*if (isKey(wparam, VK_CONTROL) || isKey(wparam, VK_SHIFT) || isKey(wparam, VK_MENU)) {
		if (keyState)
			kbModifiers |= wparam;
		else
			kbModifiers &= ~wparam;
		printf("[SurroundSelection] 1. ModifierKey: %d\n", kbModifiers);
		goto proceed;
	}*/
	if ((HIWORD(lparam) & KF_UP) != 0 || !hasFocus)
		goto proceed;


	//printf("[SurroundSelection] 1. Keyboard state\n");
	char ch1 = 0, ch2 = 0;
	UINT scanCode = (lparam >> 16) & 0xFF;
	if (!GetKeyboardState(kbState))
		goto proceed;
	//printf("[SurroundSelection] 2. Key char\n");
	//WPARAM kbModifiers = 0;
	//kbModifiers = keyIfDown(VK_CONTROL) | keyIfDown(VK_SHIFT) | keyIfDown(VK_MENU);
	int amount = ToAsciiEx((UINT)wparam, scanCode, kbState, kbBuff, 0, keyboardLayout);
	if (amount < 1)
		goto proceed;

	char ch = (char)kbBuff[0];
	//printf("[SurroundSelection] 3. Parse char #%d (%c,%c,%c,%c)\n", amount, kbBuff[3], kbBuff[2], kbBuff[1], kbBuff[0]);
	if (ch == '"' || ch == '\'' || ch == '`') {
		ch1 = ch2 = (char)ch;
	} else if (ch == '(' || ch == ')') {
		ch1 = '('; ch2 = ')';
	} else if (ch == '[' || ch == ']') {
		ch1 = '['; ch2 = ']';
	} else if (ch == '{' || ch == '}') {
		ch1 = '{'; ch2 = '}';
	} else if (ch == '<' || ch == '>') {
		ch1 = '<'; ch2 = '>';
	}
	/*if (wparam == VK_OEM_7) {
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
	else if (wparam == 0x39 || wparam == 0x30) {
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
	//}*/

	if (ch1 != 0 && editor.GetSelectionEmpty() == 0) {
		SurroundSelectionsWith(ch1, ch2);
		//printf("[SurroundSelection] Done.\n\n");
		return TRUE; // This key has been "handled" and won't propogate
	}

	proceed:
	//printf("\n\n");
	return CallNextHookEx(hook, ncode, wparam, lparam); //pass control to next hook in the hook chain.
}


BOOL APIENTRY DllMain(HANDLE hModule, DWORD  reasonForCall, LPVOID lpReserved) {
	switch (reasonForCall) {
		case DLL_PROCESS_ATTACH:
			_hModule = hModule;
			updateKL();
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
	*nbF = sizeof(funcItem) / sizeof(funcItem[0]);
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
		case NPPN_READY: {
			bool isEnabled = GetPrivateProfileInt(TEXT("SurroundSelection"), TEXT("enabled"), 1, GetIniFilePath()) == 1;
			if (isEnabled) {
				hook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, (HINSTANCE)_hModule, ::GetCurrentThreadId());
				SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[0]._cmdID, 1);
			}
			break;
		}
		case NPPN_SHUTDOWN:
			WritePrivateProfileString(TEXT("SurroundSelection"), TEXT("enabled"), hook ? TEXT("1") : TEXT("0"), GetIniFilePath());
			if (hook != NULL)
				UnhookWindowsHookEx(hook);
			break;
		case NPPN_BUFFERACTIVATED:
			editor.SetScintillaInstance(GetCurrentScintilla());
			break;
	}
	return;
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam) {
	if (Message == WM_INPUTLANGCHANGE)
		updateKL();
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode() {
	return TRUE;
}
#endif
