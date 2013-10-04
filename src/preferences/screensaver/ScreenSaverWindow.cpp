/*
 * Copyright 2003-2013 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jérôme Duval, jerome.duval@free.fr
 *		Michael Phipps
 *		John Scipione, jscipione@gmail.com
 */


#include "ScreenSaverWindow.h"

#include <algorithm>
	// for std::max and std::min

#include <stdio.h>

#include <Alignment.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Directory.h>
#include <DurationFormat.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <Layout.h>
#include <LayoutBuilder.h>
#include <ListItem.h>
#include <ListView.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <ScreenSaver.h>
#include <ScrollView.h>
#include <Size.h>
#include <Slider.h>
#include <StringView.h>
#include <TabView.h>
#include <TextView.h>

#include <BuildScreenSaverDefaultSettingsView.h>

#include "PreviewView.h"
#include "ScreenCornerSelector.h"
#include "ScreenSaverItem.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ScreenSaver"


const uint32 kPreviewMonitorGap = 16;
const uint32 kMinSettingsWidth = 230;
const uint32 kMinSettingsHeight = 120;

const int32 kMsgSaverSelected = 'SSEL';
const int32 kMsgTestSaver = 'TEST';
const int32 kMsgAddSaver = 'ADD ';
const int32 kMsgPasswordCheckBox = 'PWCB';
const int32 kMsgRunSliderChanged = 'RSch';
const int32 kMsgRunSliderUpdate = 'RSup';
const int32 kMsgPasswordSliderChanged = 'PWch';
const int32 kMsgPasswordSliderUpdate = 'PWup';
const int32 kMsgChangePassword = 'PWBT';
const int32 kMsgEnableScreenSaverBox = 'ESCH';

const int32 kMsgTurnOffCheckBox = 'TUOF';
const int32 kMsgTurnOffSliderChanged = 'TUch';
const int32 kMsgTurnOffSliderUpdate = 'TUup';

const int32 kMsgFadeCornerChanged = 'fdcc';
const int32 kMsgNeverFadeCornerChanged = 'nfcc';


class TimeSlider : public BSlider {
public:
								TimeSlider(const char* name,
									uint32 changedMessage,
									uint32 updateMessage);
	virtual						~TimeSlider();

	virtual	void				SetValue(int32 value);

			void				SetTime(bigtime_t useconds);
			bigtime_t			Time() const;

private:
			void				_TimeToString(bigtime_t useconds,
									BString& string);
};


class FadeView : public BView {
public:
								FadeView(const char* name,
									ScreenSaverSettings& settings);

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			void				UpdateTurnOffScreen();
			void				UpdateStatus();

private:
			ScreenSaverSettings&	fSettings;
			uint32				fTurnOffScreenFlags;

			BCheckBox*			fEnableCheckBox;
			TimeSlider*			fRunSlider;

			BTextView*			fTurnOffNotSupported;
			BCheckBox*			fTurnOffCheckBox;
			TimeSlider*			fTurnOffSlider;

			BCheckBox*			fPasswordCheckBox;
			TimeSlider*			fPasswordSlider;
			BButton*			fPasswordButton;

			ScreenCornerSelector*	fFadeNow;
			ScreenCornerSelector*	fFadeNever;
};


class ModulesView : public BView {
public:
								ModulesView(const char* name,
									ScreenSaverSettings& settings);
	virtual						~ModulesView();

	virtual	void				DetachedFromWindow();
	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();
	virtual	void				MessageReceived(BMessage* message);

			void				EmptyScreenSaverList();
			void				PopulateScreenSaverList();

			void				SaveState();

private:
	static	int					_CompareScreenSaverItems(const void* left,
									const void* right);

			BScreenSaver*		_ScreenSaver();
			void				_CloseSaver();
			void				_OpenSaver();

private:
			BFilePanel*			fFilePanel;
			BListView*			fScreenSaversListView;
			BButton*			fTestButton;
			BButton*			fAddButton;

			ScreenSaverSettings&	fSettings;
			ScreenSaverRunner*	fSaverRunner;
			BString				fCurrentName;

			BBox*				fSettingsBox;
			BView*				fSettingsView;

			PreviewView*		fPreviewView;

			team_id				fScreenSaverTestTeam;
};


//	#pragma mark - TimeSlider


static const int32 kTimeInUnits[] = {
	30,    60,   90,
	120,   150,  180,
	240,   300,  360,
	420,   480,  540,
	600,   900,  1200,
	1500,  1800, 2400,
	3000,  3600, 5400,
	7200,  9000, 10800,
	14400, 18000
};

static const int32 kTimeUnitCount
	= sizeof(kTimeInUnits) / sizeof(kTimeInUnits[0]);


TimeSlider::TimeSlider(const char* name, uint32 changedMessage,
	uint32 updateMessage)
	:
	BSlider(name, B_TRANSLATE("30 seconds"), new BMessage(changedMessage),
		0, kTimeUnitCount - 1, B_HORIZONTAL, B_TRIANGLE_THUMB)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetModificationMessage(new BMessage(updateMessage));
	SetBarThickness(10);
}


TimeSlider::~TimeSlider()
{
}


void
TimeSlider::SetValue(int32 value)
{
	int32 oldValue = Value();
	BSlider::SetValue(value);

	if (oldValue != Value()) {
		BString label;
		_TimeToString(kTimeInUnits[Value()] * 1000000LL, label);
		SetLabel(label.String());
	}
}


void
TimeSlider::SetTime(bigtime_t useconds)
{
	for (int t = 0; t < kTimeUnitCount; t++) {
		if (kTimeInUnits[t] * 1000000LL == useconds) {
			SetValue(t);
			break;
		}
	}
}


bigtime_t
TimeSlider::Time() const
{
	return 1000000LL * kTimeInUnits[Value()];
}


void
TimeSlider::_TimeToString(bigtime_t useconds, BString& string)
{
	BDurationFormat formatter;
	formatter.Format(0, useconds, &string);
}


//	#pragma mark - FadeView


FadeView::FadeView(const char* name, ScreenSaverSettings& settings)
	:
	BView(name, B_WILL_DRAW),
	fSettings(settings)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float textHeight = ceilf(fontHeight.ascent + fontHeight.descent);

	fEnableCheckBox = new BCheckBox("EnableCheckBox",
		B_TRANSLATE("Enable screensaver"),
		new BMessage(kMsgEnableScreenSaverBox));

	BBox* box = new BBox("EnableScreenSaverBox");
	box->SetLabel(fEnableCheckBox);

	// Start Screensaver
	BStringView* startScreenSaver = new BStringView("startScreenSaver",
		B_TRANSLATE("Start screensaver"));
	startScreenSaver->SetAlignment(B_ALIGN_RIGHT);

	fRunSlider = new TimeSlider("RunSlider", kMsgRunSliderChanged,
		kMsgRunSliderUpdate);

	// Turn Off
	rgb_color textColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DISABLED_LABEL_TINT);
	fTurnOffNotSupported = new BTextView("not_supported", be_plain_font,
		&textColor, B_WILL_DRAW);
	fTurnOffNotSupported->SetExplicitMinSize(BSize(B_SIZE_UNSET,
		3 + textHeight * 3));
	fTurnOffNotSupported->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fTurnOffNotSupported->MakeEditable(false);
	fTurnOffNotSupported->MakeSelectable(false);
	fTurnOffNotSupported->SetText(
		B_TRANSLATE("Display Power Management Signaling not available"));

	fTurnOffCheckBox = new BCheckBox("TurnOffScreenCheckBox",
		B_TRANSLATE("Turn off screen"), new BMessage(kMsgTurnOffCheckBox));
	fTurnOffCheckBox->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));

	fTurnOffSlider = new TimeSlider("TurnOffSlider", kMsgTurnOffSliderChanged,
		kMsgTurnOffSliderUpdate);

	// Password
	fPasswordCheckBox = new BCheckBox("PasswordCheckbox",
		B_TRANSLATE("Password lock"), new BMessage(kMsgPasswordCheckBox));
	fPasswordCheckBox->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));

	fPasswordSlider = new TimeSlider("PasswordSlider",
		kMsgPasswordSliderChanged, kMsgPasswordSliderUpdate);

	fPasswordButton = new BButton("PasswordButton",
		B_TRANSLATE("Password" B_UTF8_ELLIPSIS),
		new BMessage(kMsgChangePassword));

	// Bottom
	float monitorHeight = 10 + textHeight * 3;
	float aspectRatio = 4.0f / 3.0f;
	float monitorWidth = monitorHeight * aspectRatio;
	BRect monitorRect = BRect(0, 0, monitorWidth, monitorHeight);

	fFadeNow = new ScreenCornerSelector(monitorRect, "FadeNow",
		new BMessage(kMsgFadeCornerChanged), B_FOLLOW_NONE);
	BTextView* fadeNowText = new BTextView("FadeNowText", B_WILL_DRAW);
	fadeNowText->SetExplicitMinSize(BSize(B_SIZE_UNSET,
		4 + textHeight * 4));
	fadeNowText->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fadeNowText->MakeEditable(false);
	fadeNowText->MakeSelectable(false);
	fadeNowText->SetText(B_TRANSLATE("Fade now when mouse is here"));

	fFadeNever = new ScreenCornerSelector(monitorRect, "FadeNever",
		new BMessage(kMsgNeverFadeCornerChanged), B_FOLLOW_NONE);
	BTextView* fadeNeverText = new BTextView("FadeNeverText", B_WILL_DRAW);
	fadeNeverText->SetExplicitMinSize(BSize(B_SIZE_UNSET,
		4 + textHeight * 4));
	fadeNeverText->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fadeNeverText->MakeEditable(false);
	fadeNeverText->MakeSelectable(false);
	fadeNeverText->SetText(B_TRANSLATE("Don't fade when mouse is here"));

	box->AddChild(BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING)
		.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
			.Add(startScreenSaver, 0, 0)
			.Add(fRunSlider, 1, 0)
			.Add(fTurnOffCheckBox, 0, 1)
			.Add(BLayoutBuilder::Group<>(B_VERTICAL)
				.Add(fTurnOffNotSupported)
				.Add(fTurnOffSlider)
				.View(), 1, 1)
			.Add(fPasswordCheckBox, 0, 2)
			.Add(fPasswordSlider, 1, 2)
			.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fPasswordButton)
			.End()
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.Add(fFadeNow)
			.AddGroup(B_VERTICAL, 0)
				.Add(fadeNowText)
				.AddGlue()
				.End()
			.Add(fFadeNever)
			.AddGroup(B_VERTICAL, 0)
				.Add(fadeNeverText)
				.AddGlue()
				.End()
			.End()
		.AddGlue()
		.View());

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_SMALL_SPACING)
		.Add(box)
		.End();
}


void
FadeView::AttachedToWindow()
{
	fEnableCheckBox->SetTarget(this);
	fRunSlider->SetTarget(this);
	fTurnOffCheckBox->SetTarget(this);
	fTurnOffSlider->SetTarget(this);
	fFadeNow->SetTarget(this);
	fFadeNever->SetTarget(this);
	fPasswordCheckBox->SetTarget(this);
	fPasswordSlider->SetTarget(this);

	fEnableCheckBox->SetValue(
		fSettings.TimeFlags() & ENABLE_SAVER ? B_CONTROL_ON : B_CONTROL_OFF);
	fRunSlider->SetTime(fSettings.BlankTime());
	fTurnOffSlider->SetTime(fSettings.OffTime() + fSettings.BlankTime());
	fFadeNow->SetCorner(fSettings.BlankCorner());
	fFadeNever->SetCorner(fSettings.NeverBlankCorner());
	fPasswordCheckBox->SetValue(fSettings.LockEnable());
	fPasswordSlider->SetTime(fSettings.PasswordTime());

	UpdateTurnOffScreen();
	UpdateStatus();
}


void
FadeView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgRunSliderChanged:
		case kMsgRunSliderUpdate:
			if (fRunSlider->Value() > fTurnOffSlider->Value())
				fTurnOffSlider->SetValue(fRunSlider->Value());

			if (fRunSlider->Value() > fPasswordSlider->Value())
				fPasswordSlider->SetValue(fRunSlider->Value());
			break;

		case kMsgTurnOffSliderChanged:
		case kMsgTurnOffSliderUpdate:
			if (fRunSlider->Value() > fTurnOffSlider->Value())
				fRunSlider->SetValue(fTurnOffSlider->Value());
			break;

		case kMsgPasswordSliderChanged:
		case kMsgPasswordSliderUpdate:
			if (fPasswordSlider->Value() < fRunSlider->Value())
				fRunSlider->SetValue(fPasswordSlider->Value());
			break;

		case kMsgTurnOffCheckBox:
			fTurnOffSlider->SetEnabled(
				fTurnOffCheckBox->Value() == B_CONTROL_ON);
			break;
	}

	switch (message->what) {
		case kMsgRunSliderChanged:
		case kMsgTurnOffSliderChanged:
		case kMsgPasswordSliderChanged:
		case kMsgPasswordCheckBox:
		case kMsgEnableScreenSaverBox:
		case kMsgFadeCornerChanged:
		case kMsgNeverFadeCornerChanged:
			UpdateStatus();
			fSettings.Save();
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
FadeView::UpdateTurnOffScreen()
{
	bool enabled = (fSettings.TimeFlags() & ENABLE_DPMS_MASK) != 0;

	BScreen screen(Window());
	uint32 dpmsCapabilities = screen.DPMSCapabilites();

	fTurnOffScreenFlags = 0;
	if (dpmsCapabilities & B_DPMS_OFF)
		fTurnOffScreenFlags |= ENABLE_DPMS_OFF;
	if (dpmsCapabilities & B_DPMS_STAND_BY)
		fTurnOffScreenFlags |= ENABLE_DPMS_STAND_BY;
	if (dpmsCapabilities & B_DPMS_SUSPEND)
		fTurnOffScreenFlags |= ENABLE_DPMS_SUSPEND;

	fTurnOffCheckBox->SetValue(enabled && fTurnOffScreenFlags != 0
		? B_CONTROL_ON : B_CONTROL_OFF);

	enabled = fEnableCheckBox->Value() == B_CONTROL_ON;
	fTurnOffCheckBox->SetEnabled(enabled && fTurnOffScreenFlags != 0);
	if (fTurnOffScreenFlags != 0) {
		fTurnOffNotSupported->Hide();
		fTurnOffSlider->Show();
	} else {
		fTurnOffSlider->Hide();
		fTurnOffNotSupported->Show();
	}
}


void
FadeView::UpdateStatus()
{
	Window()->DisableUpdates();

	bool enabled = fEnableCheckBox->Value() == B_CONTROL_ON;
	fPasswordCheckBox->SetEnabled(enabled);
	fTurnOffCheckBox->SetEnabled(enabled && fTurnOffScreenFlags != 0);
	fRunSlider->SetEnabled(enabled);
	fTurnOffSlider->SetEnabled(enabled && fTurnOffCheckBox->Value());
	fPasswordSlider->SetEnabled(enabled && fPasswordCheckBox->Value());
	fPasswordButton->SetEnabled(enabled && fPasswordCheckBox->Value());

	Window()->EnableUpdates();

	// Update the saved preferences
	fSettings.SetWindowFrame(Frame());
	fSettings.SetTimeFlags((enabled ? ENABLE_SAVER : 0)
		| (fTurnOffCheckBox->Value() ? fTurnOffScreenFlags : 0));
	fSettings.SetBlankTime(fRunSlider->Time());
	bigtime_t offTime = fTurnOffSlider->Time() - fSettings.BlankTime();
	fSettings.SetOffTime(offTime);
	fSettings.SetSuspendTime(offTime);
	fSettings.SetStandByTime(offTime);
	fSettings.SetBlankCorner(fFadeNow->Corner());
	fSettings.SetNeverBlankCorner(fFadeNever->Corner());
	fSettings.SetLockEnable(fPasswordCheckBox->Value());
	fSettings.SetPasswordTime(fPasswordSlider->Time());

	// TODO - Tell the password window to update its stuff
}


//	#pragma mark - ModulesView


ModulesView::ModulesView(const char* name, ScreenSaverSettings& settings)
	:
	BView(name, B_WILL_DRAW),
	fSettings(settings),
	fSaverRunner(NULL),
	fSettingsView(NULL),
	fScreenSaverTestTeam(-1)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTestButton = new BButton("TestButton", B_TRANSLATE("Test"),
		new BMessage(kMsgTestSaver));

	fAddButton = new BButton("AddButton",
		B_TRANSLATE("Add" B_UTF8_ELLIPSIS), new BMessage(kMsgAddSaver));

	fPreviewView = new PreviewView("preview");

	fScreenSaversListView = new BListView("SaversListView",
		B_SINGLE_SELECTION_LIST);
	fScreenSaversListView->SetSelectionMessage(
		new BMessage(kMsgSaverSelected));
	BScrollView* saversListScrollView = new BScrollView("scroll_list",
		fScreenSaversListView, 0, false, true);

	fSettingsBox = new BBox("SettingsBox");
	fSettingsBox->SetLabel(B_TRANSLATE("Screensaver settings"));

	fFilePanel = new BFilePanel();

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL)
			.Add(fPreviewView)
			.Add(saversListScrollView)
			.AddGroup(B_HORIZONTAL)
				.Add(fTestButton)
				.Add(fAddButton)
				.End()
			.End()
		.Add(fSettingsBox)
		.End();
}


ModulesView::~ModulesView()
{
	delete fFilePanel;
}


void
ModulesView::DetachedFromWindow()
{
	SaveState();
	EmptyScreenSaverList();

	_CloseSaver();
}


void
ModulesView::AttachedToWindow()
{
	fScreenSaversListView->SetTarget(this);
	fTestButton->SetTarget(this);
	fAddButton->SetTarget(this);
}


void
ModulesView::AllAttached()
{
	PopulateScreenSaverList();
	fScreenSaversListView->Invoke(new BMessage(kMsgSaverSelected));
}


void
ModulesView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSaverSelected:
		{
			int32 selection = fScreenSaversListView->CurrentSelection();
			if (selection < 0)
				break;

			ScreenSaverItem* item
				= (ScreenSaverItem*)fScreenSaversListView->ItemAt(selection);
			if (item == NULL)
				break;

			if (strcmp(item->Text(), B_TRANSLATE("Blackness")) == 0)
				fSettings.SetModuleName("");
			else
				fSettings.SetModuleName(item->Text());

			SaveState();
			_CloseSaver();
			_OpenSaver();
			fSettings.Save();
			break;
		}

		case kMsgTestSaver:
		{
			SaveState();
			fSettings.Save();

			_CloseSaver();

			be_roster->StartWatching(BMessenger(this, Looper()),
				B_REQUEST_QUIT);
			if (be_roster->Launch(SCREEN_BLANKER_SIG, &fSettings.Message(),
					&fScreenSaverTestTeam) == B_OK) {
				break;
			}

			// Try really hard to launch it. It's very likely that this fails
			// when we run from the CD, and there is only an incomplete mime
			// database for example...
			BPath path;
			if (find_directory(B_SYSTEM_BIN_DIRECTORY, &path) != B_OK
				|| path.Append("screen_blanker") != B_OK) {
				path.SetTo("/bin/screen_blanker");
			}

			BEntry entry(path.Path());
			entry_ref ref;
			if (entry.GetRef(&ref) == B_OK) {
				be_roster->Launch(&ref, &fSettings.Message(),
					&fScreenSaverTestTeam);
			}
			break;
		}

		case kMsgAddSaver:
			fFilePanel->Show();
			break;

		case B_SOME_APP_QUIT:
		{
			team_id team;
			if (message->FindInt32("be:team", &team) == B_OK
				&& team == fScreenSaverTestTeam) {
				be_roster->StopWatching(this);
				_OpenSaver();
			}
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


void
ModulesView::SaveState()
{
	BScreenSaver* saver = _ScreenSaver();
	if (saver == NULL)
		return;

	BMessage state;
	if (saver->SaveState(&state) == B_OK)
		fSettings.SetModuleState(fCurrentName.String(), &state);
}


void
ModulesView::EmptyScreenSaverList()
{
	fScreenSaversListView->DeselectAll();
	while (BListItem* item = fScreenSaversListView->RemoveItem((int32)0))
		delete item;
}


void
ModulesView::PopulateScreenSaverList()
{
	// Blackness is a built-in screen saver
	ScreenSaverItem* defaultItem
		= new ScreenSaverItem(B_TRANSLATE("Blackness"), "");
	fScreenSaversListView->AddItem(defaultItem);

	// Iterate over add-on directories, and add their files to the list view

	directory_which which[] = {
		B_USER_NONPACKAGED_ADDONS_DIRECTORY,
		B_USER_ADDONS_DIRECTORY,
		B_SYSTEM_NONPACKAGED_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY,
	};
	ScreenSaverItem* selectedItem = NULL;

	for (uint32 i = 0; i < sizeof(which) / sizeof(which[0]); i++) {
		BPath basePath;
		find_directory(which[i], &basePath);
		basePath.Append("Screen Savers", true);

		BDirectory dir(basePath.Path());
		BEntry entry;
		while (dir.GetNextEntry(&entry, true) == B_OK) {
			char name[B_FILE_NAME_LENGTH];
			if (entry.GetName(name) != B_OK)
				continue;

			BPath path = basePath;
			path.Append(name);

			ScreenSaverItem* item = new ScreenSaverItem(name, path.Path());
			fScreenSaversListView->AddItem(item);

			if (selectedItem != NULL)
				continue;

			if (strcmp(fSettings.ModuleName(), item->Text()) == 0)
				selectedItem = item;
		}
	}

	fScreenSaversListView->SortItems(_CompareScreenSaverItems);
	if (selectedItem == NULL)
		selectedItem = defaultItem;

	fScreenSaversListView->Select(fScreenSaversListView->IndexOf(selectedItem));
	fScreenSaversListView->ScrollToSelection();
}


//! Sorting function for ScreenSaverItems
int
ModulesView::_CompareScreenSaverItems(const void* left, const void* right)
{
	ScreenSaverItem* leftItem  = *(ScreenSaverItem **)left;
	ScreenSaverItem* rightItem = *(ScreenSaverItem **)right;

	return strcasecmp(leftItem->Text(), rightItem->Text());
}


BScreenSaver*
ModulesView::_ScreenSaver()
{
	if (fSaverRunner != NULL)
		return fSaverRunner->ScreenSaver();

	return NULL;
}


void
ModulesView::_CloseSaver()
{
	// remove old screen saver preview & config

	BScreenSaver* saver = _ScreenSaver();
	BView* view = fPreviewView->RemovePreview();
	if (fSettingsView != NULL)
		fSettingsBox->RemoveChild(fSettingsView);

	if (fSaverRunner != NULL)
		fSaverRunner->Quit();

	if (saver != NULL)
		saver->StopConfig();

	delete view;
	delete fSettingsView;
	delete fSaverRunner;
		// the saver runner also unloads the add-on, so it must
		// be deleted last

	fSettingsView = NULL;
	fSaverRunner = NULL;
}


void
ModulesView::_OpenSaver()
{
	// create new screen saver preview & config

	BView* view = fPreviewView->AddPreview();
	fCurrentName = fSettings.ModuleName();
	fSaverRunner = new ScreenSaverRunner(Window(), view, true, fSettings);
	BScreenSaver* saver = _ScreenSaver();

#ifdef __HAIKU__
	BRect rect = fSettingsBox->InnerFrame().InsetByCopy(4, 4);
#else
	BRect rect = fSettingsBox->Bounds().InsetByCopy(4, 4);
	rect.top += 14;
#endif
	fSettingsView = new BView(rect, "SettingsView", B_FOLLOW_ALL, B_WILL_DRAW);

	fSettingsView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fSettingsBox->AddChild(fSettingsView);

	if (saver != NULL && fSaverRunner->Run() == B_OK)
		saver->StartConfig(fSettingsView);

	if (fSettingsView->ChildAt(0) == NULL) {
		// There are no settings at all, we add the module name here to
		// let it look a bit better at least.
		BPrivate::BuildScreenSaverDefaultSettingsView(fSettingsView,
			fSettings.ModuleName()[0] ? fSettings.ModuleName()
				: B_TRANSLATE("Blackness"),
				saver != NULL || !fSettings.ModuleName()[0]
					? B_TRANSLATE("No options available")
					: B_TRANSLATE("Could not load screen saver"));
	}
}


//	#pragma mark - ScreenSaverWindow


ScreenSaverWindow::ScreenSaverWindow()
	:
	BWindow(BRect(50, 50, 496, 375), B_TRANSLATE_SYSTEM_NAME("ScreenSaver"),
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fSettings.Load();

	fMinWidth = ceilf(std::max(446.0f,
		be_control_look->DefaultItemSpacing() * 44.6f));

	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float textHeight = ceilf(fontHeight.ascent + fontHeight.descent);

	fMinHeight = ceilf(std::max(325.0f, textHeight * 28));

	// Create the password editing window
	fPasswordWindow = new PasswordWindow(fSettings);
	fPasswordWindow->Run();

	// Create the tab view
	fTabView = new BTabView("tab_view", B_WIDTH_FROM_LABEL);
	fTabView->SetBorder(B_NO_BORDER);

	// Create the controls inside the tabs
	fFadeView = new FadeView(B_TRANSLATE("General"), fSettings);
	fModulesView = new ModulesView(B_TRANSLATE("Screensavers"), fSettings);

	fTabView->AddTab(fFadeView);
	fTabView->AddTab(fModulesView);

	// Create the topmost background view
	BView* topView = new BView("topView", B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	topView->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_USE_FULL_HEIGHT));
	topView->SetExplicitMinSize(BSize(fMinWidth, fMinHeight));
	BLayoutBuilder::Group<>(topView, B_VERTICAL)
		.SetInsets(0, B_USE_SMALL_SPACING, 0, 0)
		.Add(fTabView)
		.End();

	SetLayout(new BGroupLayout(B_VERTICAL));
	GetLayout()->AddView(topView);

	fTabView->Select(fSettings.WindowTab());

	if (fSettings.WindowFrame().left > 0 && fSettings.WindowFrame().top > 0)
		MoveTo(fSettings.WindowFrame().left, fSettings.WindowFrame().top);

	if (fSettings.WindowFrame().Width() > 0
		&& fSettings.WindowFrame().Height() > 0) {
		ResizeTo(fSettings.WindowFrame().Width(),
			fSettings.WindowFrame().Height());
	}
}


ScreenSaverWindow::~ScreenSaverWindow()
{
	Hide();
	fFadeView->UpdateStatus();
	fSettings.SetWindowTab(fTabView->Selection());

	delete fTabView->RemoveTab(1);
		// We delete this here in order to make sure the module view saves its
		// state while the window is still intact.

	fSettings.Save();
}


void
ScreenSaverWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgChangePassword:
			fPasswordWindow->CenterIn(Frame());
			fPasswordWindow->Show();
			break;

		case kMsgUpdateList:
			fModulesView->EmptyScreenSaverList();
			fModulesView->PopulateScreenSaverList();
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


void
ScreenSaverWindow::ScreenChanged(BRect frame, color_space colorSpace)
{
	fFadeView->UpdateTurnOffScreen();
}


bool
ScreenSaverWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
