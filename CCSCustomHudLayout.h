#pragma once
#include "CBaseEntity.h"
#include <utlstring.h>
#include <schemasystem/baseschema.h>

// custom_hud_layout entity (Panorama custom HUD layouts).
// Based on the CS2Fixes / CounterStrikeSharp implementation (GPLv3, Source2ZE).
//
// The vectors are network vectors, so every element goes through the game's own
// schema collection manipulator: inserting that way runs the game's element
// constructor (HUDPanelDialogVariableString_t has a vtable) and keeps the
// allocation inside the game's vector code.

enum EHudPanelClassStatus_t : int
{
	k_eHudPanelClassStatus_Undefined = -1,
	k_eHudPanelClassStatus_DoesNotHaveClass = 0,
	k_eHudPanelClassStatus_HasClass = 1,
};

// Not in the server schema scope, layout from the client dump.
struct HUDPanelHasClass_t
{
	uint16 m_nPanelIdIndex;
	uint16 m_nClassNameIndex;
	EHudPanelClassStatus_t m_eClassStatus;
};

struct HUDPanelDialogVariableString_t
{
	void* m_pVTable;
	uint16 m_nPanelIdIndex;
	uint16 m_nDialogVariableIndex;
	CUtlString m_sValue;
	bool m_bIsSet;
};

namespace SchemaCollection
{
	inline int Count(SchemaCollectionManipulatorFn_t fn, void* pCollection)
	{
		if(!fn || !pCollection) return 0;
		return (int)(intptr_t)fn(SCHEMA_COLLECTION_MANIPULATOR_ACTION_GET_COUNT, pCollection, 0, 0);
	}

	inline void* Element(SchemaCollectionManipulatorFn_t fn, void* pCollection, int i)
	{
		if(!fn || !pCollection || i < 0 || i >= Count(fn, pCollection)) return nullptr;
		return fn(SCHEMA_COLLECTION_MANIPULATOR_ACTION_GET_ELEMENT, pCollection, i, 0);
	}

	// Appends one default-constructed element, returns its index or -1
	inline int AddToTail(SchemaCollectionManipulatorFn_t fn, void* pCollection)
	{
		if(!fn || !pCollection) return -1;
		int iCount = Count(fn, pCollection);
		fn(SCHEMA_COLLECTION_MANIPULATOR_ACTION_INSERT_BEFORE, pCollection, iCount, 1);
		return Count(fn, pCollection) == iCount + 1 ? iCount : -1;
	}

	// Index of szValue in a vector of CUtlString, appended when missing
	inline int FindOrAddString(SchemaCollectionManipulatorFn_t fn, void* pCollection, const char* szValue)
	{
		if(!szValue) szValue = "";
		int iCount = Count(fn, pCollection);
		for(int i = 0; i < iCount; i++)
		{
			CUtlString* pStr = (CUtlString*)Element(fn, pCollection, i);
			if(pStr && !V_strcmp(pStr->Get(), szValue))
				return i;
		}
		int i = AddToTail(fn, pCollection);
		if(i == -1) return -1;
		*(CUtlString*)Element(fn, pCollection, i) = szValue;
		return i;
	}

	inline int FindString(SchemaCollectionManipulatorFn_t fn, void* pCollection, const char* szValue)
	{
		if(!szValue) szValue = "";
		int iCount = Count(fn, pCollection);
		for(int i = 0; i < iCount; i++)
		{
			CUtlString* pStr = (CUtlString*)Element(fn, pCollection, i);
			if(pStr && !V_strcmp(pStr->Get(), szValue))
				return i;
		}
		return -1;
	}
}

class CCSCustomHudLayoutState
{
public:
	DECLARE_SCHEMA_CLASS_INLINE(CCSCustomHudLayoutState)

	SCHEMA_FIELD(int, m_playerSlot);
	SCHEMA_FIELD(bool, m_bInputCaptureEnabled);
	SCHEMA_FIELD_POINTER(void, m_vecHasClasses);
	SCHEMA_FIELD_POINTER(void, m_vecDialogVariableStrings);

	void SetHasClass(int iPanel, int iClass, bool bHasClass)
	{
		auto fn = m_vecHasClasses.GetManipulator();
		void* pVec = m_vecHasClasses.Get();
		EHudPanelClassStatus_t eStatus = bHasClass ? k_eHudPanelClassStatus_HasClass : k_eHudPanelClassStatus_DoesNotHaveClass;
		int iCount = SchemaCollection::Count(fn, pVec);
		for(int i = 0; i < iCount; i++)
		{
			auto pElem = (HUDPanelHasClass_t*)SchemaCollection::Element(fn, pVec, i);
			if(pElem && pElem->m_nPanelIdIndex == iPanel && pElem->m_nClassNameIndex == iClass)
			{
				pElem->m_eClassStatus = eStatus;
				return;
			}
		}
		int i = SchemaCollection::AddToTail(fn, pVec);
		auto pElem = (HUDPanelHasClass_t*)SchemaCollection::Element(fn, pVec, i);
		if(!pElem) return;
		pElem->m_nPanelIdIndex = (uint16)iPanel;
		pElem->m_nClassNameIndex = (uint16)iClass;
		pElem->m_eClassStatus = eStatus;
	}

	HUDPanelDialogVariableString_t* FindDialogVariable(int iPanel, int iVariable)
	{
		auto fn = m_vecDialogVariableStrings.GetManipulator();
		void* pVec = m_vecDialogVariableStrings.Get();
		int iCount = SchemaCollection::Count(fn, pVec);
		for(int i = 0; i < iCount; i++)
		{
			auto pElem = (HUDPanelDialogVariableString_t*)SchemaCollection::Element(fn, pVec, i);
			if(pElem && pElem->m_nPanelIdIndex == iPanel && pElem->m_nDialogVariableIndex == iVariable)
				return pElem;
		}
		return nullptr;
	}

	void SetDialogVariableString(int iPanel, int iVariable, const char* szValue)
	{
		auto pElem = FindDialogVariable(iPanel, iVariable);
		if(!pElem)
		{
			auto fn = m_vecDialogVariableStrings.GetManipulator();
			void* pVec = m_vecDialogVariableStrings.Get();
			int i = SchemaCollection::AddToTail(fn, pVec);
			pElem = (HUDPanelDialogVariableString_t*)SchemaCollection::Element(fn, pVec, i);
			if(!pElem) return;
			pElem->m_nPanelIdIndex = (uint16)iPanel;
			pElem->m_nDialogVariableIndex = (uint16)iVariable;
		}
		pElem->m_sValue = szValue ? szValue : "";
		pElem->m_bIsSet = true;
	}
};

class CCSCustomHudLayout : public CBaseEntity
{
public:
	DECLARE_SCHEMA_CLASS(CCSCustomHudLayout)

	SCHEMA_FIELD(CUtlSymbolLarge, m_strLayout);
	SCHEMA_FIELD_POINTER(void, m_vecPlayerLayoutStates);
	SCHEMA_FIELD_POINTER(CCSCustomHudLayoutState, m_globalLayoutState);
	SCHEMA_FIELD_POINTER(void, m_vecPanelIds);
	SCHEMA_FIELD_POINTER(void, m_vecClassNames);
	SCHEMA_FIELD_POINTER(void, m_vecDialogVariableNames);

	// Per-player state, matched on m_playerSlot (the vector is indexed by slot in practice)
	CCSCustomHudLayoutState* GetPlayerState(int iSlot)
	{
		auto fn = m_vecPlayerLayoutStates.GetManipulator();
		void* pVec = m_vecPlayerLayoutStates.Get();
		auto pState = (CCSCustomHudLayoutState*)SchemaCollection::Element(fn, pVec, iSlot);
		if(pState && pState->m_playerSlot() == iSlot)
			return pState;
		int iCount = SchemaCollection::Count(fn, pVec);
		for(int i = 0; i < iCount; i++)
		{
			pState = (CCSCustomHudLayoutState*)SchemaCollection::Element(fn, pVec, i);
			if(pState && pState->m_playerSlot() == iSlot)
				return pState;
		}
		return nullptr;
	}

	int PanelIndex(const char* szPanelId) { return SchemaCollection::FindOrAddString(m_vecPanelIds.GetManipulator(), m_vecPanelIds.Get(), szPanelId); }
	int ClassIndex(const char* szClassName) { return SchemaCollection::FindOrAddString(m_vecClassNames.GetManipulator(), m_vecClassNames.Get(), szClassName); }
	int VariableIndex(const char* szVariable) { return SchemaCollection::FindOrAddString(m_vecDialogVariableNames.GetManipulator(), m_vecDialogVariableNames.Get(), szVariable); }

	void StateChanged(bool bGlobal)
	{
		m_vecPanelIds.NetworkStateChanged();
		m_vecClassNames.NetworkStateChanged();
		m_vecDialogVariableNames.NetworkStateChanged();
		if(bGlobal)
			m_globalLayoutState.NetworkStateChanged();
		else
			m_vecPlayerLayoutStates.NetworkStateChanged();
	}

	void SetHasClass(CCSCustomHudLayoutState* pState, bool bGlobal, const char* szPanelId, const char* szClassName, bool bHasClass)
	{
		if(!pState) return;
		int iPanel = PanelIndex(szPanelId);
		int iClass = ClassIndex(szClassName);
		if(iPanel == -1 || iClass == -1) return;
		pState->SetHasClass(iPanel, iClass, bHasClass);
		StateChanged(bGlobal);
	}

	void SetDialogVariableString(CCSCustomHudLayoutState* pState, bool bGlobal, const char* szPanelId, const char* szVariable, const char* szValue)
	{
		if(!pState) return;
		int iPanel = PanelIndex(szPanelId);
		int iVariable = VariableIndex(szVariable);
		if(iPanel == -1 || iVariable == -1) return;
		pState->SetDialogVariableString(iPanel, iVariable, szValue);
		StateChanged(bGlobal);
	}

	void SetHasClass(int iSlot, const char* szPanelId, const char* szClassName, bool bHasClass)
	{
		SetHasClass(GetPlayerState(iSlot), false, szPanelId, szClassName, bHasClass);
	}

	void SetGlobalHasClass(const char* szPanelId, const char* szClassName, bool bHasClass)
	{
		SetHasClass(m_globalLayoutState.Get(), true, szPanelId, szClassName, bHasClass);
	}

	void SetDialogVariableString(int iSlot, const char* szPanelId, const char* szVariable, const char* szValue)
	{
		SetDialogVariableString(GetPlayerState(iSlot), false, szPanelId, szVariable, szValue);
	}

	void SetGlobalDialogVariableString(const char* szPanelId, const char* szVariable, const char* szValue)
	{
		SetDialogVariableString(m_globalLayoutState.Get(), true, szPanelId, szVariable, szValue);
	}

	const char* GetDialogVariableString(int iSlot, const char* szPanelId, const char* szVariable)
	{
		CCSCustomHudLayoutState* pState = GetPlayerState(iSlot);
		if(!pState) return "";
		int iPanel = SchemaCollection::FindString(m_vecPanelIds.GetManipulator(), m_vecPanelIds.Get(), szPanelId);
		int iVariable = SchemaCollection::FindString(m_vecDialogVariableNames.GetManipulator(), m_vecDialogVariableNames.Get(), szVariable);
		if(iPanel == -1 || iVariable == -1) return "";
		auto pElem = pState->FindDialogVariable(iPanel, iVariable);
		return pElem && pElem->m_bIsSet ? pElem->m_sValue.Get() : "";
	}

	void SetInputCaptureEnabled(int iSlot, bool bEnable)
	{
		CCSCustomHudLayoutState* pState = GetPlayerState(iSlot);
		if(!pState) return;
		pState->m_bInputCaptureEnabled() = bEnable;
		m_vecPlayerLayoutStates.NetworkStateChanged();
	}

	bool GetInputCaptureEnabled(int iSlot)
	{
		CCSCustomHudLayoutState* pState = GetPlayerState(iSlot);
		return pState ? pState->m_bInputCaptureEnabled() : false;
	}

	void SetGlobalInputCaptureEnabled(bool bEnable)
	{
		m_globalLayoutState->m_bInputCaptureEnabled() = bEnable;
		m_globalLayoutState.NetworkStateChanged();
	}
};
