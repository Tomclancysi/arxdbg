﻿//////////////////////////////////////////////////////////////////////////////
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

#if defined(_DEBUG) && !defined(AC_FULL_DEBUG)
#error _DEBUG should not be defined except in internal Adesk debug builds
#endif

#include "ArxDbgApp.h"
#include "ArxDbgUiDlgSplash.h"
#include "ArxDbgDbDictRecord.h"
#include "ArxDbgWorldDraw.h"
#include "ArxDbgDwgFiler.h"
#include "ArxDbgReferenceFiler.h"
#include "ArxDbgAppEditorReactor.h"
#include "ArxDbgDatabaseReactor.h"
#include "ArxDbgDLinkerReactor.h"
#include "ArxDbgDocumentReactor.h"
#include "ArxDbgEditorReactor.h"
#include "ArxDbgEventReactor.h"
#include "ArxDbgGsReactor.h"
#include "ArxDbgLayoutManagerReactor.h"
#include "ArxDbgLongTransactionReactor.h"
#include "ArxDbgTransactionReactor.h"
#include "ArxDbgDbEntity.h"
#include "ArxDbgDbAdeskLogo.h"
#include "ArxDbgDbAdeskLogoStyle.h"
#include "MapTest.h"
#include "ArxDbgUiTdcOptions.h"
#include "AcExtensionModule.h"
#include "ArxDbgPersistentEntReactor.h"
#include "ArxDbgPersistentObjReactor.h"
#include "ArxDbgTransientEntReactor.h"
#include "ArxDbgTransientObjReactor.h"
#include "ArxDbgEdUiContext.h"
#include "ArxDbgCmdAdeskLogo.h"
#include "ArxDbgCmdSnoop.h"
#include "ArxDbgCmdReactors.h"
#include "ArxDbgCmdTests.h"
#include "ArxDbgUiTdmReactors.h"
#include "ArxDbgSelSet.h"
#include "ArxDbgUtils.h"

#include "dbsymutl.h"
#include "dbobjptr2.h"
#include "EntMakeTest.h"
#include <chrono>

extern void cmdAboutBox();
extern void mapTestExportDwg();
extern void mapTestImportDwg();

//二开代码开始

void testHw()
{
#define XDT_AGE                 20000
#define XDT_RANK                20001
#define XDT_BIRTHDAY            20002
#define XDT_FAVORITE_ANIMALS    20003
#define XDT_MISC                20004
#define XDT_HANDLE              20005
	AcDbObjectId objId;
	AcDbObject* obj;

	if (!ArxDbgUtils::selectEntityOrObject(objId))
		return;

	Acad::ErrorStatus es = acdbOpenAcDbObject(obj, objId, AcDb::kForWrite);
	if (es != Acad::eOk)
	{
		ArxDbgUtils::rxErrorMsg(es);
		return;
	}

	// add xdata for first app
	ArxDbgAppXdata connor(_T("CONNOR"), obj->database());
	connor.setDistance(XDT_AGE, 6.0);
	connor.setInteger(XDT_RANK, 1);
	connor.setString(XDT_BIRTHDAY, _T("January 14, 1993"));
    connor.setHandle(XDT_HANDLE, AcDbHandle(_T("1F")));

	// add in a uniform list
	ArxDbgXdataList list1;
	list1.appendString(_T("Tiger"));
	list1.appendString(_T("Lion"));
	list1.appendString(_T("Wolf"));
	connor.setList(XDT_FAVORITE_ANIMALS, list1);

	connor.setXdata(obj);

    ///

	AcDbDictionary* extDict = ArxDbgUtils::openExtDictForWrite(obj, true);
    if (extDict)
    {
        auto prKey = _T("A_Circle");
        if (extDict->has(prKey))
        {
            acutPrintf(_T("\n\"%s\" is already in the extension dictionary."),
                static_cast<LPCTSTR>(prKey));
        }
        else
        {
            // make an Xrecord to stick into the dictionary
            AcDbXrecord* xRec = new AcDbXrecord;
            resbuf* rb = acutBuildList(2, static_cast<LPCTSTR>(_T("CircleValue")), 0);
            xRec->setFromRbChain(*rb);

            AcDbObjectId newObjId;
            es = extDict->setAt(static_cast<LPCTSTR>(prKey), xRec, newObjId);
            if (es != Acad::eOk)
            {
                ArxDbgUtils::rxErrorMsg(es);
                delete xRec;
            }
            else
            {
                acutPrintf(_T("\nAdded it to the dictionary!"));
                xRec->close();
            }
        }
        extDict->close();
    }
    obj->close();
}



void testInsertSpeed()
{
    AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();
    AcDbObjectId blkId;
    AcString blkNameTemplate(_T("tempBlk%d"));
    AcString blkName;
    resbuf rb;
    int nRet = acedGetFileD(_T("选择插入的图纸"), NULL, _T("dwg"), 0, &rb);
    if (nRet != RTNORM || rb.restype != RTSTR)
    {
        return;
    }

    auto kszFileName = rb.resval.rstring;
    for (size_t i = 0; i < 10000; i++)
    {
        blkName.format(blkNameTemplate.kTCharPtr(), i);
        if (!AcDbSymbolUtilities::hasBlock(blkName.kTCharPtr(), pDb))
        {
            break;
        }
    }

    AcDbDatabase* pInsertDb = new AcDbDatabase;
    if (pInsertDb->readDwgFile(kszFileName))
    {
        return;
    }
    
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    if (pDb->insert(blkId, blkName.kTCharPtr(), pInsertDb))
    {
        return;
    }
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    acutPrintf(_T("\nMake Block Time: %d"), milliseconds);

	AcDbBlockReference* blkRef = new AcDbBlockReference;

	// set this block to point at the correct block ref
	Acad::ErrorStatus es = blkRef->setBlockTableRecord(blkId);
	if (es != Acad::eOk)
	{
		delete blkRef;
		ArxDbgUtils::rxErrorAlert(es);
		return;
	}

	ads_point adsPt; //用于存储用户输入的点
    AcGePoint3d insertPt(0, 0, 0);
	int res = acedGetPoint(NULL, _T("\n选择一个插入点: "), adsPt);
    if (res == RTNORM)
    {
        insertPt.set(adsPt[0], adsPt[1], adsPt[2]);
    }

	blkRef->setPosition(insertPt);
    {
	    AcDbBlockTableRecord* blkDef = NULL;
	    acdbOpenObject(blkDef, blkId, AcDb::kForRead);

	    AcDbExtents extents;
        if (extents.addBlockExt(blkDef) == Acad::eOk)
        {
			AcGePoint3d minPt = extents.minPoint();
            insertPt.set(insertPt.x - minPt.x, insertPt.y - minPt.y, insertPt.z - minPt.z);
			blkRef->setPosition(insertPt);
        }
        if (blkDef)
        {
	        blkDef->close();
        }
    }
	blkRef->setRotation(0.);
	blkRef->setScaleFactors(1.);

	if (ArxDbgUtils::addToCurrentSpace(blkRef) == Acad::eOk)
	{
        EntMakeDbox::makeAttributes(blkId, blkRef);
		ArxDbgUtils::transformToWcs(blkRef, acdbHostApplicationServices()->workingDatabase());
		blkRef->close();
	}

}

class MyCircle : public AcDbCircle
{
public:
    ACRX_DECLARE_MEMBERS(MyCircle);
    Acad::ErrorStatus   subTransformBy(const AcGeMatrix3d& xform) override
    {
        acutPrintf(_T("\nInfo:"));
        AcGePoint3d origin;
        AcGeVector3d xDir, yDir, zDir;
        xform.getCoordSystem(origin, xDir, yDir, zDir);

        acutPrintf(_T("\nOrigin: %f, %f, %f"), origin.x, origin.y, origin.z);
        acutPrintf(_T("\nXDir: %f, %f, %f"), xDir.x, xDir.y, xDir.z);
        acutPrintf(_T("\nYDir: %f, %f, %f"), yDir.x, yDir.y, yDir.z);
        acutPrintf(_T("\nZDir: %f, %f, %f"), zDir.x, zDir.y, zDir.z);

        return AcDbCircle::subTransformBy(xform);
    }

};

ACRX_DXF_DEFINE_MEMBERS(MyCircle, AcDbCircle,
	AcDb::kDHL_CURRENT, AcDb::kMReleaseCurrent,
	AcDbProxyEntity::kTransformAllowed |
	AcDbProxyEntity::kColorChangeAllowed |
	AcDbProxyEntity::kLayerChangeAllowed,
    MyCircle, Product: ZRX Enabler | Company : ZWSOFT | Website : www.zwcad.com
);

AC_IMPLEMENT_EXTENSION_MODULE(ArxDbgDll);

void addMyCircle()
{
    auto pDb = acdbHostApplicationServices()->workingDatabase();
    auto msId = acdbSymUtil()->blockModelSpaceId(pDb);
    AcDbBlockTableRecord *pMs = nullptr;
    auto es = acdbOpenObject(pMs, msId, AcDb::kForWrite);
    if (es != Acad::eOk || nullptr == pMs)
    {
        return;
    }

    MyCircle* pCir = new MyCircle;
    pCir->setCenter(AcGePoint3d(0, 0, 0));
    pCir->setRadius(50);

    es = pMs->appendAcDbEntity(pCir);
    pCir->close();
    pMs->close();

    if (es != Acad::eOk)
    {
        delete pCir;
    }
}

void addToBlock()
{
	ArxDbgSelSet ss;
	if (ss.userSelect(_T("\nselect objs append to block"), NULL, NULL) != ArxDbgSelSet::kSelected)
		return;
    AcDbObjectIdArray objIds;
    ss.asArray(objIds);
    
    AcDbEntity* pEnt = ArxDbgUtils::selectEntity(_T("\nselect a block reference:"), AcDb::kForWrite);
    if (!pEnt)
    {
        return;
    }
    AcDbBlockReference* pTmpBref = AcDbBlockReference::cast(pEnt);
    if (!pTmpBref)
    {
        pEnt->close();
        return;
    }
    // 选择的块参照由智能指针关闭
    AcDbSmartObjectPointer<AcDbBlockReference> pOldBlkRefPointer;
    pOldBlkRefPointer.acquire(pTmpBref);

    auto insPt = pOldBlkRefPointer->position();
    auto xformVec = -(insPt - AcGePoint3d::kOrigin);
    AcGeMatrix3d xform;
    xform.setTranslation(xformVec);

	AcDbObjectIdArray refIds;
    AcDbObjectId oldBtrId = pOldBlkRefPointer->blockTableRecord();
	{
        // 打开获取块的引用个数
		AcDbSmartObjectPointer<AcDbBlockTableRecord> pBtr(oldBtrId, AcDb::kForRead);
		if (pBtr.openStatus() != Acad::eOk)
		{
			return;
		}
		pBtr->getBlockReferenceIds(refIds);
	}

    if (refIds.logicalLength() == 1)
    {
        // 块定义更新
        auto pDb = acdbHostApplicationServices()->workingDatabase();
        AcDbIdMapping idmap;
        auto es = pDb->deepCloneObjects(objIds, oldBtrId, idmap);
        for (auto objId : objIds)
        {
            AcDbIdPair idPair(objId, AcDbObjectId::kNull, true, true);
            idmap.compute(idPair);
            if (idPair.value().isNull())
            {
                continue;
            }
            AcDbSmartObjectPointer<AcDbEntity> pNewEnt(idPair.value(), kForWrite);
            if (pNewEnt.openStatus() == eOk)
            {
                pNewEnt->transformBy(xform);
            }
        }
		// erase original objects
        for (auto objId : objIds)
		{
			AcDbSmartObjectPointer<AcDbEntity> pTmpEnt(objId, AcDb::kForWrite);
            pTmpEnt->erase();
		}

        // 添加新的块参照，删除老的
        pOldBlkRefPointer->erase();

        // 添加到模型空间
        AcDbBlockReference* pNewBlkRef = new AcDbBlockReference;
		AcDbBlockTableRecord* pBlkMs;
		es = ArxDbgUtils::openBlockDef(ACDB_MODEL_SPACE, pBlkMs, AcDb::kForWrite, pDb);
        if (es != eOk)
        {
            return;
        }
        es = pBlkMs->appendAcDbEntity(pNewBlkRef);
        if (es != eOk)
        {
            delete pNewBlkRef;
        }
        else
        {
            pNewBlkRef->setBlockTableRecord(oldBtrId);
            pNewBlkRef->setPosition(insPt);
            pNewBlkRef->close();
        }
        pBlkMs->close();

        //auto brefId = pBlkRefPointer->objectId();
        //acdbQueueForRegen((AcDbObjectId*)&brefId, 1);
        acutPrintf(_T("\nres = %d"), es);
    }
    else
    {
        // 需要新的块定义
		auto pDb = acdbHostApplicationServices()->workingDatabase();
        
		AcDbIdMapping idmap;
        AcDbObjectIdArray blkRecArr;
        blkRecArr.append(oldBtrId);
		auto es = pDb->deepCloneObjects(blkRecArr, pDb->blockTableId(), idmap);
        acutPrintf(_T("\nres = %d"), es);
    }
}



ArxDbgApp*    ThisApp = NULL;

/****************************************************************************
**
**  DllMain
**
**  **jma
**
*************************************/

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        ArxDbgDll.AttachInstance(hInstance);
        ThisApp = new ArxDbgApp;
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        ArxDbgDll.DetachInstance();  
        if (ThisApp) {
            delete ThisApp;
            ThisApp = NULL;
        }
    }
    return 1;   // ok
}

/****************************************************************************
**
**  acrxEntryPoint:
**
**  **jma
**
*************************************/

extern "C" __declspec(dllexport) AcRx::AppRetCode
acrxEntryPoint(AcRx::AppMsgCode msg, void* pPkt)
{
    return ThisApp->entryPoint(msg, pPkt);
}

/****************************************************************************
**
**  ArxDbgApp::getApp
**      static function to get access to this module in general
**
**  **jma
**
*************************************/

ArxDbgApp*
ArxDbgApp::getApp()
{
    return ThisApp;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/****************************************************************************
**
**  ArxDbgApp::ArxDbgApp
**
**  **jma
**
*************************************/

ArxDbgApp::ArxDbgApp()
:   m_isUnlockable(true),
    m_acadAppPtr(NULL),
    m_appServicePtr(NULL),
    m_appName(_T("ArxDbg")),
    m_verStr(_T("2.0")),
    m_appId(0),
    m_didInit(false),
	m_tdcOptions(NULL)
{
	m_moduleName = m_appName;
#ifdef _DEBUG
    m_moduleName += _T("D");
#endif
}

/****************************************************************************
**
**  ArxDbgApp::~ArxDbgApp
**
**  **jma
**
*************************************/

ArxDbgApp::~ArxDbgApp()
{
	//ASSERT(m_tdcOptions == NULL);	// TBD: should have been deleted by a call to endDialog

	if (m_tdcOptions)
		delete m_tdcOptions;
}

static const TCHAR acGeomentObjDbxFile[] = _T("AcGeomentObj.dbx");

/****************************************************************************
**
**  ArxDbgApp::entryPoint
**      print out message saying that we recieved the app message from ARX
**
**  **jma
**
*************************************/

AcRx::AppRetCode
ArxDbgApp::entryPoint(AcRx::AppMsgCode msg, void* pkt)
{
    if (msg == AcRx::kInitAppMsg) {
		m_acadAppPtr = pkt;	// keep track of this for later use
        acutPrintf(_T("\nAPPMSG: %s, kInitAppMsg"), m_appName);
        acrxLoadModule(acGeomentObjDbxFile, 0);
        return initApp();
    }
    else if (msg == AcRx::kUnloadAppMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kUnloadAppMsg"), m_appName);
        acrxUnloadModule(acGeomentObjDbxFile);
        return exitApp();
    }
    else if (msg == AcRx::kLoadDwgMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kLoadDwgMsg"), m_appName);
        return initDwg();
    }
    else if (msg == AcRx::kUnloadDwgMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kUnloadDwgMsg"), m_appName);
        return exitDwg();
    }
    else if (msg == AcRx::kInvkSubrMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kInvkSubrMsg"), m_appName);
        return invokeSubr();
    }
    else if (msg == AcRx::kCfgMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kCfgMsg"), m_appName);
        return config();
    }
    else if (msg == AcRx::kEndMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kEndMsg"), m_appName);
        return endDwg();
    }
    else if (msg == AcRx::kQuitMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kQuitMsg"), m_appName);
        return quitDwg();
    }
    else if (msg == AcRx::kSaveMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kSaveMsg"), m_appName);
        return saveDwg();
    }
    else if (msg == AcRx::kDependencyMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kDependencyMsg"), m_appName);

        if (m_appServicePtr == pkt) {
            acutPrintf(_T("\nLocking app: %s"), m_appName);
            acrxDynamicLinker->lockApplication(m_acadAppPtr);
        }

        return AcRx::kRetOK;
    }
    else if (msg == AcRx::kNoDependencyMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kNoDependencyMsg"), m_appName);

        if (m_appServicePtr == pkt) {
            acutPrintf(_T("\nUnlocking app: %s"), m_appName);
            acrxDynamicLinker->unlockApplication(m_acadAppPtr);
        }

        return AcRx::kRetOK;
    }
    else if (msg == AcRx::kOleUnloadAppMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kOleUnloadAppMsg"), m_appName);
        return AcRx::kRetOK;
    }
    else if (msg == AcRx::kPreQuitMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kPreQuitMsg"), m_appName);
        return AcRx::kRetOK;
    }
    else if (msg == AcRx::kInitDialogMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kInitDialogMsg"), m_appName);
        return initDialog(pkt);
    }
    else if (msg == AcRx::kEndDialogMsg) {
        acutPrintf(_T("\nAPPMSG: %s, kEndDialogMsg"), m_appName);
        return endDialog(pkt);
    }
    else {
        ASSERT(0);        // just see if it ever happens
        return AcRx::kRetOK;
    }
}

/****************************************************************************
**
**  ArxDbgApp::initApp
**      Called one time only each time the application is loaded. Init
**  only those things that are specific to the application itself, not the
**  current drawing.
**
**  **jma
**
*************************************/

AcRx::AppRetCode
ArxDbgApp::initApp()
{
    acrxUnlockApplication(m_acadAppPtr);
    acrxDynamicLinker->registerAppMDIAware(m_acadAppPtr);

        // get the name of this app so we can find where other
        // things are located.
    CString appFileName = acedGetAppName();

    TCHAR dir[_MAX_DIR], drive[_MAX_DRIVE], path[_MAX_PATH];
    _tsplitpath_s(appFileName, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
    _tmakepath(path, drive, dir, NULL, NULL);
    m_appPath = path;

	//CWnd* splashScreen = startSplashScreen();

    registerClasses();
    registerCommands();
    acrxBuildClassHierarchy();

    m_appServicePtr = acrxRegisterService(_T("ArxDbgServices"));

    ArxDbgAppEditorReactor::getInstance();
    MapTestReactor::getInstance();

	registerDialogExtensions();
	registerAppMenu();

	//endSplashScreen(splashScreen); // Splash开启软件时的提示信息

    m_didInit = true;
    return AcRx::kRetOK;
}

/**************************************************************************
**
**  ArxDbgApp::exitApp:
**      Clean up any application related stuff.  Note that we must
**  be unlocked in order to allow unloading.
**
**  **jma
**
*************************************/

AcRx::AppRetCode
ArxDbgApp::exitApp()
{
	unRegisterAppMenu();

        // delete any of the notification spies that have been allocated
    ArxDbgUiTdmReactors::cleanUpReactors();
    ArxDbgAppEditorReactor::destroyInstance();
	MapTestReactor::destroyInstance();

    if (m_didInit) {
        unRegisterCommands();
        unRegisterClasses();

        acutPrintf(_T("\n%s has been unloaded ... "), appName());
    }

    return AcRx::kRetOK;
}

/****************************************************************************
**
**  ArxDbgApp::initDwg:
**      Called each time a drawing is loaded. Initialize drawing
**  specific information.
**
**  **jma
**
*************************************/

AcRx::AppRetCode
ArxDbgApp::initDwg()
{
		// this is just a place-holder for tests
    return AcRx::kRetOK;
}

/**************************************************************************
**
**  ArxDbgApp::exitDwg:
**      Called each time a drawing is exited. Clean up anything
**  that is drawing specific.
**
**  **jma
**
*************************************/

AcRx::AppRetCode
ArxDbgApp::exitDwg()
{
		// this is just a place-holder for tests
    return AcRx::kRetOK;
}

/**************************************************************************
**
**  ArxDbgApp::saveDwg:
**
**  **jma
**
*************************************/

AcRx::AppRetCode
ArxDbgApp::saveDwg()
{
		// this is just a place-holder for tests
    return AcRx::kRetOK;
}

/**************************************************************************
**
**  ArxDbgApp::endDwg:
**
**  **jma
**
*************************************/

AcRx::AppRetCode
ArxDbgApp::endDwg()
{
		// this is just a place-holder for tests
    return AcRx::kRetOK;
}

/**************************************************************************
**
**  ArxDbgApp::quitDwg
**
**  **jma
**
*************************************/

AcRx::AppRetCode
ArxDbgApp::quitDwg()
{
		// this is just a place-holder for tests
    return AcRx::kRetOK;
}

/**************************************************************************
**
**  ArxDbgApp::config
**
**  **jma
**
*************************************/

AcRx::AppRetCode
ArxDbgApp::config()
{
		// this is just a place-holder for tests
    return AcRx::kRetOK;
}

/**************************************************************************
**
**  ArxDbgApp::invokeSubr
**
**  **jma
**
*************************************/

AcRx::AppRetCode
ArxDbgApp::invokeSubr()
{
		// this is just a place-holder for tests
    return AcRx::kRetOK;
}

/****************************************************************************
**
**  ArxDbgApp::initDialog
**      Called indirectly from a tab-extensible dialog (during initialization) 
**      that this app is registered with.
**
**  **jma
**
*************************************/

AcRx::AppRetCode
ArxDbgApp::initDialog(void* pkt)
{
    CAdUiTabExtensionManager* pTabExtMgr = static_cast<CAdUiTabExtensionManager*>(pkt);

        // Get name of extensible dialog
    CString tabDlgName = pTabExtMgr->GetDialogName();

        // Add the "ArxDbg" tab to the Options dialog
    if (!tabDlgName.CompareNoCase(_T("OptionsDialog"))) {

			// TBD: It seems that we have to hold on to the dialog between invocations.
			// I would think that we would get a call to endDialog that would allow us
			// to delete the dialog after it closes... but that doesn't appear to be
			// the case.
		if (m_tdcOptions == NULL)
			m_tdcOptions = new ArxDbgUiTdcOptions;

        if (m_tdcOptions) {
            pTabExtMgr->AddTab(dllInstance(), ARXDBG_TDC_OPTIONS, _T("ArxDbg"), m_tdcOptions);
        }
    }

    return AcRx::kRetOK;
}

/****************************************************************************
**
**  ArxDbgApp::endDialog
**
**  **jma
**
*************************************/

AcRx::AppRetCode
ArxDbgApp::endDialog(void* pTabExtMgr)
{
		// TBD: why does this never get called?
	delete m_tdcOptions;
    m_tdcOptions = NULL;

    return AcRx::kRetOK;
}

/**************************************************************************
**
**  ArxDbgApp::registerClasses:
**		call rxInit on all classes we have defined that are derived
**	from AcRxObject or any of its descendants
**
**  **jma
**
*************************************/

void
ArxDbgApp::registerClasses()
{
    ArxDbgPersistentObjReactor::rxInit();    // our test case for persistent object reactors
    ArxDbgPersistentEntReactor::rxInit();    // our test case for persistent entity reactors
    ArxDbgTransientObjReactor::rxInit();     // our test case for transient object reactors
    ArxDbgTransientEntReactor::rxInit();     // our test case for transient entity reactors

    ArxDbgAppEditorReactor::rxInit();
    ArxDbgDwgFiler::rxInit();
    ArxDbgReferenceFiler::rxInit();

    ArxDbgGiContext::rxInit();
    ArxDbgGiSubEntityTraits::rxInit();
    ArxDbgGiWorldGeometry::rxInit();
    ArxDbgGiWorldDraw::rxInit();

    ArxDbgDbDictRecord::rxInit();    // test case for basic dictionary entry
	ArxDbgDbEntity::rxInit();
    ArxDbgDbAdeskLogo::rxInit();
    ArxDbgDbAdeskLogoStyle::rxInit();

	ArxDbgDatabaseReactor::rxInit();
	ArxDbgDLinkerReactor::rxInit();
	ArxDbgDocumentReactor::rxInit();
	ArxDbgEditorReactor::rxInit();
    ArxDbgEventReactor::rxInit();
	ArxDbgLayoutManagerReactor::rxInit();
	ArxDbgLongTransactionReactor::rxInit();
	ArxDbgTransactionReactor::rxInit();
}

/**************************************************************************
**
**  ArxDbgApp::unRegisterClasses:
**      reverse everything done in ArxDbgApp::registerClasses()
**
**  **jma
**
*************************************/

void
ArxDbgApp::unRegisterClasses()
{
    deleteAcRxClass(ArxDbgPersistentEntReactor::desc());
    deleteAcRxClass(ArxDbgPersistentObjReactor::desc());
    deleteAcRxClass(ArxDbgTransientEntReactor::desc());
    deleteAcRxClass(ArxDbgTransientObjReactor::desc());

    deleteAcRxClass(ArxDbgAppEditorReactor::desc());
    deleteAcRxClass(ArxDbgDwgFiler::desc());
    deleteAcRxClass(ArxDbgReferenceFiler::desc());

    deleteAcRxClass(ArxDbgGiWorldDraw::desc());
    deleteAcRxClass(ArxDbgGiWorldGeometry::desc());
    deleteAcRxClass(ArxDbgGiSubEntityTraits::desc());
    deleteAcRxClass(ArxDbgGiContext::desc());

    deleteAcRxClass(ArxDbgDbAdeskLogo::desc());
    deleteAcRxClass(ArxDbgDbAdeskLogoStyle::desc());
    deleteAcRxClass(ArxDbgDbDictRecord::desc());
	deleteAcRxClass(ArxDbgDbEntity::desc());

	deleteAcRxClass(ArxDbgDatabaseReactor::desc());
	deleteAcRxClass(ArxDbgDLinkerReactor::desc());
	deleteAcRxClass(ArxDbgDocumentReactor::desc());
	deleteAcRxClass(ArxDbgEditorReactor::desc());
    deleteAcRxClass(ArxDbgEventReactor::desc());
	deleteAcRxClass(ArxDbgLayoutManagerReactor::desc());
	deleteAcRxClass(ArxDbgLongTransactionReactor::desc());
	deleteAcRxClass(ArxDbgTransactionReactor::desc());
}

/**************************************************************************
**
**  ArxDbgApp::registerCommands:
**
**  **jma
**
*************************************/

void
ArxDbgApp::registerCommands()
{
		// allocate the command classes.  These will in turn register their own
		// group of CmdLine functions through AcEd::addCommand()
	m_cmdClasses.append(new ArxDbgCmdAdeskLogo);
	m_cmdClasses.append(new ArxDbgCmdSnoop);
	m_cmdClasses.append(new ArxDbgCmdReactors);
	m_cmdClasses.append(new ArxDbgCmdTests);

    AcEdCommandStack* cmdStack = acedRegCmds;
    VERIFY(cmdStack != NULL);

	ArxDbgCmd* tmpCmdClass;
	int len = m_cmdClasses.length();
	for (int i=0; i<len; i++) {
		tmpCmdClass = static_cast<ArxDbgCmd*>(m_cmdClasses[i]);
		tmpCmdClass->registerCommandLineFunctions(cmdStack, m_appName);
	}

		// add in a few stragglers that don't have their own ArxDbgCmd class yet
    cmdStack->addCommand(m_appName, _T("ArxDbgAbout"), _T("ArxDbgAbout"), ACRX_CMD_MODAL, &cmdAboutBox);

		// GIS Map tests for ADE
	cmdStack->addCommand(m_appName, _T("ArxDbgMapTestExport"), _T("MapTestExport"), ACRX_CMD_MODAL, &mapTestExportDwg);
    cmdStack->addCommand(m_appName, _T("ArxDbgMapTestImport"), _T("MapTestImport"), ACRX_CMD_MODAL, &mapTestImportDwg);

#define register_cmd(cmd) acedRegCmds->addCommand(m_appName, L#cmd, L#cmd, ACRX_CMD_MODAL, cmd)
    register_cmd(testHw);
    register_cmd(testInsertSpeed);
    register_cmd(addMyCircle);
    register_cmd(addToBlock);

    MyCircle::rxInit();
    acrxBuildClassHierarchy();
}

/**************************************************************************
**
**  ArxDbgApp::unRegisterCommands:
**
**  **jma
**
*************************************/

void
ArxDbgApp::unRegisterCommands()
{
		// delete any command classes we have allocated
	ArxDbgCmd* tmpCmdClass;
	int len = m_cmdClasses.length();
	for (int i=0; i<len; i++) {
		tmpCmdClass = static_cast<ArxDbgCmd*>(m_cmdClasses[i]);
		delete tmpCmdClass;
	}

	m_cmdClasses.setLogicalLength(0);

    acedRegCmds->removeGroup(m_appName);    // remove any registered commands

    deleteAcRxClass(MyCircle::desc());
}

/**************************************************************************
**
**  ArxDbgApp::startSplashScreen
**
**  **jma
**
*************************************/

CWnd*
ArxDbgApp::startSplashScreen()
{
    CString bmpFileName = appPath();
    bmpFileName += _T("support\\ArxDbgSplash.bmp");    // TBD: make this better later!

        // make sure file is there (either in a main directory, which is
		// where someone would install it, or for developers, go out of 
		// "Debug" directory and up to the main directory.
    CFileStatus status;
    // Watch out for exceptions from GetStatus() due to bad date fields on the file.
    BOOL success = false;
    try {
        success = CFile::GetStatus(bmpFileName, status); // try/catch safe
        if (success == FALSE) {
    	    bmpFileName = appPath();
    	    //x64 binaries are created within x64\ folder
#if defined(_WIN64) || defined(_AC64)
        	bmpFileName += _T("..\\..\\support\\ArxDbgSplash.bmp");  // TBD: make this better later!
#else
        	bmpFileName += _T("..\\support\\ArxDbgSplash.bmp");  // TBD: make this better later!
#endif
    	    success = CFile::GetStatus(bmpFileName, status);  // try/catch safe
        }
    }
    catch (COleException* e) {
        e->Delete();
        // If we get here the file exists but it has a bad date field
        success = true;
    }

	if (success) {
        ArxDbgUiDlgSplash* dbox = new ArxDbgUiDlgSplash(bmpFileName);
        dbox->setTextStrings(appName(), versionStr(), _T(""), _T(""));

        // TBD: have to do this manually right now, although it should
        // have worked to pass up resource handle through the constructor!
        {
            CAcModuleResourceOverride resOverride;
            dbox->Create(acedGetAcadDwgView());
        }

        dbox->ShowWindow(SW_SHOW);
        dbox->UpdateWindow();

        return dbox;
    }
    else {        // intended a splash screen but could not find it!
        acutPrintf(_T("\nCould not find splash box image: \"%s\""), bmpFileName);
        return NULL;
    }
}

/**************************************************************************
**
**  ArxDbgApp::endSplashScreen
**
**  **jma
**
*************************************/

void
ArxDbgApp::endSplashScreen(CWnd* splashScreen)
{
    if (splashScreen) {
        Sleep(1000);    // waste a little bit of time to ensure that they see it
        splashScreen->ShowWindow(SW_HIDE);
        splashScreen->UpdateWindow();
        splashScreen->DestroyWindow();
        delete splashScreen;
        acedGetAcadDwgView()->Invalidate();
        acedGetAcadDwgView()->UpdateWindow();
    }
}

/**************************************************************************
**
**  ArxDbgApp::registerAppMenu
**
**  **jma
**
*************************************/

void
ArxDbgApp::registerAppMenu()
{
    m_uiContext = new ArxDbgEdUiContextApp;

	if ((m_uiContext->isValid() == false) ||
		(acedAddDefaultContextMenu(m_uiContext, m_acadAppPtr, _T("ArxDbg")) == false)) {
		ASSERT(0);

        delete m_uiContext;
        m_uiContext = NULL;
    }
}

/**************************************************************************
**
**  ArxDbgApp::unRegisterAppMenu
**
**  **jma
**
*************************************/

void
ArxDbgApp::unRegisterAppMenu()
{
    if (m_uiContext) {
        acedRemoveDefaultContextMenu(m_uiContext);
        delete m_uiContext;
        m_uiContext = NULL;
    }
}

/**************************************************************************
**
**  ArxDbgApp::registerDialogExtensions
**
**  **jfk
**
*************************************/

void
ArxDbgApp::registerDialogExtensions()
{
        // Register the fact that we want to add a tab to the following dialogs
    acedRegisterExtendedTab(m_moduleName, _T("OptionsDialog"));
}

/**************************************************************************
**
**  ArxDbgApp::dllInstance
**
**  **jma
**
*************************************/

HINSTANCE
ArxDbgApp::dllInstance() const
{
	return ArxDbgDll.ModuleResourceInstance();
}

