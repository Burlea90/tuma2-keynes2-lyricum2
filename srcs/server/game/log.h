#ifndef __INC_LOG_MANAGER_H__
#define __INC_LOG_MANAGER_H__

#include "../libsql/AsyncSQL.h"
#include "locale_service.h"

enum log_level {LOG_LEVEL_NONE=0, LOG_LEVEL_MIN=1, LOG_LEVEL_MID=2, LOG_LEVEL_MAX=3};

#define LOG_LEVEL_CHECK_N_RET(x) { if (g_iDbLogLevel < x) return; }

#define LOG_LEVEL_CHECK(x, fnc)	\
	{\
		if (g_iDbLogLevel >= (x))\
			fnc;\
	}

enum GOLDBAR_HOW
{
	PERSONAL_SHOP_BUY	= 1 ,
	PERSONAL_SHOP_SELL	= 2 ,
	SHOP_BUY			= 3 ,
	SHOP_SELL			= 4 ,
	EXCHANGE_TAKE		= 5 ,
	EXCHANGE_GIVE		= 6 ,
	QUEST				= 7 ,
};

class LogManager : public singleton<LogManager>
{
	public:
		LogManager();
		virtual ~LogManager();

		bool		IsConnected();

		bool		Connect(const char * host, const int port, const char * user, const char * pwd, const char * db);

		void		ItemLog(DWORD dwPID, DWORD x, DWORD y, DWORD dwItemID, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, DWORD dwVnum);
		void		ItemLog(LPCHARACTER ch, LPITEM item, const char * c_pszText, const char * c_pszHint);
		void		ItemLog(LPCHARACTER ch, int itemID, int itemVnum, const char * c_pszText, const char * c_pszHint);

		void		CharLog(DWORD dwPID, DWORD x, DWORD y, DWORD dw, const char * c_pszText, const char * c_pszHint, const char * c_pszIP);
		void		CharLog(LPCHARACTER ch, DWORD dw, const char * c_pszText, const char * c_pszHint);

		void		LoginLog(bool isLogin, DWORD dwAccountID, DWORD dwPID, BYTE bLevel, BYTE bJob, DWORD dwPlayTime);
#if defined(ENABLE_NEW_GOLD_LIMIT)
		void MoneyLog(BYTE type, DWORD vnum, uint64_t gold);
#else
		void MoneyLog(BYTE type, DWORD vnum, uint32_t gold);
#endif
		void		HackLog(const char * c_pszHackName, const char * c_pszLogin, const char * c_pszName, const char * c_pszIP);
		void		HackLog(const char * c_pszHackName, LPCHARACTER ch);
		void		HackCRCLog(const char * c_pszHackName, const char * c_pszLogin, const char * c_pszName, const char * c_pszIP, DWORD dwCRC);
		void		GoldBarLog(DWORD dwPID, DWORD dwItemID, GOLDBAR_HOW eHow, const char * c_pszHint);
#ifdef __ATTR_TRANSFER_SYSTEM__
		void	AttrTransferLog(DWORD dwPID, DWORD x, DWORD y, DWORD item_vnum);
#endif
		void		CubeLog(DWORD dwPID, DWORD x, DWORD y, DWORD item_vnum, DWORD item_uid, int item_count, bool success);
		void		GMCommandLog(DWORD dwPID, const char * szName, const char * szIP, BYTE byChannel, const char * szCommand);
		void		ChangeNameLog(DWORD pid, const char * old_name, const char * new_name, const char * ip);
		void		RefineLog(DWORD pid, const char * item_name, DWORD item_id, int item_refine_level, int is_success, const char * how);
		void		ShoutLog(BYTE bChannel, BYTE bEmpire, const char * pszText);
		void		LevelLog(LPCHARACTER pChar, unsigned int level, unsigned int playhour);
		void		BootLog(const char * c_pszHostName, BYTE bChannel);
		void		FishLog(DWORD dwPID, int prob_idx, int fish_id, int fish_level, DWORD dwMiliseconds, DWORD dwVnum = false, DWORD dwValue = 0);
		void		QuestRewardLog(const char * c_pszQuestName, DWORD dwPID, DWORD dwLevel, int iValue1, int iValue2);
		void		DetailLoginLog(bool isLogin, LPCHARACTER ch);
		void		DragonSlayLog(DWORD dwGuildID, DWORD dwDragonVnum, DWORD dwStartTime, DWORD dwEndTime);
		void		InvalidServerLog(enum eLocalization eLocaleType, const char* pcszIP, const char* pszRevision);
		void		ChatLog(DWORD where, DWORD who_id, const char* who_name, DWORD whom_id, const char* whom_name, const char* type, const char* msg, const char* ip);

#ifdef ENABLE_ACCE_SYSTEM
		void		AcceLog(DWORD dwPID, DWORD x, DWORD y, DWORD item_vnum, DWORD item_uid, int item_count, int abs_chance, bool success);
#endif
#ifdef ENABLE_NEW_OFFLINESHOP_LOGS
		void		OfflineshopLog(const DWORD dwOwnerID, const DWORD dwItemID, const char* fmt, ...);
#endif
#if defined(ENABLE_NEW_ANTICHEAT_RULES)
		void		HackReport(uint32_t, const char*, const char*, const char*);
#endif
#if defined(ENABLE_OKEY_CARD_GAME)
		void OkayEventLog(uint32_t, const char*, uint32_t);
#endif

		size_t EscapeString(char* dst, size_t dstSize, const char *src, size_t srcSize);
	private:
		void		Query(const char * c_pszFormat, ...);

		CAsyncSQL	m_sql;
		bool		m_bIsConnect;
};

#endif
