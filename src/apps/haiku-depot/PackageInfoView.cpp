/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfoView.h"

#include <algorithm>
#include <stdio.h>

#include <Autolock.h>
#include <Bitmap.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <Cursor.h>
#include <Font.h>
#include <GridView.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <TabView.h>
#include <ScrollView.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>

#include <support/Url.h>

#include "BitmapButton.h"
#include "BitmapView.h"
#include "MarkupParser.h"
#include "PackageActionHandler.h"
#include "PackageManager.h"
#include "TextDocumentView.h"
#include "TextView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageInfoView"


static const rgb_color kLightBlack = (rgb_color){ 60, 60, 60, 255 };
static const float kContentTint = (B_NO_TINT + B_LIGHTEN_1_TINT) / 2.0f;


//! Layouts the scrollbar so it looks nice with no border and the document
// window look.
class CustomScrollView : public BScrollView {
public:
	CustomScrollView(const char* name, BView* target)
		:
		BScrollView(name, target, 0, false, true, B_NO_BORDER)
	{
	}

	virtual void DoLayout()
	{
		BRect innerFrame = Bounds();
		innerFrame.right -= B_V_SCROLL_BAR_WIDTH + 1;

		BView* target = Target();
		if (target != NULL) {
			Target()->MoveTo(innerFrame.left, innerFrame.top);
			Target()->ResizeTo(innerFrame.Width(), innerFrame.Height());
		}

		BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
		if (scrollBar != NULL) {
			BRect rect = innerFrame;
			rect.left = rect.right + 1;
			rect.right = rect.left + B_V_SCROLL_BAR_WIDTH;
			rect.bottom -= B_H_SCROLL_BAR_HEIGHT;

			scrollBar->MoveTo(rect.left, rect.top);
			scrollBar->ResizeTo(rect.Width(), rect.Height());
		}
	}
};


class RatingsScrollView : public CustomScrollView {
public:
	RatingsScrollView(const char* name, BView* target)
		:
		CustomScrollView(name, target)
	{
	}

	virtual void DoLayout()
	{
		CustomScrollView::DoLayout();

		BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
		BView* target = Target();
		if (target != NULL && scrollBar != NULL) {
			// Set the scroll steps
			BView* item = target->ChildAt(0);
			if (item != NULL) {
				scrollBar->SetSteps(item->MinSize().height + 1,
					item->MinSize().height + 1);
			}
		}
	}
};


class MarkupTextView : public TextDocumentView {
public:
	MarkupTextView(const char* name)
		:
		TextDocumentView(name)
	{
		CharacterStyle regularStyle;

		float fontSize = regularStyle.Font().Size();

		ParagraphStyle paragraphStyle;
		paragraphStyle.SetJustify(true);
		paragraphStyle.SetSpacingTop(ceilf(fontSize * 0.3f));
		paragraphStyle.SetLineSpacing(ceilf(fontSize * 0.2f));

		fMarkupParser.SetStyles(regularStyle, paragraphStyle);
	}

	void SetText(const BString& markupText) {
		SetTextDocument(fMarkupParser.CreateDocumentFromMarkup(markupText));
	}

	void SetText(const BString heading, const BString& markupText) {
		TextDocumentRef document(new(std::nothrow) TextDocument(), true);

		Paragraph paragraph(fMarkupParser.HeadingParagraphStyle());
		paragraph.Append(TextSpan(heading,
			fMarkupParser.HeadingCharacterStyle()));
		document->Append(paragraph);

		fMarkupParser.AppendMarkup(document, markupText);

		SetTextDocument(document);
	}

private:
	MarkupParser	fMarkupParser;
};


class LinkView : public BStringView, public BInvoker {
public:
	LinkView(const char* name, const char* string, BMessage* message,
		rgb_color color)
		:
		BStringView(name, string),
		BInvoker(message, NULL),
		fNormalColor(color),
		fHoverColor(make_color(1, 141, 211))
	{
	}

	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
	{
		if (transit == B_ENTERED_VIEW) {
			SetHighColor(fHoverColor);
			BCursor cursor(B_CURSOR_ID_FOLLOW_LINK);
			SetViewCursor(&cursor, true);
			Invalidate();
		} else if (transit == B_EXITED_VIEW) {
			SetHighColor(fNormalColor);
			SetViewCursor(NULL);
			Invalidate();
		}
	}

	virtual void MouseDown(BPoint where)
	{
		Invoke(Message());
	}

private:
	rgb_color	fNormalColor;
	rgb_color	fHoverColor;
};


// #pragma mark - AboutView


class RatingView : public BView {
public:
	RatingView()
		:
		BView("package rating view", B_WILL_DRAW),
		fStarBitmap(501),
		fRating(-1.0f)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}

	virtual ~RatingView()
	{
	}

	virtual void AttachedToWindow()
	{
		BView* parent = Parent();
		if (parent != NULL)
			SetLowColor(parent->ViewColor());
	}

	virtual void Draw(BRect updateRect)
	{
		FillRect(updateRect, B_SOLID_LOW);

		if (fRating < 0.0f)
			return;

		const BBitmap* star = fStarBitmap.Bitmap(SharedBitmap::SIZE_16);
		if (star == NULL) {
			fprintf(stderr, "No star icon found in application resources.\n");
			return;
		}

		SetDrawingMode(B_OP_OVER);

		float x = 0;
		for (int i = 0; i < 5; i++) {
			DrawBitmap(star, BPoint(x, 0));
			x += 16 + 2;
		}

		if (fRating >= 5.0f)
			return;

		SetDrawingMode(B_OP_OVER);

		BRect rect(Bounds());
		rect.left = ceilf(rect.left + (fRating / 5.0f) * rect.Width());

		rgb_color color = LowColor();
		color.alpha = 190;
		SetHighColor(color);

		SetDrawingMode(B_OP_ALPHA);
		FillRect(rect, B_SOLID_HIGH);
	}

	virtual BSize MinSize()
	{
		return BSize(16 * 5 + 2 * 4, 16 + 2);
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual BSize MaxSize()
	{
		return MinSize();
	}

	void SetRating(float rating)
	{
		fRating = rating;
		Invalidate();
	}

private:
	SharedBitmap	fStarBitmap;
	float			fRating;
};


class DiagramBarView : public BView {
public:
	DiagramBarView()
		:
		BView("diagram bar view", B_WILL_DRAW),
		fValue(0.0f)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetHighColor(tint_color(LowColor(), B_DARKEN_2_TINT));
	}

	virtual ~DiagramBarView()
	{
	}

	virtual void AttachedToWindow()
	{
		BView* parent = Parent();
		if (parent != NULL) {
			SetLowColor(parent->ViewColor());
			SetHighColor(tint_color(LowColor(), B_DARKEN_2_TINT));
		}
	}

	virtual void Draw(BRect updateRect)
	{
		FillRect(updateRect, B_SOLID_LOW);

		if (fValue <= 0.0f)
			return;

		BRect rect(Bounds());
		rect.right = ceilf(rect.left + fValue * rect.Width());

		FillRect(rect, B_SOLID_HIGH);
	}

	virtual BSize MinSize()
	{
		return BSize(64, 10);
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual BSize MaxSize()
	{
		return BSize(64, 10);
	}

	void SetValue(float value)
	{
		if (fValue != value) {
			fValue = value;
			Invalidate();
		}
	}

private:
	float			fValue;
};


// #pragma mark - TitleView


enum {
	MSG_PACKAGE_ACTION			= 'pkga',
};


class TitleView : public BGroupView {
public:
	TitleView()
		:
		BGroupView("title view", B_HORIZONTAL)
	{
		fIconView = new BitmapView("package icon view");
		fTitleView = new BStringView("package title view", "");
		fPublisherView = new BStringView("package publisher view", "");

		// Title font
		BFont font;
		GetFont(&font);
		font_family family;
		font_style style;
		font.SetSize(ceilf(font.Size() * 1.5f));
		font.GetFamilyAndStyle(&family, &style);
		font.SetFamilyAndStyle(family, "Bold");
		fTitleView->SetFont(&font);

		// Publisher font
		GetFont(&font);
		font.SetSize(std::max(9.0f, floorf(font.Size() * 0.92f)));
		font.SetFamilyAndStyle(family, "Italic");
		fPublisherView->SetFont(&font);
		fPublisherView->SetHighColor(kLightBlack);
		fPublisherView->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT,
			B_ALIGN_VERTICAL_UNSET));

		// slightly bigger font
		GetFont(&font);
		font.SetSize(ceilf(font.Size() * 1.2f));

		// Version info
		fVersionInfo = new BStringView("package version info", "");
		fVersionInfo->SetFont(&font);
		fVersionInfo->SetHighColor(kLightBlack);

		// Rating view
		fRatingView = new RatingView();

		fAvgRating = new BStringView("package average rating", "");
		fAvgRating->SetFont(&font);
		fAvgRating->SetHighColor(kLightBlack);

		fVoteInfo = new BStringView("package vote info", "");
		// small font
		GetFont(&font);
		font.SetSize(std::max(9.0f, floorf(font.Size() * 0.85f)));
		fVoteInfo->SetFont(&font);
		fVoteInfo->SetHighColor(kLightBlack);

		BLayoutBuilder::Group<>(this)
			.Add(fIconView)
			.AddGroup(B_VERTICAL, 1.0f)
				.Add(fTitleView)
				.Add(fPublisherView)
			.End()
			.AddGlue(0.1f)
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
				.Add(fRatingView)
				.Add(fAvgRating)
				.Add(fVoteInfo)
			.End()
			.AddGlue(0.2f)
			.Add(fVersionInfo)
			.AddGlue(3.0f)
		;

		Clear();
	}

	virtual ~TitleView()
	{
	}

	void SetPackage(const PackageInfo& package)
	{
		if (package.Icon().Get() != NULL)
			fIconView->SetBitmap(package.Icon()->Bitmap(SharedBitmap::SIZE_32));
		else
			fIconView->SetBitmap(NULL);

		fTitleView->SetText(package.Title());

		BString publisher = package.Publisher().Name();
		fPublisherView->SetText(publisher);

		BString version = B_TRANSLATE("%Version%");
		version.ReplaceAll("%Version%", package.Version());
		fVersionInfo->SetText(version);

		RatingSummary ratingSummary = package.CalculateRatingSummary();

		fRatingView->SetRating(ratingSummary.averageRating);

		if (ratingSummary.ratingCount > 0) {
			BString avgRating;
			avgRating.SetToFormat("%.1f", ratingSummary.averageRating);
			fAvgRating->SetText(avgRating);

			BString votes;
			votes.SetToFormat("%d", ratingSummary.ratingCount);

			BString voteInfo(B_TRANSLATE("(%Votes%)"));
			voteInfo.ReplaceAll("%Votes%", votes);

			fVoteInfo->SetText(voteInfo);
		} else {
			fAvgRating->SetText("");
			fVoteInfo->SetText(B_TRANSLATE("n/a"));
		}

		InvalidateLayout();
		Invalidate();
	}

	void Clear()
	{
		fIconView->SetBitmap(NULL);
		fTitleView->SetText("");
		fPublisherView->SetText("");
		fVersionInfo->SetText("");
		fRatingView->SetRating(-1.0f);
		fAvgRating->SetText("");
		fVoteInfo->SetText("");
	}

private:
	BitmapView*		fIconView;

	BStringView*	fTitleView;
	BStringView*	fPublisherView;

	BStringView*	fVersionInfo;

	RatingView*		fRatingView;
	BStringView*	fAvgRating;
	BStringView*	fVoteInfo;
};


// #pragma mark - PackageActionView


class PackageActionView : public BView {
public:
	PackageActionView(PackageActionHandler* handler)
		:
		BView("about view", B_WILL_DRAW),
		fLayout(new BGroupLayout(B_HORIZONTAL)),
		fPackageActionHandler(handler)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		SetLayout(fLayout);
	}

	virtual ~PackageActionView()
	{
		Clear();
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_PACKAGE_ACTION:
			{
				int32 index;
				if (message->FindInt32("index", &index) == B_OK) {
					const PackageActionRef& action
						= fPackageActions.ItemAt(index);
					if (action.Get() != NULL) {
						PackageActionList actions;
						actions.Add(action);
						status_t result = fPackageActionHandler
							->SchedulePackageActions(actions);
						if (result != B_OK) {
							fprintf(stderr, "Failed to schedule action: "
								"%s '%s'\n", action->Label(),
								action->Package()->Title().String());
						}
					}
				}
				break;
			}

			default:
				BView::MessageReceived(message);
				break;
		}
	}

	void SetPackage(const PackageInfo& package)
	{
		PackageManager manager(
			BPackageKit::B_PACKAGE_INSTALLATION_LOCATION_HOME);

		PackageActionList actions = manager.GetPackageActions(
			const_cast<PackageInfo*>(&package),
			fPackageActionHandler->GetModel());

		bool clearNeeded = false;
		if (actions.CountItems() != fPackageActions.CountItems())
			clearNeeded = true;
		else {
			for (int32 i = 0; i < actions.CountItems(); i++) {
				if (actions.ItemAtFast(i)->Type()
					!= fPackageActions.ItemAtFast(i)->Type()) {
					clearNeeded = true;
					break;
				}
			}
		}

		fPackageActions = actions;
		if (!clearNeeded)
			return;

		Clear();

		// Add Buttons in reverse action order
		for (int32 i = fPackageActions.CountItems() - 1; i >= 0; i--) {
			const PackageActionRef& action = fPackageActions.ItemAtFast(i);

			BMessage* message = new BMessage(MSG_PACKAGE_ACTION);
			message->AddInt32("index", i);

			BButton* button = new BButton(action->Label(), message);
			fLayout->AddView(button);
			button->SetTarget(this);

			fButtons.AddItem(button);
		}
	}

	void Clear()
	{
		for (int32 i = fButtons.CountItems() - 1; i >= 0; i--) {
			BButton* button = (BButton*)fButtons.ItemAtFast(i);
			button->RemoveSelf();
			delete button;
		}
		fButtons.MakeEmpty();
	}

private:
	BGroupLayout*		fLayout;
	PackageActionList	fPackageActions;
	PackageActionHandler* fPackageActionHandler;
	BList				fButtons;
};


// #pragma mark - AboutView


enum {
	MSG_EMAIL_PUBLISHER				= 'emlp',
	MSG_VISIT_PUBLISHER_WEBSITE		= 'vpws',
};


class AboutView : public BView {
public:
	AboutView()
		:
		BView("about view", 0),
		fEmailIcon("text/x-email"),
		fWebsiteIcon("text/html")
	{
		SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			kContentTint));

		fDescriptionView = new MarkupTextView("description view");
		fDescriptionView->SetLowColor(ViewColor());
		fDescriptionView->SetInsets(be_plain_font->Size());

		BScrollView* scrollView = new CustomScrollView(
			"description scroll view", fDescriptionView);

		BFont smallFont;
		GetFont(&smallFont);
		smallFont.SetSize(std::max(9.0f, ceilf(smallFont.Size() * 0.85f)));

		// TODO: Clicking the screen shot view should open ShowImage with the
		// the screen shot. This could be done by writing the screen shot to
		// a temporary folder, launching ShowImage to display it, and writing
		// all other screenshots associated with the package to the same folder
		// so the user can use the ShowImage navigation to view the other
		// screenshots.
		fScreenshotView = new BitmapView("screenshot view");
		fScreenshotView->SetExplicitMinSize(BSize(64.0f, 64.0f));
		fScreenshotView->SetExplicitMaxSize(
			BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		fScreenshotView->SetExplicitAlignment(
			BAlignment(B_ALIGN_CENTER, B_ALIGN_TOP));

		fEmailIconView = new BitmapView("email icon view");
		fEmailLinkView = new LinkView("email link view", "",
			new BMessage(MSG_EMAIL_PUBLISHER), kLightBlack);
		fEmailLinkView->SetFont(&smallFont);

		fWebsiteIconView = new BitmapView("website icon view");
		fWebsiteLinkView = new LinkView("website link view", "",
			new BMessage(MSG_VISIT_PUBLISHER_WEBSITE), kLightBlack);
		fWebsiteLinkView->SetFont(&smallFont);

		BGroupView* leftGroup = new BGroupView(B_VERTICAL,
			B_USE_DEFAULT_SPACING);
		leftGroup->SetViewColor(ViewColor());

		BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0.0f)
//			.Add(BSpaceLayoutItem::CreateHorizontalStrut(32.0f))
			.AddGroup(leftGroup)
				.Add(fScreenshotView)
				.AddGroup(B_HORIZONTAL)
//					.AddGlue()
					.AddGrid(B_USE_HALF_ITEM_SPACING, B_USE_HALF_ITEM_SPACING)
						.Add(fEmailIconView, 0, 0)
						.Add(fEmailLinkView, 1, 0)
						.Add(fWebsiteIconView, 0, 1)
						.Add(fWebsiteLinkView, 1, 1)
					.End()
				.End()
				.SetInsets(B_USE_DEFAULT_SPACING)
			.End()
			.Add(scrollView, 1.0f)

			.SetExplicitMaxSize(BSize(B_SIZE_UNSET, B_SIZE_UNLIMITED))
			.SetInsets(0.0f, -1.0f, -1.0f, -1.0f)
		;
	}

	virtual ~AboutView()
	{
		Clear();
	}

	virtual void AttachedToWindow()
	{
		fWebsiteLinkView->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_VISIT_PUBLISHER_WEBSITE:
			{
				BPrivate::Support::BUrl url(fWebsiteLinkView->Text());
				url.OpenWithPreferredApplication();
				break;
			}

			default:
				BView::MessageReceived(message);
				break;
		}
	}

	void SetPackage(const PackageInfo& package)
	{
		fDescriptionView->SetText(package.ShortDescription(),
			package.FullDescription());

		fEmailIconView->SetBitmap(fEmailIcon.Bitmap(SharedBitmap::SIZE_16));
		fEmailLinkView->SetText(package.Publisher().Email());
		fWebsiteIconView->SetBitmap(fWebsiteIcon.Bitmap(SharedBitmap::SIZE_16));
		fWebsiteLinkView->SetText(package.Publisher().Website());

		const BBitmap* screenshot = NULL;
		const BitmapList& screenShots = package.Screenshots();
		if (screenShots.CountItems() > 0) {
			const BitmapRef& bitmapRef = screenShots.ItemAtFast(0);
			if (bitmapRef.Get() != NULL)
				screenshot = bitmapRef->Bitmap(SharedBitmap::SIZE_ANY);
		}
		fScreenshotView->SetBitmap(screenshot);
	}

	void Clear()
	{
		fDescriptionView->SetText("");
		fEmailIconView->SetBitmap(NULL);
		fEmailLinkView->SetText("");
		fWebsiteIconView->SetBitmap(NULL);
		fWebsiteLinkView->SetText("");

		fScreenshotView->SetBitmap(NULL);
	}

private:
	MarkupTextView*		fDescriptionView;

	BitmapView*			fScreenshotView;

	SharedBitmap		fEmailIcon;
	BitmapView*			fEmailIconView;
	BStringView*		fEmailLinkView;

	SharedBitmap		fWebsiteIcon;
	BitmapView*			fWebsiteIconView;
	LinkView*			fWebsiteLinkView;
};


// #pragma mark - UserRatingsView


class RatingItemView : public BGroupView {
public:
	RatingItemView(const UserRating& rating, const BitmapRef& voteUpIcon,
		const BitmapRef& voteDownIcon)
		:
		BGroupView(B_HORIZONTAL, 0.0f)
	{
		SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			kContentTint));

		fAvatarView = new BitmapView("avatar view");
		if (rating.User().Avatar().Get() != NULL) {
			fAvatarView->SetBitmap(
				rating.User().Avatar()->Bitmap(SharedBitmap::SIZE_16));
		}
		fAvatarView->SetExplicitMinSize(BSize(16.0f, 16.0f));

		fNameView = new BStringView("user name", rating.User().NickName());
		BFont nameFont(be_bold_font);
		nameFont.SetSize(std::max(9.0f, floorf(nameFont.Size() * 0.9f)));
		fNameView->SetFont(&nameFont);
		fNameView->SetHighColor(kLightBlack);
		fNameView->SetExplicitMaxSize(
			BSize(nameFont.StringWidth("xxxxxxxxxxxxxxxxxxxxxx"), B_SIZE_UNSET));

		fRatingView = new RatingView();
		fRatingView->SetRating(rating.Rating());

		BString ratingLabel;
		ratingLabel.SetToFormat("%.1f", rating.Rating());
		fRatingLabelView = new BStringView("rating label", ratingLabel);

		BString versionLabel(B_TRANSLATE("for %Version%"));
		versionLabel.ReplaceAll("%Version%", rating.PackageVersion());
		fPackageVersionView = new BStringView("package version",
			versionLabel);
		BFont versionFont(be_plain_font);
		versionFont.SetSize(std::max(9.0f, floorf(versionFont.Size() * 0.85f)));
		fPackageVersionView->SetFont(&versionFont);
		fPackageVersionView->SetHighColor(kLightBlack);

		// TODO: User rating IDs to identify which rating to vote up or down
		BMessage* voteUpMessage = new BMessage(MSG_VOTE_UP);
		voteUpMessage->AddInt32("rating id", -1);
		BMessage* voteDownMessage = new BMessage(MSG_VOTE_DOWN);
		voteDownMessage->AddInt32("rating id", -1);

		fVoteUpIconView = new BitmapButton("vote up icon", voteUpMessage);
		fUpVoteCountView = new BStringView("up vote count", "");
		fVoteDownIconView = new BitmapButton("vote down icon", voteDownMessage);
		fDownVoteCountView = new BStringView("up vote count", "");

		fVoteUpIconView->SetBitmap(
			voteUpIcon->Bitmap(SharedBitmap::SIZE_16));
		fVoteDownIconView->SetBitmap(
			voteDownIcon->Bitmap(SharedBitmap::SIZE_16));

		fUpVoteCountView->SetFont(&versionFont);
		fUpVoteCountView->SetHighColor(kLightBlack);
		fDownVoteCountView->SetFont(&versionFont);
		fDownVoteCountView->SetHighColor(kLightBlack);

		BString voteCountLabel;
		voteCountLabel.SetToFormat("%" B_PRId32, rating.UpVotes());
		fUpVoteCountView->SetText(voteCountLabel);
		voteCountLabel.SetToFormat("%" B_PRId32, rating.DownVotes());
		fDownVoteCountView->SetText(voteCountLabel);

		fTextView = new TextView("rating text");
		fTextView->SetViewColor(ViewColor());
		ParagraphStyle paragraphStyle(fTextView->ParagraphStyle());
		paragraphStyle.SetJustify(true);
		fTextView->SetParagraphStyle(paragraphStyle);

		fTextView->SetText(rating.Comment());

		BLayoutBuilder::Group<>(this)
			.Add(fAvatarView, 0.2f)
			.AddGroup(B_VERTICAL, 0.0f)
				.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
					.Add(fNameView)
					.Add(fRatingView)
					.Add(fRatingLabelView)
					.AddGlue(0.1f)
					.Add(fPackageVersionView)
					.AddGlue(5.0f)
					.AddGroup(B_HORIZONTAL, 0.0f, 0.0f)
						.Add(fVoteUpIconView)
						.Add(fUpVoteCountView)
						.AddStrut(B_USE_HALF_ITEM_SPACING)
						.Add(fVoteDownIconView)
						.Add(fDownVoteCountView)
					.End()
				.End()
				.Add(fTextView)
			.End()
			.SetInsets(B_USE_DEFAULT_SPACING)
		;
	}

private:
	BitmapView*		fAvatarView;
	BStringView*	fNameView;
	RatingView*		fRatingView;
	BStringView*	fRatingLabelView;
	BStringView*	fPackageVersionView;

	BitmapView*		fVoteUpIconView;
	BStringView*	fUpVoteCountView;
	BitmapView*		fVoteDownIconView;
	BStringView*	fDownVoteCountView;

	TextView*		fTextView;
};


class RatingContainerView : public BGroupView {
public:
	RatingContainerView()
		:
		BGroupView(B_VERTICAL, 0.0)
	{
		SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			kContentTint));
		AddChild(BSpaceLayoutItem::CreateGlue());
	}

	virtual BSize MinSize()
	{
		BSize minSize = BGroupView::MinSize();
		return BSize(minSize.width, 80);
	}

protected:
	virtual void DoLayout()
	{
		BGroupView::DoLayout();
		if (BScrollBar* scrollBar = ScrollBar(B_VERTICAL)) {
			BRect layoutArea = GroupLayout()->LayoutArea();
			float layoutHeight = layoutArea.Height();
			// Min size is not reliable with HasHeightForWidth() children,
			// since it does not reflect how thos children are currently
			// laid out, but what their theoretical minimum size would be.

			BLayoutItem* lastItem = GroupLayout()->ItemAt(
				GroupLayout()->CountItems() - 1);
			if (lastItem != NULL) {
				layoutHeight = lastItem->Frame().bottom;
			}

			float viewHeight = Bounds().Height();

			float max = layoutHeight- viewHeight;
			scrollBar->SetRange(0, max);
			if (layoutHeight > 0)
				scrollBar->SetProportion(viewHeight / layoutHeight);
			else
				scrollBar->SetProportion(1);
		}
	}
};


class RatingSummaryView : public BGridView {
public:
	RatingSummaryView()
		:
		BGridView("rating summary view", B_USE_HALF_ITEM_SPACING, 0.0f)
	{
		SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			kContentTint - 0.1));

		BLayoutBuilder::Grid<> layoutBuilder(this);

		BFont smallFont;
		GetFont(&smallFont);
		smallFont.SetSize(std::max(9.0f, floorf(smallFont.Size() * 0.85f)));

		for (int32 i = 0; i < 5; i++) {
			BString label;
			label.SetToFormat("%" B_PRId32, 5 - i);
			fLabelViews[i] = new BStringView("", label);
			fLabelViews[i]->SetFont(&smallFont);
			fLabelViews[i]->SetHighColor(kLightBlack);
			layoutBuilder.Add(fLabelViews[i], 0, i);

			fDiagramBarViews[i] = new DiagramBarView();
			layoutBuilder.Add(fDiagramBarViews[i], 1, i);

			fCountViews[i] = new BStringView("", "");
			fCountViews[i]->SetFont(&smallFont);
			fCountViews[i]->SetHighColor(kLightBlack);
			fCountViews[i]->SetAlignment(B_ALIGN_RIGHT);
			layoutBuilder.Add(fCountViews[i], 2, i);
		}

		layoutBuilder.SetInsets(5);
	}

	void SetToSummary(const RatingSummary& summary) {
		for (int32 i = 0; i < 5; i++) {
			int32 count = summary.ratingCountByStar[4 - i];

			BString label;
			label.SetToFormat("%" B_PRId32, count);
			fCountViews[i]->SetText(label);

			if (summary.ratingCount > 0) {
				fDiagramBarViews[i]->SetValue(
					(float)count / summary.ratingCount);
			} else
				fDiagramBarViews[i]->SetValue(0.0f);
		}
	}

	void Clear() {
		for (int32 i = 0; i < 5; i++) {
			fCountViews[i]->SetText("");
			fDiagramBarViews[i]->SetValue(0.0f);
		}
	}

private:
	BStringView*	fLabelViews[5];
	DiagramBarView*	fDiagramBarViews[5];
	BStringView*	fCountViews[5];
};


class UserRatingsView : public BGroupView {
public:
	UserRatingsView()
		:
		BGroupView("package ratings view", B_HORIZONTAL),
		fThumbsUpIcon(BitmapRef(new SharedBitmap(502), true)),
		fThumbsDownIcon(BitmapRef(new SharedBitmap(503), true))
	{
		SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			kContentTint));

		fRatingSummaryView = new RatingSummaryView();

		RatingContainerView* ratingsContainerView = new RatingContainerView();
		fRatingContainerLayout = ratingsContainerView->GroupLayout();

		BScrollView* scrollView = new RatingsScrollView(
			"ratings scroll view", ratingsContainerView);

		BLayoutBuilder::Group<>(this)
			.AddGroup(B_VERTICAL)
				.Add(fRatingSummaryView, 0.0f)
				.AddGlue()
				.SetInsets(0.0f, B_USE_DEFAULT_SPACING, 0.0f, 0.0f)
			.End()
			.Add(scrollView, 1.0f)
			.SetInsets(B_USE_DEFAULT_SPACING, -1.0f, -1.0f, -1.0f)
		;
	}

	virtual ~UserRatingsView()
	{
		Clear();
	}

	void SetPackage(const PackageInfo& package)
	{
		ClearRatings();

		// TODO: Re-use rating summary already used for TitleView...
		fRatingSummaryView->SetToSummary(package.CalculateRatingSummary());

		const UserRatingList& userRatings = package.UserRatings();

		// TODO: Sort by age or usefullness rating
		// TODO: Optionally hide ratings that are not in the system language

		for (int i = userRatings.CountItems() - 1; i >= 0; i--) {
			const UserRating& rating = userRatings.ItemAtFast(i);
			RatingItemView* view = new RatingItemView(rating, fThumbsUpIcon,
				fThumbsDownIcon);
			fRatingContainerLayout->AddView(0, view);
		}
	}

	void Clear()
	{
		fRatingSummaryView->Clear();
		ClearRatings();
	}

	void ClearRatings()
	{
		for (int32 i = fRatingContainerLayout->CountItems() - 1;
				BLayoutItem* item = fRatingContainerLayout->ItemAt(i); i--) {
			RatingItemView* view = dynamic_cast<RatingItemView*>(
				item->View());
			if (view != NULL) {
				view->RemoveSelf();
				delete view;
			}
		}
	}

private:
	BGroupLayout*			fRatingContainerLayout;
	RatingSummaryView*		fRatingSummaryView;
	BitmapRef				fThumbsUpIcon;
	BitmapRef				fThumbsDownIcon;
};


// #pragma mark - ChangelogView


class ChangelogView : public BView {
public:
	ChangelogView()
		:
		BView("package changelog view", B_WILL_DRAW),
		fLayout(new BGroupLayout(B_HORIZONTAL))
	{
		SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			kContentTint));

		SetLayout(fLayout);

		fTextView = new MarkupTextView("changelog view");
		fTextView->SetLowColor(ViewColor());
		fTextView->SetInsets(be_plain_font->Size());

		BScrollView* scrollView = new CustomScrollView(
			"changelog scroll view", fTextView);

		BLayoutBuilder::Group<>(fLayout)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(32.0f))
			.Add(scrollView, 1.0f)
			.SetInsets(B_USE_DEFAULT_SPACING, -1.0f, -1.0f, -1.0f)
		;
	}

	virtual ~ChangelogView()
	{
	}

	virtual void Draw(BRect updateRect)
	{
	}

	void SetPackage(const PackageInfo& package)
	{
		fTextView->SetText(package.Changelog());
	}

	void Clear()
	{
		fTextView->SetText("");
	}

private:
	BGroupLayout*		fLayout;
	MarkupTextView*		fTextView;
};


// #pragma mark - PagesView


class PagesView : public BTabView {
public:
	PagesView()
		:
		BTabView("pages view", B_WIDTH_FROM_WIDEST),
		fLayout(new BCardLayout())
	{
		SetBorder(B_NO_BORDER);

		fAboutView = new AboutView();
		fUserRatingsView = new UserRatingsView();
		fChangelogView = new ChangelogView();

		AddTab(fAboutView);
		AddTab(fUserRatingsView);
		AddTab(fChangelogView);

		TabAt(0)->SetLabel(B_TRANSLATE("About"));
		TabAt(1)->SetLabel(B_TRANSLATE("Ratings"));
		TabAt(2)->SetLabel(B_TRANSLATE("Changelog"));

		Select(0);
	}

	virtual ~PagesView()
	{
		Clear();
	}

	void SetPackage(const PackageInfo& package)
	{
		Select(0);
		fAboutView->SetPackage(package);
		fUserRatingsView->SetPackage(package);
		fChangelogView->SetPackage(package);
	}

	void Clear()
	{
		fAboutView->Clear();
		fUserRatingsView->Clear();
		fChangelogView->Clear();
	}

private:
	BCardLayout*		fLayout;

	AboutView*			fAboutView;
	UserRatingsView*	fUserRatingsView;
	ChangelogView*		fChangelogView;
};


// #pragma mark - PackageInfoViewListener


enum {
	MSG_UPDATE_PACKAGE		= 'updp'
};


class PackageInfoView::Listener : public PackageInfoListener {
public:
	Listener(PackageInfoView* view)
		:
		fView(view)
	{
	}

	virtual ~Listener()
	{
	}

	virtual void PackageChanged(const PackageInfoEvent& event)
	{
		BMessenger messenger(fView);
		if (!messenger.IsValid())
			return;

		const PackageInfo& package = *event.Package().Get();

		BMessage message(MSG_UPDATE_PACKAGE);
		message.AddString("title", package.Title());
		message.AddUInt32("changes", event.Changes());

		messenger.SendMessage(&message);
	}

	void SetPackage(const PackageInfoRef& package)
	{
		if (fPackage == package)
			return;

		PackageInfoListenerRef listener(this);

		if (fPackage.Get() != NULL)
			fPackage->RemoveListener(listener);

		fPackage = package;

		if (fPackage.Get() != NULL)
			fPackage->AddListener(listener);
	}

	const PackageInfoRef& Package() const
	{
		return fPackage;
	}

private:
	PackageInfoView*	fView;
	PackageInfoRef		fPackage;
};


// #pragma mark - PackageInfoView


PackageInfoView::PackageInfoView(BLocker* modelLock,
		PackageActionHandler* handler)
	:
	BGroupView("package info view", B_VERTICAL),
	fModelLock(modelLock),
	fPackageListener(new(std::nothrow) Listener(this))
{
	fTitleView = new TitleView();
	fPackageActionView = new PackageActionView(handler);
	fPagesView = new PagesView();

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(fTitleView)
			.Add(fPackageActionView)
			.SetInsets(
				B_USE_DEFAULT_SPACING, 0.0f,
				B_USE_DEFAULT_SPACING, 0.0f)
		.End()
		.Add(fPagesView)
	;

	Clear();
}


PackageInfoView::~PackageInfoView()
{
	fPackageListener->SetPackage(PackageInfoRef(NULL));
	delete fPackageListener;
}


void
PackageInfoView::AttachedToWindow()
{
}


void
PackageInfoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_UPDATE_PACKAGE:
		{
			if (fPackageListener->Package().Get() == NULL)
				break;

			BString title;
			uint32 changes;
			if (message->FindString("title", &title) != B_OK
				|| message->FindUInt32("changes", &changes) != B_OK) {
				break;
			}

			const PackageInfo& package = *fPackageListener->Package().Get();
			if (package.Title() != title)
				break;

			BAutolock _(fModelLock);

			if ((changes & PKG_CHANGED_DESCRIPTION) != 0
				|| (changes & PKG_CHANGED_SCREENSHOTS) != 0) {
				fTitleView->SetPackage(package);
			}

			if ((changes & PKG_CHANGED_RATINGS) != 0)
				fPagesView->SetPackage(package);

			if ((changes & PKG_CHANGED_STATE) != 0) {
				fPackageActionView->SetPackage(package);
			}

			break;
		}
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
PackageInfoView::SetPackage(const PackageInfoRef& packageRef)
{
	BAutolock _(fModelLock);

	const PackageInfo& package = *packageRef.Get();

	fTitleView->SetPackage(package);
	fPackageActionView->SetPackage(package);
	fPagesView->SetPackage(package);

	if (fPagesView->IsHidden(fPagesView))
		fPagesView->Show();

	fPackageListener->SetPackage(packageRef);
}


void
PackageInfoView::Clear()
{
	fTitleView->Clear();
	fPackageActionView->Clear();
	fPagesView->Clear();

	if (!fPagesView->IsHidden(fPagesView))
		fPagesView->Hide();

	BAutolock _(fModelLock);

	fPackageListener->SetPackage(PackageInfoRef(NULL));
}

