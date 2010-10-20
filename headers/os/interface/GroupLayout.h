/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_GROUP_LAYOUT_H
#define	_GROUP_LAYOUT_H

#include <TwoDimensionalLayout.h>

class BGroupLayout : public BTwoDimensionalLayout {
public:
								BGroupLayout(enum orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING);
								BGroupLayout(BMessage* from);
	virtual						~BGroupLayout();

			float				Spacing() const;
			void				SetSpacing(float spacing);

			orientation			Orientation() const;
			void				SetOrientation(enum orientation orientation);
	
			float				ItemWeight(int32 index) const;
			void				SetItemWeight(int32 index, float weight);

	virtual	BLayoutItem*		AddView(BView* child);
	virtual	BLayoutItem*		AddView(int32 index, BView* child);
	virtual	BLayoutItem*		AddView(BView* child, float weight);
	virtual	BLayoutItem*		AddView(int32 index, BView* child,
									float weight);

	virtual	bool				AddItem(BLayoutItem* item);
	virtual	bool				AddItem(int32 index, BLayoutItem* item);
	virtual	bool				AddItem(BLayoutItem* item, float weight);
	virtual	bool				AddItem(int32 index, BLayoutItem* item,
									float weight);

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	virtual	status_t			AllUnarchived(const BMessage* from);
	static	BArchivable*		Instantiate(BMessage* from);

	virtual status_t			ItemArchived(BMessage* into, BLayoutItem* item,
									int32 index) const;
	virtual	status_t			ItemUnarchived(const BMessage* from,
									BLayoutItem* item, int32 index);

protected:	
	virtual	bool				ItemAdded(BLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(BLayoutItem* item, int32 fromIndex);

	virtual	void				PrepareItems(enum orientation orientation);
	
	virtual	int32				InternalCountColumns();
	virtual	int32				InternalCountRows();
	virtual	void				GetColumnRowConstraints(
									enum orientation orientation,
									int32 index,
									ColumnRowConstraints* constraints);
	virtual	void				GetItemDimensions(BLayoutItem* item,
									Dimensions* dimensions);

private:
			struct ItemLayoutData;

			ItemLayoutData*		_LayoutDataForItem(BLayoutItem* item) const;

			orientation			fOrientation;
			BList				fVisibleItems;
};

#endif	// _GROUP_LAYOUT_H
