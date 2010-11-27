/*
 * Copyright 2010, Haiku Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <Layout.h>

#include <algorithm>
#include <new>
#include <syslog.h>

#include <LayoutContext.h>
#include <Message.h>
#include <View.h>
#include <ViewPrivate.h>

#include "ViewLayoutItem.h"


using std::nothrow;
using std::swap;


namespace {
	// flags for our state
	const uint32 B_LAYOUT_INVALID = 0x80000000UL; // needs layout
	const uint32 B_LAYOUT_CACHE_INVALID = 0x40000000UL; // needs recalculation
	const uint32 B_LAYOUT_REQUIRED = 0x20000000UL; // needs layout
	const uint32 B_LAYOUT_IN_PROGRESS = 0x10000000UL;
	const uint32 B_LAYOUT_ALL_CLEAR = 0UL;

	// handy masks to check various states
	const uint32 B_LAYOUT_INVALIDATION_ILLEGAL
		= B_LAYOUT_CACHE_INVALID | B_LAYOUT_IN_PROGRESS;
	const uint32 B_LAYOUT_NECESSARY
		= B_LAYOUT_INVALID | B_LAYOUT_REQUIRED | B_LAYOUT_CACHE_INVALID;
	const uint32 B_RELAYOUT_NOT_OK
		= B_LAYOUT_INVALID | B_LAYOUT_IN_PROGRESS;

	const char* const kLayoutItemField = "BLayout:items";
}


BLayout::BLayout()
	:
	fState(B_LAYOUT_ALL_CLEAR),
	fAncestorsVisible(true),
	fInvalidationDisabled(0),
	fContext(NULL),
	fOwner(NULL),
	fTarget(NULL),
	fItems(20)
{
}


BLayout::BLayout(BMessage* from)
	:
	BLayoutItem(BUnarchiver::PrepareArchive(from)),
	fState(B_LAYOUT_ALL_CLEAR),
	fAncestorsVisible(true),
	fInvalidationDisabled(0),
	fContext(NULL),
	fOwner(NULL),
	fTarget(NULL),
	fItems(20)
{
	BUnarchiver unarchiver(from);

	int32 i = 0;
	while (unarchiver.EnsureUnarchived(kLayoutItemField, i++) == B_OK)
		;
}


BLayout::~BLayout()
{
	// in case we have a view, but have been added to a layout as a BLayoutItem
	// we will get deleted before our view, so we should tell it that we're
	// going, so that we aren't double-freed.
	if (fOwner && this == fOwner->GetLayout())
		fOwner->_LayoutLeft(this);

	// removes and deletes all items
	if (fTarget)
		SetTarget(NULL);
}


BView*
BLayout::Owner() const
{
	return fOwner;
}


BView*
BLayout::TargetView() const
{
	return fTarget;
}


BView*
BLayout::View()
{
	return fOwner;
}


BLayoutItem*
BLayout::AddView(BView* child)
{
	return AddView(-1, child);
}


BLayoutItem*
BLayout::AddView(int32 index, BView* child)
{
	BLayoutItem* item = child->GetLayout();
	if (!item)
		item = new(nothrow) BViewLayoutItem(child);

	if (item && AddItem(index, item))
		return item;

	if (!child->GetLayout())
		delete item;
	return NULL;
}


bool
BLayout::AddItem(BLayoutItem* item)
{
	return AddItem(-1, item);
}


bool
BLayout::AddItem(int32 index, BLayoutItem* item)
{
	if (!fTarget || !item || fItems.HasItem(item))
		return false;

	// if the item refers to a BView, we make sure it is added to the parent
	// view
	bool addedView = false;
	BView* view = item->View();
	if (view && view->fParent != fTarget
		&& !(addedView = fTarget->_AddChild(view, NULL)))
		return false;

	// validate the index
	if (index < 0 || index > fItems.CountItems())
		index = fItems.CountItems();

	if (fItems.AddItem(item, index) && ItemAdded(item, index)) {
		item->SetLayout(this);
		if (!fAncestorsVisible)
			item->AncestorVisibilityChanged(fAncestorsVisible);
		InvalidateLayout();
		return true;
	} else {
		// this check is necessary so that if an addition somewhere other
		// than the end of the list fails, we don't remove the wrong item
		if (fItems.ItemAt(index) == item)
			fItems.RemoveItem(index);
		if (addedView)
			view->_RemoveSelf();
		return false;
	}
}


bool
BLayout::RemoveView(BView* child)
{
	bool removed = false;

	// a view can have any number of layout items - we need to remove them all
	for (int32 i = fItems.CountItems(); i-- > 0;) {
		BLayoutItem* item = ItemAt(i);

		if (item->View() != child)
			continue;

		RemoveItem(i);
		removed = true;
		delete item;
	}

	return removed;
}


bool
BLayout::RemoveItem(BLayoutItem* item)
{
	int32 index = IndexOfItem(item);
	return (index >= 0 ? RemoveItem(index) : false);
}


BLayoutItem*
BLayout::RemoveItem(int32 index)
{
	if (index < 0 || index >= fItems.CountItems())
		return NULL;

	BLayoutItem* item = (BLayoutItem*)fItems.RemoveItem(index);

	// if the item refers to a BView, we make sure, it is removed from the
	// parent view
	BView* view = item->View();
	if (view && view->fParent == fTarget)
		view->_RemoveSelf();

	ItemRemoved(item, index);
	item->SetLayout(NULL);
	InvalidateLayout();

	return item;
}


BLayoutItem*
BLayout::ItemAt(int32 index) const
{
	return (BLayoutItem*)fItems.ItemAt(index);
}


int32
BLayout::CountItems() const
{
	return fItems.CountItems();
}


int32
BLayout::IndexOfItem(const BLayoutItem* item) const
{
	return fItems.IndexOf(item);
}


int32
BLayout::IndexOfView(BView* child) const
{
	if (child == NULL)
		return -1;

	int itemCount = fItems.CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		BLayoutItem* item = (BLayoutItem*)fItems.ItemAt(i);
		if (item->View() == child && (dynamic_cast<BViewLayoutItem*>(item)
				|| dynamic_cast<BLayout*>(item)))
			return i;
	}

	return -1;
}


bool
BLayout::AncestorsVisible() const
{
	return fAncestorsVisible;
}


void
BLayout::InvalidateLayout(bool children)
{
	// printf("BLayout(%p)::InvalidateLayout(%i) : state %x, disabled %li\n",
	// this, children, (unsigned int)fState, fInvalidationDisabled);

	if (!InvalidationLegal())
		return;

	fState |= B_LAYOUT_NECESSARY;

	if (children) {
		for (int32 i = CountItems() - 1; i >= 0; i--)
			ItemAt(i)->InvalidateLayout(children);
	}

	if (fOwner && BView::Private(fOwner).MinMaxValid())
		fOwner->InvalidateLayout(children);

	if (BLayout* nestedIn = Layout()) {
		if (nestedIn->InvalidationLegal())
			nestedIn->InvalidateLayout();
	} else if (fOwner) {
		// If we weren't added as a BLayoutItem, we still have to invalidate
		// whatever layout our owner is in.
		BView* ownerParent = fOwner->fParent;
		if (ownerParent) {
			BLayout* layout = ownerParent->GetLayout();
			if (layout && layout->fNestedLayouts.CountItems() > 0)
				layout->InvalidateLayoutsForView(fOwner);
			else if (BView::Private(ownerParent).MinMaxValid())
				ownerParent->InvalidateLayout(false);
		}
	}
}


void
BLayout::RequireLayout()
{
	fState |= B_LAYOUT_REQUIRED;
}


bool
BLayout::IsValid()
{
	return (fState & B_LAYOUT_INVALID) == 0;
}


void
BLayout::DisableLayoutInvalidation()
{
	fInvalidationDisabled++;
}


void
BLayout::EnableLayoutInvalidation()
{
	if (fInvalidationDisabled > 0)
		fInvalidationDisabled--;
}


void
BLayout::LayoutItems(bool force)
{
	if ((fState & B_LAYOUT_NECESSARY) == 0 && !force)
		return;

	if (Layout() && (Layout()->fState & B_LAYOUT_IN_PROGRESS) != 0)
		return; // wait for parent layout to lay us out.

	if (fTarget && fTarget->LayoutContext())
		return;

	BLayoutContext context;
	_LayoutWithinContext(force, &context);
}


void
BLayout::Relayout(bool immediate)
{
	if ((fState & B_RELAYOUT_NOT_OK) == 0 || immediate) {
		fState |= B_LAYOUT_REQUIRED;
		LayoutItems(false);
	}
}



void
BLayout::_LayoutWithinContext(bool force, BLayoutContext* context)
{
// printf("BLayout(%p)::_LayoutWithinContext(%i, %p), state %x, fContext %p\n",
// this, force, context, (unsigned int)fState, fContext);

	if ((fState & B_LAYOUT_NECESSARY) == 0 && !force)
		return;

	BLayoutContext* oldContext = fContext;
	fContext = context;

	if (fOwner && BView::Private(fOwner).WillLayout()) {
		// in this case, let our owner decide whether or not to have us
		// do our layout, if they do, we won't end up here again.
		fOwner->_Layout(force, context);
	} else {
		fState |= B_LAYOUT_IN_PROGRESS;
		DerivedLayoutItems();
		// we must ensure that all items are laid out, layouts with a view will
		// have their layout process triggered by their view, but nested
		// view-less layouts must have their layout triggered here (if it hasn't
		// already been triggered).
		int32 nestedLayoutCount = fNestedLayouts.CountItems();
		for (int32 i = 0; i < nestedLayoutCount; i++) {
			BLayout* layout = (BLayout*)fNestedLayouts.ItemAt(i);
			if ((layout->fState & B_LAYOUT_NECESSARY) != 0)
				layout->_LayoutWithinContext(force, context);
		}
		fState = B_LAYOUT_ALL_CLEAR;
	}

	fContext = oldContext;
}


BRect
BLayout::LayoutArea()
{
	BRect area(Frame());
	if (fOwner)
		area.OffsetTo(B_ORIGIN);
	return area;
}


status_t
BLayout::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BLayoutItem::Archive(into, deep);

	if (deep) {
		int32 count = CountItems();
		for (int32 i = 0; i < count && err == B_OK; i++) {
			BLayoutItem* item = ItemAt(i);
			err = archiver.AddArchivable(kLayoutItemField, item, deep);

			if (err == B_OK) {
				err = ItemArchived(into, item, i);
				if (err != B_OK)
					syslog(LOG_ERR, "ItemArchived() failed at index: %d.", i);
			}
		}
	}

	return archiver.Finish(err);
}


status_t
BLayout::AllUnarchived(const BMessage* from)
{
	BUnarchiver unarchiver(from);
	status_t err = BLayoutItem::AllUnarchived(from);
	if (err != B_OK)
		return err;

	int32 itemCount;
	unarchiver.ArchiveMessage()->GetInfo(kLayoutItemField, NULL, &itemCount);
	for (int32 i = 0; i < itemCount && err == B_OK; i++) {
		BLayoutItem* item;
		err = unarchiver.FindObject(kLayoutItemField,
			i, BUnarchiver::B_DONT_ASSUME_OWNERSHIP, item);
		if (err != B_OK)
			return err;

		if (!fItems.AddItem(item, i) || !ItemAdded(item, i)) {
			fItems.RemoveItem(i);
			return B_ERROR;
		}

		err = ItemUnarchived(from, item, i);
		if (err != B_OK) {
			fItems.RemoveItem(i);
			ItemRemoved(item, i);
			return err;
		}

		item->SetLayout(this);
		unarchiver.AssumeOwnership(item);
	}

	InvalidateLayout();
	return err;
}


status_t
BLayout::ItemArchived(BMessage* into, BLayoutItem* item, int32 index) const
{
	return B_OK;
}


status_t
BLayout::ItemUnarchived(const BMessage* from, BLayoutItem* item, int32 index)
{
	return B_OK;
}


bool
BLayout::ItemAdded(BLayoutItem* item, int32 atIndex)
{
	return true;
}


void
BLayout::ItemRemoved(BLayoutItem* item, int32 fromIndex)
{
}


void
BLayout::OwnerChanged(BView* was)
{
}


void
BLayout::AttachedToLayout()
{
	if (!fOwner) {
		Layout()->fNestedLayouts.AddItem(this);
		SetTarget(Layout()->TargetView());
	}
}


void
BLayout::DetachedFromLayout(BLayout* from)
{
	if (!fOwner) {
		from->fNestedLayouts.RemoveItem(this);
		SetTarget(NULL);
	}
}


void
BLayout::AncestorVisibilityChanged(bool shown)
{
	if (fAncestorsVisible == shown)
		return;

	fAncestorsVisible = shown;
	VisibilityChanged(shown);
}


void
BLayout::VisibilityChanged(bool show)
{
	if (fOwner)
		return;

	for (int32 i = CountItems() - 1; i >= 0; i--)
		ItemAt(i)->AncestorVisibilityChanged(show);
}


void
BLayout::ResetLayoutInvalidation()
{
	fState &= ~B_LAYOUT_CACHE_INVALID;
}


BLayoutContext*
BLayout::LayoutContext() const
{
	return fContext;
}


bool
BLayout::RemoveViewRecursive(BView* view)
{
	bool removed = RemoveView(view);
	for (int32 i = fNestedLayouts.CountItems() - 1; i >= 0; i--) {
		BLayout* nested = (BLayout*)fNestedLayouts.ItemAt(i);
		removed |= nested->RemoveViewRecursive(view);
	}
	return removed;
}


bool
BLayout::InvalidateLayoutsForView(BView* view)
{
	bool found = false;
	for (int32 i = fNestedLayouts.CountItems() - 1; i >= 0; i--) {
		BLayout* layout = (BLayout*)fNestedLayouts.ItemAt(i);
		found |= layout->InvalidateLayoutsForView(view);
	}

	if (found)
		return found;

	if (!InvalidationLegal())
		return false;

	for (int32 i = CountItems() - 1; i >= 0; i--) {
		if (ItemAt(i)->View() == view) {
			InvalidateLayout();
			return true;
		}
	}
	return found;
}


bool
BLayout::InvalidationLegal()
{
	return fInvalidationDisabled <= 0
		&& (fState & B_LAYOUT_INVALIDATION_ILLEGAL) == 0;
}


void
BLayout::SetOwner(BView* owner)
{
	if (fOwner == owner)
		return;

	SetTarget(owner);
	swap(fOwner, owner);

	OwnerChanged(owner);
		// call hook
}


void
BLayout::SetTarget(BView* target)
{
	if (fTarget != target) {
		fTarget = NULL;
			// only remove items, not views

		// remove and delete all items
		for (int32 i = CountItems() - 1; i >= 0; i--)
			delete RemoveItem(i);

		fTarget = target;

		InvalidateLayout();
	}
}
