//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2023 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
//
#include "StdAfx.h"
#include "atlimage.h"

#if defined(_DEBUG) && !defined(AC_FULL_DEBUG)
#error _DEBUG should not be defined except in internal Adesk debug builds
#endif

#include "ArxDbgUiTdcEntities.h"
#include "ArxDbgUtils.h"
#include "ArxDbgUtilsGe.h"



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
**
**  ArxDbgUiTdcEntities::ArxDbgUiTdcEntities
**
**  **jma
**
*************************************/

ArxDbgUiTdcEntities::ArxDbgUiTdcEntities(const AcDbObjectIdArray& objList, bool isBlkDef)
:   m_origEnts(objList),
    m_isBlkDef(isBlkDef)
{
    //{{AFX_DATA_INIT(ArxDbgUiTdcEntities)
    //}}AFX_DATA_INIT
}

/****************************************************************************
**
**  ArxDbgUiTdcEntities::DoDataExchange
**
**  **jma
**
*************************************/

void
ArxDbgUiTdcEntities::DoDataExchange(CDataExchange* pDX)
{
    ArxDbgUiTdcDbObjectBase::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(ArxDbgUiTdcEntities)
    DDX_Control(pDX, ARXDBG_TXT_ERASE_STATUS, m_txtErased);
    DDX_Control(pDX, ARXDBG_LC_DATA, m_dataList);
    DDX_Control(pDX, ARXDBG_TREE_ENTS, m_entTree);
    //}}AFX_DATA_MAP
    DDX_Control(pDX, ARXDBG_FR_BLKPREVIEW, m_BlkPreview);
}

/////////////////////////////////////////////////////////////////////////////
// ArxDbgUiTdcEntities message map

BEGIN_MESSAGE_MAP(ArxDbgUiTdcEntities, ArxDbgUiTdcDbObjectBase)
    //{{AFX_MSG_MAP(ArxDbgUiTdcEntities)
    ON_WM_PAINT()
    ON_NOTIFY(TVN_SELCHANGED, ARXDBG_TREE_ENTS, OnSelchangedEnts)
    ON_BN_CLICKED(ARXDBG_BN_DATABASE, OnDatabase)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ArxDbgUiTdcEntities message handlers

/****************************************************************************
**
**  ArxDbgUiTdcEntities::OnInitDialog
**
**  **jma
**
*************************************/

BOOL
ArxDbgUiTdcEntities::OnInitDialog() 
{
    ArxDbgUiTdcDbObjectBase::OnInitDialog();

	AcDbEntity* ent;
	Acad::ErrorStatus es;
    HTREEITEM treeItem;
	CString str;

	int len = m_origEnts.length();
	for (int i=0; i<len; i++) {
		es = acdbOpenAcDbEntity(ent, m_origEnts[i], AcDb::kForRead, true);		// might have passed in erased ones
		if (es == Acad::eOk) {
			treeItem = addOneTreeItem(ArxDbgUtils::objToClassAndHandleStr(ent, str), ent, TVI_ROOT);
			if (treeItem != NULL) {
				addSubentitiesToTree(ent, treeItem);
				if (i == 0)
					m_entTree.SelectItem(treeItem);		// make first one selected
			}
			ent->close();
		}
	}

    buildColumns(m_dataList);
    displayCurrent(0);

    return TRUE;
}

void ArxDbgUiTdcEntities::OnPaint()
{
	CPaintDC dc(this); // device context for painting

    bool enabled = true;
    AcArray<Adesk::UInt8> icon;
    do 
    {
		AcDbObject* pObj = NULL;
		Acad::ErrorStatus es = acdbOpenObject(pObj, m_currentObjId, AcDb::kForRead, true);	// might want to show erased
        if (es != eOk || !pObj)
        {
            enabled = false;
            break;
        }
        AcDbBlockReference* pBlkRef = AcDbBlockReference::cast(pObj);
        if (!pBlkRef)
        {
            enabled = false;
            pObj->close();
            break;
        }
        AcDbObjectId btrId = pBlkRef->blockTableRecord();
        pObj->close();
        
        AcDbBlockTableRecord* pBtr = NULL;
        es = acdbOpenObject(pBtr, btrId, AcDb::kForRead, true);	// might want to show erased
        if (es != eOk || !pBtr)
        {
			enabled = false;
			break;
        }

        if (!pBtr->hasPreviewIcon())
        {
            enabled = false;
            pBtr->close();
            break;
        }

        es = pBtr->getPreviewIcon(icon);
        pBtr->close();
        if (icon.isEmpty())
        {
            enabled = false;
        }

    } while (false);

    m_BlkPreview.EnableWindow(enabled);
    if (enabled)
    {
		//BITMAPINFOHEADER ih;
		//memcpy(&ih, icon.asArrayPtr(), sizeof(ih));
		//size_t memsize = sizeof(BITMAPINFOHEADER) + ((1 << ih.biBitCount) * sizeof(RGBQUAD));
		//LPBITMAPINFO bi = (LPBITMAPINFO)malloc(memsize);
		//memcpy(bi, icon.asArrayPtr(), memsize);
		//HBITMAP hbm = CreateDIBitmap(dc.GetSafeHdc(), &ih, CBM_INIT, icon.asArrayPtr() + memsize, bi, DIB_RGB_COLORS);
		//free(bi);
		const BITMAPINFOHEADER* pBmih = reinterpret_cast<const BITMAPINFOHEADER*>(icon.asArrayPtr());
		
		// -----------------------------------------------------------
		// 步骤 2：构造 BITMAPINFO（包含颜色表）
		// -----------------------------------------------------------
		const BYTE* pColorTable = icon.asArrayPtr() + pBmih->biSize;
		BITMAPINFO* pBmi = (BITMAPINFO*)pBmih;

		// -----------------------------------------------------------
		// 步骤 3：获取像素数据指针
		// -----------------------------------------------------------
		const BYTE* pPixelData = icon.asArrayPtr() + pBmih->biSize + (pBmih->biClrUsed * sizeof(RGBQUAD));

		// -----------------------------------------------------------
		// 步骤 4：创建 HBITMAP
		// -----------------------------------------------------------
		HBITMAP hBitmap = CreateDIBitmap(
			dc.GetSafeHdc(),          // 设备上下文
			pBmih,             // BITMAPINFOHEADER
			CBM_INIT,          // 初始化标志
			pPixelData,        // 像素数据
			pBmi,              // BITMAPINFO（含颜色表）
			DIB_RGB_COLORS     // 颜色表类型
		);

        HBITMAP hOldBitMap = m_BlkPreview.GetBitmap();
        if (hOldBitMap)
        {
            DeleteObject(hOldBitMap);
        }
		m_BlkPreview.SetBitmap(hBitmap);
		m_BlkPreview.Invalidate();
    }

	/*if (es == Acad::eOk)
	{
		m_frPreview.EnableWindow(TRUE);
		if (!acdbDisplayPreviewFromDwg(fname, m_frPreview.m_hWnd))
			ArxDbgUtils::stopAlertBox(_T("ERROR: Could not display preview image!"));
	}
	else
	{
		m_frPreview.EnableWindow(FALSE);
	}*/
}

/****************************************************************************
**
**  ArxDbgUiTdcEntities::OnSelchangedEnts
**
**  **jma
**
*************************************/

void
ArxDbgUiTdcEntities::OnSelchangedEnts(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    displayCurrent(static_cast<int>(pNMTreeView->itemNew.lParam));    
    *pResult = 0;
}

/****************************************************************************
**
**  ArxDbgUiTdcEntities::addOneTreeItem
**
**  **jma
**
*************************************/

HTREEITEM
ArxDbgUiTdcEntities::addOneTreeItem(LPCTSTR name, const AcDbObject* obj, HTREEITEM parent, bool sort)
{
    m_treeObjIdList.append(obj->objectId());    // keep track of the objectId for each entry
    int index = m_treeObjIdList.length() - 1;
    ASSERT(index >= 0);

    TV_ITEM    tvItem;
    TV_INSERTSTRUCT tvInsert;

    tvItem.mask = TVIF_TEXT | TVIF_PARAM;
    tvItem.pszText = const_cast<TCHAR*>(name);
    tvItem.cchTextMax = lstrlen(name);
    tvItem.lParam = (LPARAM)index;        //index of AcDbObjectId
    
    tvInsert.item = tvItem;
    if (sort)
        tvInsert.hInsertAfter = TVI_SORT;
    else
        tvInsert.hInsertAfter = TVI_LAST;
    tvInsert.hParent = parent;

    return m_entTree.InsertItem(&tvInsert);            
}

/****************************************************************************
**
**  ArxDbgUiTdcEntities::addSubentitiesToTree
**
**  **jma
**
*************************************/

void
ArxDbgUiTdcEntities::addSubentitiesToTree(const AcDbEntity* mainEnt, HTREEITEM parent)
{
	AcDbObjectIdArray subEnts;

    AcDbBlockReference* blkRef;
	
	if ((blkRef = AcDbBlockReference::cast(mainEnt)) != NULL) {
		collectAttributes(blkRef, subEnts);
	}
	else if ((mainEnt->isKindOf(AcDbPolyFaceMesh::desc())) ||
			 (mainEnt->isKindOf(AcDbPolygonMesh::desc())) ||
			 (mainEnt->isKindOf(AcDb2dPolyline::desc())) ||
			 (mainEnt->isKindOf(AcDb3dPolyline::desc()))) {
		collectVertices(mainEnt, subEnts);
	}

	AcDbObject* tmpObj;
	HTREEITEM treeItem;
	Acad::ErrorStatus es;
	CString str;

	int len = subEnts.length();
	for (int i=0; i<len; i++) {
		es = acdbOpenAcDbObject(tmpObj, subEnts[i], AcDb::kForRead, true);		// might have passed in erased ones
		if (es == Acad::eOk) {
			treeItem = addOneTreeItem(ArxDbgUtils::objToClassAndHandleStr(tmpObj, str), tmpObj, parent);
			tmpObj->close();
		}
		else
			ArxDbgUtils::rxErrorMsg(es);
	}
}

/****************************************************************************
**
**  ArxDbgUiTdcEntities::displayCurrent
**
**  **jma
**
*************************************/

void
ArxDbgUiTdcEntities::displayCurrent(int index)
{
		// remove any previous entries
    m_fieldStrList.RemoveAll();
    m_valueStrList.RemoveAll();

    ASSERT((index >= 0) && (index < m_treeObjIdList.length()));
    m_currentObjId = m_treeObjIdList[index];

    CString str;

    AcDbObject* obj = NULL;
    Acad::ErrorStatus es = acdbOpenObject(obj, m_currentObjId, AcDb::kForRead, true);	// might want to show erased
    setExtensionButtons(obj);

    if (es == Acad::eOk) {
		display(obj);

		    // hide or show the erased entity message
		if (obj->isErased())
			m_txtErased.ShowWindow(SW_SHOW);
		else
			m_txtErased.ShowWindow(SW_HIDE);

        obj->close();
    }

	drawPropsList(m_dataList);
}

/****************************************************************************
**
**  ArxDbgUiTdcEntities::collectAttributes
**
**  **jma
**
*************************************/

void
ArxDbgUiTdcEntities::collectAttributes(const AcDbBlockReference* blkRef, AcDbObjectIdArray& subEnts)
{
	ASSERT(blkRef != NULL);

    AcDbObjectIterator* attribIter = blkRef->attributeIterator();
    if (attribIter != NULL) {
        while (attribIter->done() == false) {
            subEnts.append(attribIter->objectId());
            attribIter->step();
        }
        delete attribIter;
    }
}

/****************************************************************************
**
**  ArxDbgUiTdcEntities::collectVertices
**
**  **jma
**
*************************************/

void
ArxDbgUiTdcEntities::collectVertices(const AcDbEntity* ent, AcDbObjectIdArray& subEnts)
{
    AcDb2dPolyline* pline2d;
    AcDb3dPolyline* pline3d;
    AcDbPolygonMesh* pmesh;
    AcDbPolyFaceMesh* pface;

    AcDbObjectIterator* vertexIter;

    if ((pline2d = AcDb2dPolyline::cast(ent)) != NULL)
        vertexIter = pline2d->vertexIterator();
    else if ((pline3d = AcDb3dPolyline::cast(ent)) != NULL)
        vertexIter = pline3d->vertexIterator();
    else if ((pmesh = AcDbPolygonMesh::cast(ent)) != NULL)
        vertexIter = pmesh->vertexIterator();
    else if ((pface = AcDbPolyFaceMesh::cast(ent)) != NULL)
        vertexIter = pface->vertexIterator();
    else
        vertexIter = NULL;

	if (vertexIter == NULL) {
		ASSERT(0);
		return;
	}

    while (vertexIter->done() == false) {
		subEnts.append(vertexIter->objectId());
        vertexIter->step();
    }

    delete vertexIter;
}

