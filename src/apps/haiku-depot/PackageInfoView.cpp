/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfoView.h"

#include <algorithm>
#include <stdio.h>

#include <Bitmap.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <StringView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageInfoView"


class BitmapView : public BView {
public:
	BitmapView(const char* name)
		:
		BView(name, B_WILL_DRAW),
		fBitmap(NULL)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetDrawingMode(B_OP_OVER);
	}
	
	virtual ~BitmapView()
	{
		delete fBitmap;
	}

	virtual void Draw(BRect updateRect)
	{
		BRect bounds(Bounds());
		FillRect(updateRect, B_SOLID_LOW);
		
		if (fBitmap == NULL)
			return;

		DrawBitmap(fBitmap, fBitmap->Bounds(), bounds);
	}

	virtual BSize MinSize()
	{
		BSize size(0.0f, 0.0f);
		
		if (fBitmap != NULL) {
			BRect bounds = fBitmap->Bounds();
			size.width = bounds.Width();
			size.height = bounds.Height();
		}
		
		return size;
	}

	virtual BSize MaxSize()
	{
		return MinSize();
	}
	
	void SetBitmap(BBitmap* bitmap)
	{
		if (bitmap == fBitmap)
			return;

		BSize size = MinSize();

		delete fBitmap;
		fBitmap = bitmap;
		
		BSize newSize = MinSize();
		if (size != newSize)
			InvalidateLayout();
		
		Invalidate();
	}

private:
	BBitmap*	fBitmap;
};


// #pragma mark - TitleView


class TitleView : public BGroupView {
public:
	TitleView()
		:
		BGroupView("title view", B_HORIZONTAL)
	{
		fIconView = new BitmapView("package icon view");
		fTitleView = new BStringView("package title view", "");

		// Title font
		BFont font;
		GetFont(&font);
		font.SetSize(font.Size() * 2.0f);
		fTitleView->SetFont(&font);
	
		BLayoutBuilder::Group<>(this)
			.Add(fIconView)
			.Add(fTitleView)
			.AddGlue()
		;
	}
	
	virtual ~TitleView()
	{
		Clear();
	}

	void SetPackage(const PackageInfo& package)
	{
		// TODO: Fetch icon
		fTitleView->SetText(package.Title());
		InvalidateLayout();
		Invalidate();
	}

	void Clear()
	{
		fTitleView->SetText("");
	}

private:
	BitmapView*		fIconView;
	BStringView*	fTitleView;
};


// #pragma mark - AboutView


class AboutView : public BView {
public:
	AboutView()
		:
		BView("about view", B_WILL_DRAW),
		fLayout(new BGroupLayout(B_VERTICAL))
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		
		SetLayout(fLayout);
		
		fDescriptionView = new BTextView("description view");
		fDescriptionView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fDescriptionView->MakeEditable(false);
		
		BLayoutBuilder::Group<>(fLayout)
			.Add(fDescriptionView)
		;
	}
	
	virtual ~AboutView()
	{
		Clear();
	}

	virtual void Draw(BRect updateRect)
	{
	}

	void SetPackage(const PackageInfo& package)
	{
		fDescriptionView->SetText(package.FullDescription());
	}

	void Clear()
	{
		fDescriptionView->SetText("");
	}

private:
	BGroupLayout*	fLayout;
	BTextView*		fDescriptionView;
	
};


// #pragma mark - PagesView


class PagesView : public BView {
public:
	PagesView()
		:
		BView("pages view", B_WILL_DRAW),
		fLayout(new BCardLayout())
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetLayout(fLayout);
		
		fAboutView = new AboutView();
		
		fLayout->AddView(fAboutView);
		
		fLayout->SetVisibleItem(0L);
	}
	
	virtual ~PagesView()
	{
		Clear();
	}

	virtual void Draw(BRect updateRect)
	{
	}

	void SetPackage(const PackageInfo& package)
	{
		fAboutView->SetPackage(package);
	}

	void Clear()
	{
		fAboutView->Clear();
	}

private:
	BCardLayout*	fLayout;
	
	AboutView*		fAboutView;
};


// #pragma mark - PackageInfoView


PackageInfoView::PackageInfoView()
	:
	BGroupView("package info view", B_VERTICAL)
{
	fTitleView = new TitleView();
	fPagesView = new PagesView();

	BLayoutBuilder::Group<>(this)
		.Add(fTitleView, 0.0f)
		.Add(fPagesView)
	;
}


PackageInfoView::~PackageInfoView()
{
}


void
PackageInfoView::AttachedToWindow()
{
}


void
PackageInfoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
PackageInfoView::SetPackage(const PackageInfo& package)
{
	fTitleView->SetPackage(package);
	fPagesView->SetPackage(package);
}


void
PackageInfoView::Clear()
{
	fTitleView->Clear();
	fPagesView->Clear();
}
