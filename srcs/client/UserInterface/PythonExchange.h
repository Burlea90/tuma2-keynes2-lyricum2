#pragma once

#include "Packet.h"

/*
 *	��ȯ â ����
 */
class CPythonExchange : public CSingleton<CPythonExchange>
{
	public:
		enum
		{
#ifdef ENABLE_NEW_EXCHANGE_WINDOW
			EXCHANGE_ITEM_MAX_NUM = 24,
#else
			EXCHANGE_ITEM_MAX_NUM = 12,
#endif
		};

		typedef struct trade
		{
			char					name[CHARACTER_NAME_MAX_LEN + 1];

#if defined (ENABLE_LEVEL_IN_TRADE) && defined(ENABLE_NEW_EXCHANGE_WINDOW)
			DWORD					level;
#endif

			DWORD					item_vnum[EXCHANGE_ITEM_MAX_NUM];
			WORD						item_count[EXCHANGE_ITEM_MAX_NUM];
			DWORD					item_metin[EXCHANGE_ITEM_MAX_NUM][ITEM_SOCKET_SLOT_MAX_NUM];
			TPlayerItemAttribute	item_attr[EXCHANGE_ITEM_MAX_NUM][ITEM_ATTRIBUTE_SLOT_MAX_NUM];

			BYTE					accept;
#if defined(ENABLE_NEW_GOLD_LIMIT)
			uint64_t elk;
#else
			uint32_t elk;
#endif
#if defined(ENABLE_CHANGELOOK)
			uint32_t transmutation[EXCHANGE_ITEM_MAX_NUM];
#endif
#ifdef ATTR_LOCK
			short					lockedattr[EXCHANGE_ITEM_MAX_NUM];
#endif
#ifdef ENABLE_NEW_EXCHANGE_WINDOW
			DWORD					race;
#endif
		} TExchangeData;

	public:
		CPythonExchange();
		virtual ~CPythonExchange();

		void			Clear();

		void			Start();
		void			End();
		bool			isTrading();

		// Interface

		void			SetSelfName(const char *name);
		void			SetTargetName(const char *name);

		char			*GetNameFromSelf();
		char			*GetNameFromTarget();

#if defined (ENABLE_LEVEL_IN_TRADE) && defined(ENABLE_NEW_EXCHANGE_WINDOW)
		void			SetSelfLevel(DWORD level);
		void			SetTargetLevel(DWORD level);

		DWORD			GetLevelFromSelf();
		DWORD			GetLevelFromTarget();
#endif
#if defined(ENABLE_NEW_GOLD_LIMIT)
		void SetElkToTarget(uint64_t);
		void SetElkToSelf(uint64_t);
		uint64_t GetElkFromTarget() const;
		uint64_t GetElkFromSelf() const;
#else
		void SetElkToTarget(uint32_t);
		void SetElkToSelf(uint32_t);
		uint32_t GetElkFromTarget() const;
		uint32_t GetElkFromSelf() const;
#endif

#ifdef ENABLE_NEW_EXCHANGE_WINDOW
		void			SetSelfRace(DWORD race);
		void			SetTargetRace(DWORD race);
		DWORD			GetRaceFromSelf();
		DWORD			GetRaceFromTarget();
#endif

		void			SetItemToTarget(DWORD pos, DWORD vnum, WORD count);
		void			SetItemToSelf(DWORD pos, DWORD vnum, WORD count);

		void			SetItemMetinSocketToTarget(int pos, int imetinpos, DWORD vnum);
		void			SetItemMetinSocketToSelf(int pos, int imetinpos, DWORD vnum);

		void			SetItemAttributeToTarget(int pos, int iattrpos, BYTE byType, short sValue);
		void			SetItemAttributeToSelf(int pos, int iattrpos, BYTE byType, short sValue);

		void			DelItemOfTarget(BYTE pos);
		void			DelItemOfSelf(BYTE pos);

		DWORD			GetItemVnumFromTarget(BYTE pos);
		DWORD			GetItemVnumFromSelf(BYTE pos);

		WORD GetItemCountFromTarget(BYTE pos);
		WORD GetItemCountFromSelf(BYTE pos);

		DWORD			GetItemMetinSocketFromTarget(BYTE pos, int iMetinSocketPos);
		DWORD			GetItemMetinSocketFromSelf(BYTE pos, int iMetinSocketPos);

		void			GetItemAttributeFromTarget(BYTE pos, int iAttrPos, BYTE * pbyType, short * psValue);
		void			GetItemAttributeFromSelf(BYTE pos, int iAttrPos, BYTE * pbyType, short * psValue);
#if defined(ENABLE_CHANGELOOK)
		void SetItemTransmutation(int32_t, uint32_t, bool);
		uint32_t GetItemTransmutation(int32_t, bool);
#endif

		void			SetAcceptToTarget(BYTE Accept);
		void			SetAcceptToSelf(BYTE Accept);

#ifdef ATTR_LOCK
		void			SetItemAttrLocked(int iPos, short dwTransmutation, bool bSelf);
		short			GetItemAttrLocked(int iPos, bool bSelf);
#endif

		bool			GetAcceptFromTarget();
		bool			GetAcceptFromSelf();

		bool			GetElkMode();
		void			SetElkMode(bool value);

	protected:
		bool				m_isTrading;

		bool				m_elk_mode;   // ��ũ�� Ŭ���ؼ� ��ȯ�������� ���� ������.
		TExchangeData		m_self;
		TExchangeData		m_victim;
};
