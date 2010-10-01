/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "ALMLayout.h"

#include <math.h>		// for floor
#include <new>

#include "ViewLayoutItem.h"

#include "ResultType.h"


const BSize kUnsetSize(B_SIZE_UNSET, B_SIZE_UNSET);
const BSize kMinSize(0, 0);
const BSize kMaxSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);


/**
 * Constructor.
 * Creates new layout engine.
 */
BALMLayout::BALMLayout(float spacing)
	:
	fInset(0.0f),
	fSpacing(spacing),
	fCurrentArea(NULL)
{
	fLeft = new XTab(&fSolver);
	fRight = new XTab(&fSolver);
	fTop = new YTab(&fSolver);
	fBottom = new YTab(&fSolver);

	// the Left tab is always at x-position 0, and the Top tab is always at y-position 0
	fLeft->SetRange(0, 0);
	fTop->SetRange(0, 0);

	// cached layout values
	// need to be invalidated whenever the layout specification is changed
	fMinSize = kUnsetSize;
	fMaxSize = kUnsetSize;
	fPreferredSize = kUnsetSize;

	fPerformancePath = NULL;
}


BALMLayout::~BALMLayout()
{
	
}


/**
 * Adds a new x-tab to the specification.
 *
 * @return the new x-tab
 */
XTab*
BALMLayout::AddXTab()
{
	return new XTab(&fSolver);
}


/**
 * Adds a new y-tab to the specification.
 *
 * @return the new y-tab
 */
YTab*
BALMLayout::AddYTab()
{
	return new YTab(&fSolver);
}


/**
 * Adds a new row to the specification.
 *
 * @return the new row
 */
Row*
BALMLayout::AddRow()
{
	return new Row(&fSolver);
}


/**
 * Adds a new row to the specification that is glued to the given y-tabs.
 *
 * @param top
 * @param bottom
 * @return the new row
 */
Row*
BALMLayout::AddRow(YTab* top, YTab* bottom)
{
	Row* row = new Row(&fSolver);
	if (top != NULL)
		row->Constraints()->AddItem(row->Top()->IsEqual(top));
	if (bottom != NULL)
		row->Constraints()->AddItem(row->Bottom()->IsEqual(bottom));
	return row;
}


/**
 * Adds a new column to the specification.
 *
 * @return the new column
 */
Column*
BALMLayout::AddColumn()
{
	return new Column(&fSolver);
}


/**
 * Adds a new column to the specification that is glued to the given x-tabs.
 *
 * @param left
 * @param right
 * @return the new column
 */
Column*
BALMLayout::AddColumn(XTab* left, XTab* right)
{
	Column* column = new Column(&fSolver);
	if (left != NULL)
		column->Constraints()->AddItem(column->Left()->IsEqual(left));
	if (right != NULL)
		column->Constraints()->AddItem(column->Right()->IsEqual(right));
	return column;
}


/**
 * Finds the area that contains the given control.
 *
 * @param control	the control to look for
 * @return the area that contains the control
 */
Area*
BALMLayout::AreaFor(const BView* control) const
{
	return AreaFor(ItemAt(IndexOfView(const_cast<BView*>(control))));
}


Area*
BALMLayout::AreaFor(const BLayoutItem* item) const
{
	if (!item)
		return NULL;
	return static_cast<Area*>(item->LayoutData());
}


Area*
BALMLayout::CurrentArea() const
{
	return fCurrentArea;
}


void
BALMLayout::SetCurrentArea(const Area* area)
{
	fCurrentArea = const_cast<Area*>(area);
}


void
BALMLayout::SetCurrentArea(const BView* view)
{
	fCurrentArea = AreaFor(view);
}


void
BALMLayout::SetCurrentArea(const BLayoutItem* item)
{
	fCurrentArea = AreaFor(item);
}


XTab*
BALMLayout::LeftOf(const BView* view) const
{
	return AreaFor(view)->Left();
}


XTab*
BALMLayout::LeftOf(const BLayoutItem* item) const
{
	return AreaFor(item)->Left();
}


XTab*
BALMLayout::RightOf(const BView* view) const
{
	return AreaFor(view)->Right();
}


XTab*
BALMLayout::RightOf(const BLayoutItem* item) const
{
	return AreaFor(item)->Right();
}


YTab*
BALMLayout::TopOf(const BView* view) const
{
	return AreaFor(view)->Top();
}


YTab*
BALMLayout::TopOf(const BLayoutItem* item) const
{
	return AreaFor(item)->Top();
}


YTab*
BALMLayout::BottomOf(const BView* view) const
{
	return AreaFor(view)->Bottom();
}


YTab*
BALMLayout::BottomOf(const BLayoutItem* item) const
{
	return AreaFor(item)->Bottom();
}


BLayoutItem*
BALMLayout::AddView(BView* child)
{
	return AddView(-1, child);
}


BLayoutItem*
BALMLayout::AddView(int32 index, BView* child)
{
	return BAbstractLayout::AddView(index, child);
}


/**
 * Adds a new area to the specification, automatically setting preferred size constraints.
 *
 * @param left			left border
 * @param top			top border
 * @param right		right border
 * @param bottom		bottom border
 * @param content		the control which is the area content
 * @return the new area
 */
Area*
BALMLayout::AddView(BView* view, XTab* left, YTab* top, XTab* right,
	YTab* bottom)
{
	BLayoutItem* item = _CreateLayoutItem(view);
	Area* area = AddItem(item, left, top, right, bottom);
	if (!area) {
		delete item;
		return NULL;
	}
	return area;
}


/**
 * Adds a new area to the specification, automatically setting preferred size constraints.
 *
 * @param row			the row that defines the top and bottom border
 * @param column		the column that defines the left and right border
 * @param content		the control which is the area content
 * @return the new area
 */
Area*
BALMLayout::AddView(BView* view, Row* row, Column* column)
{
	BLayoutItem* item = _CreateLayoutItem(view);
	Area* area = AddItem(item, row, column);
	if (!area) {
		delete item;
		return NULL;
	}
	return area;
}


Area*
BALMLayout::AddViewToRight(BView* view, XTab* right, YTab* top, YTab* bottom)
{
	BLayoutItem* item = _CreateLayoutItem(view);
	Area* area = AddItemToRight(item, right, top, bottom);
	if (!area) {
		delete item;
		return NULL;
	}
	return area;
}


Area*
BALMLayout::AddViewToLeft(BView* view, XTab* left, YTab* top, YTab* bottom)
{
	BLayoutItem* item = _CreateLayoutItem(view);
	Area* area = AddItemToLeft(item, left, top, bottom);
	if (!area) {
		delete item;
		return NULL;
	}
	return area;
}


Area*
BALMLayout::AddViewToTop(BView* view, YTab* top, XTab* left, XTab* right)
{
	BLayoutItem* item = _CreateLayoutItem(view);
	Area* area = AddItemToTop(item, top, left, right);
	if (!area) {
		delete item;
		return NULL;
	}
	return area;
}


Area*
BALMLayout::AddViewToBottom(BView* view, YTab* bottom, XTab* left, XTab* right)
{
	BLayoutItem* item = _CreateLayoutItem(view);
	Area* area = AddItemToBottom(item, bottom, left, right);
	if (!area) {
		delete item;
		return NULL;
	}
	return area;
}


bool
BALMLayout::AddItem(BLayoutItem* item)
{
	return AddItem(-1, item);
}


bool
BALMLayout::AddItem(int32 index, BLayoutItem* item)
{
	if (!item)
		return NULL;

	// simply add the item at the upper right corner of the previous item
	// TODO maybe find a more elegant solution
	XTab* left = Left();
	YTab* top = Top();

	// check range
	if (index < 0 || index > CountItems())
		index = CountItems();

	// for index = 0 we already have set the right tabs
	if (index != 0) {
		BLayoutItem* prevItem = ItemAt(index - 1);
		Area* area = AreaFor(prevItem);
		if (area) {
			left = area->Right();
			top = area->Top();
		}
	}
	Area* area = AddItem(item, left, top);
	return area ? true : false;
}


Area*
BALMLayout::AddItem(BLayoutItem* item, XTab* left, YTab* top, XTab* right,
	YTab* bottom)
{
	if (!right)
		right = AddXTab();
	if (!bottom)
		bottom = AddYTab();

	if (!BAbstractLayout::AddItem(-1, item))
		return NULL;
	Area* area = AreaFor(item);
	if (!area)
		return NULL;
	fCurrentArea = area;

	area->_Init(&fSolver, left, top, right, bottom);
	return area;
}


Area*
BALMLayout::AddItem(BLayoutItem* item, Row* row, Column* column)
{
	if (!BAbstractLayout::AddItem(-1, item))
		return NULL;
	Area* area = AreaFor(item);
	if (!area)
		return NULL;
	fCurrentArea = area;

	area->_Init(&fSolver, row, column);
	return area;
}


Area*
BALMLayout::AddItemToRight(BLayoutItem* item, XTab* right, YTab* top,
	YTab* bottom)
{
	XTab* left = fCurrentArea->Right();
	if (!right)
		right = AddXTab();
	if (!top)
		top = fCurrentArea->Top();
	if (!bottom)
		bottom = fCurrentArea->Bottom();

	return AddItem(item, left, top, right, bottom);
}


Area*
BALMLayout::AddItemToLeft(BLayoutItem* item, XTab* left, YTab* top,
	YTab* bottom)
{
	if (!left)
		left = AddXTab();
	XTab* right = fCurrentArea->Left();
	if (!top)
		top = fCurrentArea->Top();
	if (!bottom)
		bottom = fCurrentArea->Bottom();

	return AddItem(item, left, top, right, bottom);
}


Area*
BALMLayout::AddItemToTop(BLayoutItem* item, YTab* top, XTab* left, XTab* right)
{
	if (!left)
		left = fCurrentArea->Left();
	if (!right)
		right = fCurrentArea->Right();
	if (!top)
		top = AddYTab();
	YTab* bottom = fCurrentArea->Top();

	return AddItem(item, left, top, right, bottom);
}


Area*
BALMLayout::AddItemToBottom(BLayoutItem* item, YTab* bottom, XTab* left,
	XTab* right)
{
	if (!left)
		left = fCurrentArea->Left();
	if (!right)
		right = fCurrentArea->Right();
	YTab* top = fCurrentArea->Bottom();
	if (!bottom)
		bottom = AddYTab();

	return AddItem(item, left, top, right, bottom);
}


/**
 * Gets the left variable.
 */
XTab*
BALMLayout::Left() const
{
	return fLeft;
}


/**
 * Gets the right variable.
 */
XTab*
BALMLayout::Right() const
{
	return fRight;
}


/**
 * Gets the top variable.
 */
YTab*
BALMLayout::Top() const
{
	return fTop;
}


/**
 * Gets the bottom variable.
 */
YTab*
BALMLayout::Bottom() const
{
	return fBottom;
}


/**
 * Gets minimum size.
 */
BSize
BALMLayout::BaseMinSize() {
	if (fMinSize == kUnsetSize)
		fMinSize = _CalculateMinSize();
	return fMinSize;
}


/**
 * Gets maximum size.
 */
BSize
BALMLayout::BaseMaxSize()
{
	if (fMaxSize == kUnsetSize)
		fMaxSize = _CalculateMaxSize();
	return fMaxSize;
}


/**
 * Gets preferred size.
 */
BSize
BALMLayout::BasePreferredSize()
{
	if (fPreferredSize == kUnsetSize)
		fPreferredSize = _CalculatePreferredSize();
	return fPreferredSize;
}


/**
 * Gets the alignment.
 */
BAlignment
BALMLayout::BaseAlignment()
{
	BAlignment alignment;
	alignment.SetHorizontal(B_ALIGN_HORIZONTAL_CENTER);
	alignment.SetVertical(B_ALIGN_VERTICAL_CENTER);
	return alignment;
}


/**
 * Invalidates the layout.
 * Resets minimum/maximum/preferred size.
 */
void
BALMLayout::InvalidateLayout(bool children)
{
	BLayout::InvalidateLayout(children);
	fMinSize = kUnsetSize;
	fMaxSize = kUnsetSize;
	fPreferredSize = kUnsetSize;
}


bool
BALMLayout::ItemAdded(BLayoutItem* item, int32 atIndex)
{
	item->SetLayoutData(new(std::nothrow) Area(item));
	return item->LayoutData() != NULL;
}


void
BALMLayout::ItemRemoved(BLayoutItem* item, int32 fromIndex)
{
	if (Area* area = AreaFor(item)) {
		item->SetLayoutData(NULL);
		delete area;
	}
}


/**
 * Calculate and set the layout.
 * If no layout specification is given, a specification is reverse engineered automatically.
 */
void
BALMLayout::DerivedLayoutItems()
{
	_UpdateAreaConstraints();

	// Enforced absolute positions of Right and Bottom
	BRect area(LayoutArea());
	Right()->SetRange(area.right, area.right);
	Bottom()->SetRange(area.bottom, area.bottom);

	_SolveLayout();

	// if new layout is infasible, use previous layout
	if (fSolver.Result() == INFEASIBLE)
		return;

	if (fSolver.Result() != OPTIMAL) {
		fSolver.Save("failed-layout.txt");
		printf("Could not solve the layout specification (%d). ",
			fSolver.Result());
		printf("Saved specification in file failed-layout.txt\n");
	}

	// set the calculated positions and sizes for every area
	for (int32 i = 0; i < CountItems(); i++)
		AreaFor(ItemAt(i))->_DoLayout();
}


/**
 * Gets the path of the performance log file.
 *
 * @return the path of the performance log file
 */
char*
BALMLayout::PerformancePath() const
{
	return fPerformancePath;
}


/**
 * Sets the path of the performance log file.
 *
 * @param path	the path of the performance log file
 */
void
BALMLayout::SetPerformancePath(char* path)
{
	fPerformancePath = path;
}


LinearSpec*
BALMLayout::Solver() const
{
	return const_cast<LinearSpec*>(&fSolver);
}


void
BALMLayout::SetInset(float inset)
{
	fInset = inset;
}


float
BALMLayout::Inset() const
{
	return fInset;
}


void
BALMLayout::SetSpacing(float spacing)
{
	fSpacing = spacing;
}


float
BALMLayout::Spacing() const
{
	return fSpacing;
}


BLayoutItem*
BALMLayout::_CreateLayoutItem(BView* view)
{
	return new(std::nothrow) BViewLayoutItem(view);
}


void
BALMLayout::_SolveLayout()
{
	// Try to solve the layout until the result is OPTIMAL or INFEASIBLE,
	// maximally 15 tries sometimes the solving algorithm encounters numerical
	// problems (NUMFAILURE), and repeating the solving often helps to overcome
	// them.
	BFile* file = NULL;
	if (fPerformancePath != NULL) {
		file = new BFile(fPerformancePath,
			B_READ_WRITE | B_CREATE_FILE | B_OPEN_AT_END);
	}

	ResultType result;
	for (int32 tries = 0; tries < 15; tries++) {
		result = fSolver.Solve();
		if (fPerformancePath != NULL) {
			/*char buffer [100];
			file->Write(buffer, sprintf(buffer, "%d\t%fms\t#vars=%ld\t"
				"#constraints=%ld\n", result, fSolver.SolvingTime(),
				fSolver.Variables()->CountItems(),
				fSolver.Constraints()->CountItems()));*/
		}
		if (result == OPTIMAL || result == INFEASIBLE)
			break;
	}
	delete file;
}


/**
 * Caculates the miminum size.
 */
BSize
BALMLayout::_CalculateMinSize()
{
	_UpdateAreaConstraints();

	SummandList* oldObjFunction = fSolver.ObjFunction();
	SummandList* newObjFunction = new SummandList(2);
	newObjFunction->AddItem(new Summand(1.0, fRight));
	newObjFunction->AddItem(new Summand(1.0, fBottom));
	fSolver.SetObjFunction(newObjFunction);
	_SolveLayout();
	fSolver.SetObjFunction(oldObjFunction);
	fSolver.UpdateObjFunction();
	delete newObjFunction->ItemAt(0);
	delete newObjFunction->ItemAt(1);
	delete newObjFunction;

	if (fSolver.Result() == UNBOUNDED)
		return kMinSize;
	if (fSolver.Result() != OPTIMAL) {
		fSolver.Save("failed-layout.txt");
		printf("Could not solve the layout specification (%d). "
			"Saved specification in file failed-layout.txt", fSolver.Result());
	}

	return BSize(Right()->Value() - Left()->Value(),
		Bottom()->Value() - Top()->Value());
}


/**
 * Caculates the maximum size.
 */
BSize
BALMLayout::_CalculateMaxSize()
{
	_UpdateAreaConstraints();

	SummandList* oldObjFunction = fSolver.ObjFunction();
	SummandList* newObjFunction = new SummandList(2);
	newObjFunction->AddItem(new Summand(-1.0, fRight));
	newObjFunction->AddItem(new Summand(-1.0, fBottom));
	fSolver.SetObjFunction(newObjFunction);
	_SolveLayout();
	fSolver.SetObjFunction(oldObjFunction);
	fSolver.UpdateObjFunction();
	delete newObjFunction->ItemAt(0);
	delete newObjFunction->ItemAt(1);
	delete newObjFunction;

	if (fSolver.Result() == UNBOUNDED)
		return kMaxSize;
	if (fSolver.Result() != OPTIMAL) {
		fSolver.Save("failed-layout.txt");
		printf("Could not solve the layout specification (%d). "
			"Saved specification in file failed-layout.txt", fSolver.Result());
	}

	return BSize(Right()->Value() - Left()->Value(),
		Bottom()->Value() - Top()->Value());
}


/**
 * Caculates the preferred size.
 */
BSize
BALMLayout::_CalculatePreferredSize()
{
	_UpdateAreaConstraints();

	_SolveLayout();
	if (fSolver.Result() != OPTIMAL) {
		fSolver.Save("failed-layout.txt");
		printf("Could not solve the layout specification (%d). "
			"Saved specification in file failed-layout.txt", fSolver.Result());
	}

	return BSize(Right()->Value() - Left()->Value(),
		Bottom()->Value() - Top()->Value());
}


void
BALMLayout::_UpdateAreaConstraints()
{
	for (int i = 0; i < CountItems(); i++)
		AreaFor(ItemAt(i))->InvalidateSizeConstraints();
}